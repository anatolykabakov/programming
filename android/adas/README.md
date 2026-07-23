# ADAS (Android + C++)

Приложение для записи сенсоров / vision и нативного стека ADAS на телефоне (Panda USB, ZMQ, lane-keep, localization, VP-calib).

**Алгоритмы только в C++** (сервисы внутри `AdasApp`). Python — bag visualizer, MetaDrive sim и тонкие обёртки: publish входов → `step` → `pop_messages`.

Все команды ниже — из корня проекта:

```bash
cd programming/android/adas
```

## Структура

```
adas/                              ← корень (здесь README и run_*.sh)
├── build_project.sh               # APK (+ C++ Android .so)
├── install_dependencies.sh
├── run_bag_vis.sh                 # bag visualizer
├── run_sim.sh                     # MetaDrive sim
└── app/
    ├── libs/arm64-v8a/            # libadas_app_android.so
    └── src/main/
        ├── java/…/adas/           # UI, Logger, ZMQ, camera, vision ONNX
        ├── cpp/                   # native AdasApp + services
        ├── proto/                 # protobuf (bag / ZMQ)
        ├── assets/                # config.json, supercombo.onnx, DBC
        └── scripts/               # Python: vis, sim, core, pyadas
```

### Java (`app/src/main/java/…/adas/`)

| Файл / пакет | Назначение |
|---|---|
| `MainActivity` | UI, старт сервисов |
| `Logger` / `BagLogger` | запись bag + forward на ZMQ IN |
| `ZMQBridgeService` | PUB→`:5555`, SUB←`:5556` |
| `CameraHandler` / `IMUHandler` / `GPSHandler` | сенсоры → bag/ZMQ |
| `vision/` | Supercombo ONNX → `vision/lanes` |
| `AdasConfig` | читает `assets/config.json` |
| `ProtoUtils` | сборка `ZMQMessage` |

### C++ (`app/src/main/cpp/`)

| Путь | Назначение |
|---|---|
| `include/adas_app.h` | RealTime (телефон) / Simulated (host) |
| `include/services/` | Panda, ZmqBridge, TopicConvert, LaneKeep, Localization, CameraCalib, InternalSubscriber |
| `include/utils/` | PurePursuit, EKF, VP calib, topic convert |
| `src/python/` | pybind11 → `pyadas` (только `AdasApp` + DTO) |
| `scripts/build_cpp.sh` | Conan + CMake (`-t android\|linux [--python]`) |

### Assets (`app/src/main/assets/`)

| Файл | Назначение |
|---|---|
| `config.json` | флаги нод + priors калибровки / vehicle |
| `supercombo.onnx` | модель разметки (`assets/`; host ищет её автоматически) |
| `vw_mqb_2010.dbc` | CAN Golf / MQB |

### Python (`app/src/main/scripts/`)

| Путь | Назначение |
|---|---|
| `vis/interactive_visualizer.py` | просмотр bag (линии из `vision/lanes` по умолчанию) |
| `sim/main.py` | MetaDrive lane-keep через `AdasApp` |
| `core/` | glue: lane_keep, VP Hough, viz (без своих алгоритмов управления) |
| `pyadas/` | `core*.so` после linux `--python` |

## Host API (`pyadas`)

Симулированный `AdasApp` — тот же каркас, что на телефоне. Сервисы и алгоритмы в Python **не** экспортируются.

```python
from pyadas import AdasApp, LaneKeepOutput, LocalizationPose, CameraCalibrationState

app = AdasApp(wheelbase=2.636, pitch0_deg=0.0, camera_height=1.40)
app.publish_chassis(t_us, speed_mps=10.0, steer_rad=0.0)
app.publish_lanes(t_us, [(1, 0), (10, 0.1), (30, 0.2)])
app.publish_gps(t_us, x, y)
app.publish_imu(t_us, yaw_rate)
app.publish_lane_uv(t_us, left_uv, right_uv)
app.step(t_us)

for msg in app.pop_messages():
    if isinstance(msg, LaneKeepOutput):
        ...
    elif isinstance(msg, LocalizationPose):
        ...
    elif isinstance(msg, CameraCalibrationState):
        ...
```

Подробнее: [`app/src/main/cpp/src/python/README.md`](app/src/main/cpp/src/python/README.md).

## Сборка

### Android (телефон)

```bash
./install_dependencies.sh          # при необходимости
./build_project.sh                 # debug APK + native arm64
./build_project.sh --cpp-only      # только .so
# или напрямую:
./app/src/main/cpp/scripts/build_cpp.sh -t android
```

APK: `app/build/outputs/apk/…`
`.so`: `app/libs/arm64-v8a/libadas_app_android.so`

Нужны `ANDROID_HOME` / NDK (`local.properties`), Java 17+, Conan 2.

### Linux + pyadas (sim / bag)

```bash
./app/src/main/cpp/scripts/build_cpp.sh -t linux --python
# → app/src/main/scripts/pyadas/core*.so

pip install -r app/src/main/scripts/requirements.txt
pip install -r app/src/main/scripts/sim/requirements.txt   # metadrive-simulator, НЕ metadrive
```

## Запуск bag visualizer

```bash
./run_bag_vis.sh /path/to/bag_session
```

Session dir или `.zip` / `.tar.gz`. Нужен собранный `pyadas`.

| UI | Смысл |
|---|---|
| **Lanes → Bag** | оверлей из `vision/lanes` (по умолчанию; ONNX на хосте не нужен) |
| **Lanes → Runtime** | локальный `supercombo.onnx` с **тем же** calib warp + RNN, что Android |
| **VP calib** | vanishing-point через `AdasApp` (`publish_lane_uv` → `step` → `pop_messages`) |
| **PP** | Pure Pursuit через `AdasApp`; polyline = `laneLinesToPath` (device Y-right), как TopicConvert |

Паритет кадров/PP: [`docs/HOST_ANDROID_PARITY.md`](docs/HOST_ANDROID_PARITY.md).

## Запуск MetaDrive sim

```bash
./run_sim.sh --controller pure_pursuit --show --lanes gt
./run_sim.sh --controller pure_pursuit --show --lanes supercombo
./run_sim.sh --controller straight --show
```

| Флаг | Смысл |
|---|---|
| `--controller` | `straight` / `pure_pursuit` |
| `--lanes` | `gt` (MetaDrive GT) или `supercombo` (нужен ONNX) |
| `--pp-on` | при `supercombo`: `plan` (default) или `lanes` |
| `--show` | окно с overlay |
| `--overlay` | сохранять кадры с overlay в `--out-dir` |

Управление — только C++ `LaneKeepService` через Simulated `AdasApp` (не Python PD).

## Runtime (телефон)

```
Camera/ONNX → vision/lanes ─┐
Sensors / Panda ────────────┼─► Logger ─► ZMQ IN :5555
                            │              ▼
                            │         ZmqBridge → TopicConvert
                            │              ▼
                            │         LaneKeep / Localization / Calib
                            └◄── ZMQ OUT :5556 → BagLogger
```

Подробная цепочка камера→HCA CAN: [`docs/IMAGE_TO_CAN_PIPELINE.md`](docs/IMAGE_TO_CAN_PIPELINE.md).
Аудит vs flowpilot (PP + ONNX): [`docs/ADAS_AUDIT.md`](docs/ADAS_AUDIT.md).
