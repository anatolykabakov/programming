# ADAS audit vs flowpilot (2026-07-27)

Сравнение всего проекта `programming/android/adas` (телефон + bag visualizer + MetaDrive sim) с flowpilot при явных допущениях:

1. **Контроль упрощён до Pure Pursuit + angle PID** (не lateral MPC / full controlsd).
2. **Модель = ONNX + Java ORT runner** (не thrneed / SNPE dual-cam pipeline).

Критерии: **OK** — соответствует допущениям / flowpilot-паритету где заявлено; **PARTIAL** — близко, есть дыры; **GAP** — расхождение или риск; **N/A** — намеренно вне скоупа.

Подробная цепочка телефона: [`IMAGE_TO_CAN_PIPELINE.md`](IMAGE_TO_CAN_PIPELINE.md).

---

## 1. Vision (phone)

| # | Тема | Статус | Комментарий |
|---|------|--------|-------------|
| V1 | Medmodel warp `K·V·R·inv(Km·V)` | OK | `ModelCalibWarp` (fl=910, cy=47.6) |
| V2 | Layout PLAN/lanes/edges/probs | OK | как `driving.cc`; не GitHub demo ll_t |
| V3 | Y right+ device frame | OK | Parser parity; C++ mid `lll+w/2` |
| V4 | Overlay Draw remap + path lift 1.28 | OK | `LaneOverlayView` |
| V5 | Temporal stack 2 frames + RNN | OK | `prevFrame6` / rnn 512 |
| V5b | YUV→tensor vs flowpilot `Preprocess.YUV420toTensor` | PARTIAL | свой 6ch/frame путь после CPU warp; сверить numeric vs OpenCL |
| V6 | Dual wide/road cameras | N/A | один phone cam |
| V7 | thrneed / SNPE / OpenCL warp | N/A | CPU warp + ORT |
| V8 | Desire / traffic convention | PARTIAL | нули; нет UI desire |
| V9 | modelV2 cereal broadcast | N/A | свой protobuf `vision/lanes` |
| V10 | Drop frames when busy | PARTIAL | vs flowpilot queue — выше latency jitter |
| V11 | Warp image not bagged | PARTIAL | только JPEG + model_out; offline re-warp хрупкий |

---

## 2. Path / planning (замена MPC)

| # | Тема | Статус | Комментарий |
|---|------|--------|-------------|
| P1 | Plan MHP → best hyp | OK | в Java parse |
| P2 | Lane fusion mid-path core | OK | `lll+w/2`, `rll−w/2`, soft d_prob blend с PLAN |
| P2b | Full `lane_planner.get_stock_path` | PARTIAL | нет std-downweight, width-at-horizon mods, `CAMERA_OFFSET=0.08`, LC multiplier; нет F3 `get_nlp_path` |
| P3 | Lateral MPC + LanePlanner | N/A | заменено PP (намеренно) |
| P4 | Longitudinal / ACC | N/A | нет |
| P5 | Path curvature continuity | PARTIAL | PP локален; нет MPC smoothing |
| P6 | modelV2.position vs plan | PARTIAL | контроль на fused polyline, не полный modelV2 |
| P7 | `lateralPlan` / lag-adj curvature | N/A | flowpilot: MPC → `get_lag_adjusted_curvature` → LaC; у нас PP δ → SWA напрямую |

---

## 3. Control → actuators

| # | Тема | Статус | Комментарий |
|---|------|--------|-------------|
| C1 | Pure Pursuit geometry | OK | C++ `PurePursuit`, live JNI params; **заменяет MPC + lag curvature**, не весь LatControl |
| C2 | Lat angle PID → torque | OK | упрощённый `LatControlPID` после desired SWA |
| C3 | VW `steer_sign=-1` | OK | config + LaneKeep |
| C4 | chassis SWA / pressed | OK | `carStateToChassis` |
| C5 | HCA_01 + CRC / rate limits | OK | `CarController` / `mqbcan` |
| C6 | `controls_allowed` gate | OK | Panda TX + HCA UI |
| C6b | EPS HCA status / standstill | PARTIAL | decode есть; сверить все TX gates vs flowpilot CI |
| C7 | Steer cmd timeout 250 ms | OK | fail-safe |
| C8 | Full panda safety / stock alerts | PARTIAL | MQB subset; нет полного openpilot safety UX |
| C9 | Dry-run без Panda | GAP | vision/PP на UI есть; нет явного «sim steer» профиля на телефоне |
| C10 | PP params до nativeStart | OK | pending flush после `nativeStart` |
| C11 | cereal `sendcan` / `controlsState` | N/A | прямой `PandaService` TX; логирование через `controls/steer` |
---

## 4. Calibration / localization

| # | Тема | Статус | Комментарий |
|---|------|--------|-------------|
| K1 | Pose calib from cameraOdometry | OK | `CameraCalibService` |
| K2 | VP / lane_uv path | PARTIAL | native + bag/host; phone редко шлёт UV |
| K3 | Calib → Java warp feedback | OK | MainActivity при success / ≥50% |
| K4 | liveCalibration flowpilot | PARTIAL | свой proto; нет cereal liveCalibration |
| K5 | Localization EKF | PARTIAL | есть сервис; не в loop PP |
| K6 | Height in warp | N/A | height только overlay/C++ |

---

## 5. Messaging / bag

| # | Тема | Статус | Комментарий |
|---|------|--------|-------------|
| M1 | ZMQ :5555 / :5556 split | OK | |
| M2 | Topics documented | OK | `adas_topics.h`, pipeline doc |
| M3 | Bag fy vs fx | OK | `bagFy = fyBag` |
| M4 | model_out in bag only | OK | control path без тяжёлого вектора |
| M5 | Config force=false | OK | не затирает RuntimeParams |
| M6 | Latency stamps capture/infer/publish | OK | lanes + lane_keep + steer |
| M7 | cereal / logreader совместимость | N/A | свой bag format |

---

## 6. Bag visualizer (`run_bag_vis.sh`)

| # | Тема | Статус | Комментарий |
|---|------|--------|-------------|
| B1 | Lanes from bag `vision/lanes` | OK | default, без ONNX |
| B2 | Runtime ONNX на хосте | OK | assets / SUPERCOMBO_MODEL |
| B3 | PP через `pyadas.AdasApp` | OK | тот же C++ LaneKeep |
| B4 | VP calib via AdasApp | OK | |
| B5 | `y_sign` convention | OK | device Y-right for PP; `DRAW_Y_SIGN=-1` for ISO draw (`frames.py`) |
| B6 | Overlay vs phone Draw | PARTIAL | Python pinhole ISO; phone Rt Euler path — сверять глазами |
| B7 | PlotJuggler export | OK | |
| B8 | Warped model input replay | GAP | нет 1:1 replay preprocess без model_out/re-run |

---

## 7. MetaDrive sim (`run_sim.sh`)

| # | Тема | Статус | Комментарий |
|---|------|--------|-------------|
| S1 | PP = C++ AdasApp | OK | не Python PD |
| S2 | GT vs supercombo lanes | OK | |
| S3 | Camera geom vs phone | PARTIAL | sim K/RPY from MetaDrive; host warp now uses same ModelCalibWarp math |
| S4 | CAN / HCA / safety | N/A | нет Panda |
| S5 | Domain gap ONNX | GAP | MetaDrive RGB ≠ phone road; метрики осторожно |
| S6 | Longitudinal | N/A | simple speed loop |

---

## 8. Top residual risks (после прошлого фикса)

1. **HCA зависит от stock ACC `controls_allowed`** — без engage крутящий момент не уйдёт; UI теперь показывает причину.
2. **Frame drop при busy ONNX** — на слабом SoC path обновляется реже скорости камеры → PP «рваный».
3. **Bag JPEG ≠ full-res warp** — точный PP replay: bag `vision/lanes`; host re-infer на 640×360 даёт другой вход сети.
4. **Calib feedback может дёргать RPY** при колебании `cal_percent` около 50 — порог эвристический.
5. **Sim не валидирует CAN path** — перед дорогой нужен телефон + Panda + logging.
6. **Нет desire** — модель всегда «straight-ish» desire zeros; поведение в съездах хуже flowpilot.
7. **Single cam / ONNX numeric ≠ thrneed** — абсолютный паритет траекторий не ожидается.

---

## 9. Рекомендации (не сделано в этом проходе)

| Приоритет | Действие |
|-----------|----------|
| MED | Опциональная очередь 1 latest-frame вместо hard drop |
| MED | Добить fusion: std-weight / `CAMERA_OFFSET` (если нужен ближе к stock path) |
| LOW | Bag warped preview или явный offline re-warp script |
| LOW | Phone dry-run: `steer_output_enabled=false` профиль + UI badge |
| LOW | Desire stub / manual lane-change bit |
| LOW | Свести sim camera prior к Golf phone K/RPY для fair compare |
| LOW | Сверить YUV tensor numeric vs flowpilot Preprocess |

---

## 10. Вердикт

Для заявленного скоупа (**PP + ONNX Java + MQB HCA**) критический путь **камера → path → PP → PID → HCA** согласован и после последнего прохода аудита закрыты известные HIGH/MED блокеры (SWA, steer_sign, live PP JNI, calib→warp, HCA UI, bag fy, config force).

Полный паритет с flowpilot **не** достигнут и **не** требуется по допущениям: нет MPC, dual-cam thrneed, modelV2/cereal, longitudinal, полного safety UX. Основные остаточные риски — drop кадров, domain gap sim, и operational gate `controls_allowed`.
