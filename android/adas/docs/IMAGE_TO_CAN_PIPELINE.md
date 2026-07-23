# Цепочка: камера → CAN (HCA)

Подробное описание пути от кадра камеры телефона до кадров Volkswagen MQB `HCA_01` на шине через Panda.

Актуально для Android ADAS (`programming/android/adas`) с допущениями:

- модель = `supercombo.onnx` + Java ONNX Runtime;
- латеральный контроль = Pure Pursuit + angle PID (не lateral MPC flowpilot);
- целевая машина = VW Golf 7 MQB.

Связанные краткие обзоры: корневой [`README.md`](../README.md), [`vision/README.md`](../app/src/main/java/ai/flow/adas/vision/README.md), [`cpp/README.md`](../app/src/main/cpp/README.md).

---

## Обзор потока

```
Camera2 YUV 1280×720
        │
        ├─► preview TextureView
        ├─► grayscale JPEG @ 640×360 ──► bag (camera)          [если logging]
        └─► ARGB Bitmap ──► VisionPipeline (фон. поток)
                              │
                              ├─ ModelCalibWarp (512×256)
                              ├─ SupercomboOnnxRunner (ORT)
                              ├─ parse → LaneLines (+ CameraOdometry)
                              ├─ LaneOverlayView (UI)
                              ├─ bag: vision/lanes (+ model_out)
                              └─ ZMQ PUB :5555  vision/lanes  (без model_out)
                                                 model/camera_odometry
        │
        ▼
 Native ZmqBridgeService  SUB :5555
        │
        ▼
 TopicConvertService
        ├─ vision/lanes  → vision/path   (LanePathMsg polyline)
        └─ vehicle/state → vehicle/chassis
        │
        ▼
 LaneKeepService
        ├─ PurePursuit(polyline, v) → δ_road (device Y right+)
        ├─ desired_SWA = steer_sign × δ × steer_ratio
        ├─ LatControlPID(desired, actual_SWA) → steer_norm
        ├─ control/lane_keep   (геометрия PP)
        └─ controls/steer      (torque_cNm, enabled)
        │
        ▼
 PandaService @ 100 Hz TX
        ├─ safety: ignition + controls_allowed + cmd age ≤ 250 ms
        ├─ CarController → HCA_01 + LDW HUD
        └─ panda.can_send(...)
        │
        ▼
 CAN PT bus → EPS HCA
```

Параллельные ветки (не на критическом пути крутящего момента, но влияют на RPY / оверлей / bag):

| Ветка | Вход | Выход |
|-------|------|--------|
| Panda RX | CAN | `vehicle/state`, `panda/health`, `can/rx` |
| GPS / IMU (Java) | сенсоры | `sensors/gps/location`, `sensors/imu` → localization / imu_calib |
| Camera calib | `model/camera_odometry` (+ optional UV) | `calibration/camera` → UI → Java warp |
| ZMQ OUT `:5556` | native алгоритмы | bag + overlay (`control/lane_keep`, `controls/steer`, …) |

---

## Этап 1. Захват кадра

**Код:** `CameraHandler.java`

| Параметр | Значение |
|----------|----------|
| API | Camera2 |
| Формат буфера | `ImageFormat.YUV_420_888` |
| Разрешение | `W×H = 1280×720` |
| Выходы сессии | `ImageReader` + `TextureView` preview |

На каждый `onImageAvailable`:

1. `acquireLatestImage()` (дроп старых кадров).
2. Если `VisionPipeline` включён (`nodes.vision_supercombo` и инициализация OK):
   - `convertImageToArgbBitmap` → full-res ARGB;
   - `captureTs = TimeUtil.nowMs()` (BOOTTIME ms);
   - `visionPipeline.submitBitmap(color, captureTs)`;
   - bitmap сразу `recycle()` после постановки в очередь (pipeline копирует кадр).
3. Для bag (только если `Logger.isRunning()`):
   - grayscale bitmap → scale `SCALE_FACTOR=2` → **640×360**;
   - JPEG quality 70;
   - topic camera image + bag intrinsics (`bagFx/bagFy/…` из CameraCharacteristics, fy отдельно от fx).

**Важно:** в bag попадает JPEG превью, **не** warped 512×256 вход модели. Для offline re-run ONNX нужен либо JPEG+intrinsics+RPY, либо `vision/lanes.model_out`.

---

## Этап 2. VisionPipeline (очередь инференса)

**Код:** `vision/VisionPipeline.java`

- Отдельный `HandlerThread` `"SupercomboInfer"`.
- Флаг `busy`: пока кадр обрабатывается, новые кадры **отбрасываются** (не очередь).
- Копия ARGB → `SupercomboOnnxRunner.run` → `Result{lanes, pose}`.

После успешного результата:

| Действие | Куда | Примечание |
|----------|------|------------|
| `overlay.setLanes(lanes)` | UI | мгновенно |
| `ProtoUtils.createLaneLinesMessage(lanes, true)` | `Logger` / bag | **с** `model_out` (~6409 float) |
| `ProtoUtils.createLaneLinesMessage(lanes, false)` | `ZMQBridgeService.publishToNative` | **без** `model_out` (лёгкий контроль) |
| `createCameraOdometryMessage` | ZMQ + bag | если `publishPose` и pose.valid |

Topic имён: `vision/lanes`, `model/camera_odometry`.

Калибровка warp: `MainActivity.applyParamsToVision` / live RPY / inbound `calibration/camera` → `runner.setCalib(...)`.

---

## Этап 3. Calib warp (preprocess)

**Код:** `vision/ModelCalibWarp.java`

Цель — как flowpilot `getWrapMatrix` / TransformCL для **medmodel**:

\[
M_{\text{model→cam}} = K_{\text{cam}} \cdot V \cdot R(\text{rpy}) \cdot (K_{\text{med}} \cdot V)^{-1}
\]

| Константа | Значение |
|-----------|----------|
| Model size | 512×256 |
| Med focal | 910 |
| Med cy | 47.6 |
| `VIEW_FROM_DEVICE` | device (x fwd, y right, z down) → view |

`warpToModel`: для каждого пикселя модели берёт sample из camera bitmap через \(M\) (out-of-bounds = чёрный).

**Height / cam X/Y** в `RuntimeParams` на вход сети **не** влияют — только overlay / C++ priors.

---

## Этап 4. ONNX Supercombo

**Код:** `vision/SupercomboOnnxRunner.java`

| Вход | Размер / смысл |
|------|----------------|
| Image tensor | 12×128×256 = 2 кадра × 6 ch (YUV-подобная раскладка после warp+resize) |
| desire | 8 |
| traffic | 2 |
| rnn state | 512 (рекуррентно между кадрами) |

Первый полезный выход — после **2** кадров (`hasPrev`).

Модель: `/sdcard/adas_models/supercombo.onnx` → filesDir cache → `assets/supercombo.onnx` (`AdasConfig`).

### Парсинг выхода (layout driving.cc, out≈6409)

| Срез | Содержимое |
|------|------------|
| `[0:4955)` | PLAN — 5 MHP × (33×15 mean+std) + logit |
| `[4955:5483)` | 4 lanes × 33 × (y,z) + stds |
| `[5483:5491)` | lane probs — `sigmoid(prob[i*2+1])` |
| `[5491:5755)` | 2 road edges × 33 × (y,z) |
| далее | pose / прочее → `CameraOdometry` |

Система координат **device**: X forward, **Y right+**, Z up (как flowpilot Parser).
Порядок lanes в proto: leftFar, leftNear, rightNear, rightFar (near = индексы 1 и 2).
Путь на оверлее — **лучший PLAN**, не mid-lane.

Слияние в C++ path (следующий этап) может подмешивать mid-lane `lll+w/2`, `rll−w/2`
(упрощённый `get_stock_path`: без std-downweight / `CAMERA_OFFSET` / NLP).

---

## Этап 5. Публикация в native (ZMQ IN)

**Код:** `ZMQBridgeService.java` + `ZmqBridgeService` (C++)

| Сокет | Роль |
|-------|------|
| Java PUB → `tcp://127.0.0.1:5555` | сенсоры / vision → native |
| Native PUB → `tcp://127.0.0.1:5556` | алгоритмы / panda → Java bag + UI |

Формат multipart: `[topic UTF-8][ZMQMessage protobuf]`.

Java `publishToNative` пишет на IN. Native `ZmqBridgeService` на таймере ~10 ms читает IN и `publish` во внутреннюю шину Middleware с тем же topic.

Inbound с телефона (типично):

- `vision/lanes`
- `model/camera_odometry`
- `sensors/imu`, `sensors/gps/location`
- (камера JPEG **не** обязана идти в native для lane-keep)

Outbound topics (native → Java), см. `kZmqOutboundTopics`:

`can/rx`, `panda/health`, `vehicle/state`, `control/lane_keep`, `localization/pose`, `calibration/camera`, `controls/steer`, `middleware/stats`.

---

## Этап 6. TopicConvert: lanes → path, carState → chassis

**Код:** `topic_convert_service.cpp`, `utils/topic_convert.cpp`

### `vision/lanes` → `vision/path` (`laneLinesToPath`)

1. Базовый polyline из **plan_x / plan_y** (точки с \(x \ge 1\)).
2. Если есть near L/R lanes (индексы 1 и 2) с soft-prob:
   - mid: `from_l = yl + w/2`, `from_r = yr − w/2`, \(w\in[2.6,4]\);
   - смесь по вероятностям (flowpilot `lane_planner` стиль, Y right+).
3. Если есть и plan, и lanes:
   `y = d_prob * y_lane + (1-d_prob) * y_plan`.
4. Иначе — только lanes mid или только plan.

Таймстемпы: `capture_ts` / `infer_ts` прокидываются в us для latency в `control/lane_keep`.

### `vehicle/state` → `vehicle/chassis` (`carStateToChassis`)

Из MQB decode (Panda):

- `speed_mps = v_ego`
- `steering_angle_deg`, `steering_pressed`
- `steer_rad = SWA_rad / steer_ratio`
- `yaw_rate`

Без корректного SWA/pressed LatPID и driver override на MQB работают неверно.

---

## Этап 7. LaneKeep: Pure Pursuit + Lat PID

**Код:** `lane_keep_service.cpp`, `pure_pursuit.cpp`, `lat_control_pid.h`

### 7.1 Pure Pursuit

Заменяет flowpilot **LateralMpc + `get_lag_adjusted_curvature`**. Внутренний angle/torque
loop (LatPID → HCA) остаётся.

На `vision/path` polyline + скорость с chassis:

- lookahead \(L_d = \mathrm{clamp}(K_{dd}\cdot v, L_{d,\min}, L_{d,\max})\);
- rear-axle shift `pp_shift` (м назад по X);
- цель = пересечение окружности lookahead с polyline (\(x>0\));
- \(\delta = \mathrm{atan2}(2\,L\,\sin\alpha,\,L_d^2)\) → `steer_rad` (угол **колёс**, device frame).

Параметры live с UI: `ppKdd`, `ppLdMin/Max`, `ppShift`, `steerRatio` → JNI `nativeSetLaneKeepPp` / `nativeSetSteerRatio` (после `nativeStart` сбрасываются из pending).

### 7.2 Знак VW

Device Y right+ → положительный δ «вправо».
EPS MQB / HCA: крутящий момент **left-positive**.

```
desired_swa_deg = steer_sign * (steer_rad * 180/π) * steer_ratio
```

В `config.json`: `"steer_sign": -1.0`.

### 7.3 LatControlPID

На каждом chassis update и после lanes:

- `active` = `steer_output_enabled` ∧ есть цель ∧ status ok ∧ есть chassis;
- вход: `desired_swa_deg`, `actual` SWA, `steering_pressed` (unwind I);
- выход: `steer_norm ∈ [-1,1]` → `torque_cNm = round(steer_norm * max_torque_cnm)`.

Публикации:

| Topic | Содержимое |
|-------|------------|
| `control/lane_keep` | δ, Ld, target, κ, status, latency stamps (`steer_norm` геометрический до PID overwrite на steer) |
| `controls/steer` | `torque_cNm`, `enabled` |

`control/lane_keep` уходит на `:5556` → UI overlay (дуга PP, HUD).
`controls/steer` подписывает PandaService.

---

## Этап 8. Panda → CAN HCA

**Код:** `panda_service.cpp`, `volkswagen/carcontroller.cpp`, `mqbcan.cpp`

### RX / state

- Таймер ~50 ms: CAN RX → decode → `vehicle/state`, фильтрованный `can/rx`.
- ~100 ms: `panda/health` (ignition, `controls_allowed`, safety mode, heartbeat).

### TX (`carControllerCallback` @ 10 ms)

Условия `latActive`:

1. Panda connected / comms healthy;
2. safety mode Volkswagen + ignition;
3. последняя `controls/steer` не старше **250 ms**;
4. `enabled` и torque ≠ 0;
5. `controls_allowed` (обычно нужен stock ACC engage).

Дополнительно CarController/decoder учитывают EPS HCA status, standstill и driver-torque
rate limits (см. `carcontroller.cpp`, `mqb_car_state_decoder.cpp`).

Дальше `CarController::update`:

- rate/driver torque limits;
- `create_steering_control` → **HCA_01** (`0x126`) с CRC/counter;
- LDW HUD кадры.

Без Panda USB / без `controls_allowed` цепочка до этапа 7 жива (vision + PP на UI), на шину ничего не уходит.

---

## Этап 9. Обратная связь на UI

**Код:** `MainActivity.onOutboundMessage`, `LaneOverlayView`

| Сообщение | UI |
|-----------|-----|
| `control/lane_keep` | PP target / arc / status |
| `controls/steer` | torque bar, enabled |
| `panda/health` | строка HCA (ignition / controls_allowed / ok) |
| `calibration/camera` | при success или `cal_percent≥50` → params RPY/K → warp + overlay |

CAN online LED: свежесть `panda/health` ≤ 500 ms.

---

## Конфиг и флаги

`app/src/main/assets/config.json` (копируется в filesDir при первом старте, **без force overwrite** — правки RuntimeParams сохраняются):

| Ключ | Роль |
|------|------|
| `nodes.panda` / `zmq_bridge` / `lane_keep` / `localization` / `camera_calib` | native сервисы |
| `nodes.vision_supercombo` | Java ONNX pipeline |
| `vehicle.steer_ratio` / `steer_sign` / `wheelbase_m` | LaneKeep |
| `calibration.camera.*` | priors K / RPY / height |
| `supercombo_asset` | имя ONNX в assets |

`RuntimeParams` (UI sliders) мержится в тот же JSON для C++ и пушится live в PP через JNI.

---

## Bag / pull / offline

| Компонент | Назначение |
|-----------|------------|
| `BagLogger` + `Logger` | session под `/sdcard/adas_logs/...` |
| `pull_bags.sh` | `adb pull` → `./adas_logs/`, очистка на устройстве |
| `./run_bag_vis.sh` | `vis/interactive_visualizer.py` |
| `bag_overlay_lanes.py` | оверлей lanes на JPEG |
| `bag_lane_keep_offline.py` | PP через `pyadas.AdasApp` |
| `export_to_plotjuggler.py` | таймсерии topics |

Типичные bag topics: camera JPEG, intrinsics, IMU/GPS, `vision/lanes`(+model_out), odometry, outbound `vehicle/state`, `control/lane_keep`, `controls/steer`, `calibration/camera`, `panda/health`, …

Проекция bag lanes: **Y right+** → в визуализаторе чаще `y_sign=-1` при ISO left+ drawing (`supercombo_compare.py`).

---

## Sim (MetaDrive)

`./run_sim.sh` → `scripts/sim/main.py`:

1. RGB камера MetaDrive;
2. lanes = GT centerline **или** host `supercombo.onnx`;
3. `LaneKeepController` → Simulated `AdasApp.publish_chassis/lanes` → `step` → PP из **того же** C++ `LaneKeepService`;
4. steer в MetaDrive; опционально overlay.

Panda/CAN в sim нет — проверяется vision→path→PP, не HCA.

---

## Карта файлов (критический путь)

| Файл | Роль |
|------|------|
| `CameraHandler.java` | захват |
| `VisionPipeline.java` | поток + publish |
| `ModelCalibWarp.java` | homography |
| `SupercomboOnnxRunner.java` | ONNX + parse |
| `LaneLines.java` | DTO ego xyz |
| `ProtoUtils.java` | protobuf ZMQMessage |
| `ZMQBridgeService.java` | Java ↔ ZMQ |
| `zmq_bridge_service.cpp` | ZMQ ↔ Middleware |
| `topic_convert*.cpp` | lanes→path, carState→chassis |
| `lane_keep_service.cpp` | PP + PID |
| `pure_pursuit.cpp` | геометрия |
| `panda_service.cpp` | RX/TX orchestration |
| `carcontroller.cpp` / `mqbcan.cpp` | HCA_01 |
| `LaneOverlayView.java` | визуализация |
| `MainActivity.java` | wiring params / calib / HCA UI |
