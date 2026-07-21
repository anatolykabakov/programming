# ADAS (Android + C++)

Приложение для записи сенсоров / vision и нативного стека ADAS на телефоне (Panda USB, ZMQ, lane-keep, localization, VP-calib). Алгоритмы — в C++; Python — sim, bag visualizer и обёртки над `pyadas`.

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
| `include/adas_app.h` | RealTime / Simulated app |
| `include/services/` | Panda, ZmqBridge, TopicConvert, LaneKeep, Localization, CameraCalib |
| `include/utils/` | PurePursuit, EKF, VP calib, topic convert |
| `src/python/` | pybind11 → `pyadas` |
| `scripts/build_cpp.sh` | Conan + CMake (`-t android\|linux [--python]`) |

### Assets (`app/src/main/assets/`)

| Файл | Назначение |
|---|---|
| `config.json` | флаги нод + priors калибровки / vehicle |
| `supercombo.onnx` | модель разметки |
| `vw_mqb_2010.dbc` | CAN Golf / MQB |

### Python (`app/src/main/scripts/`)

| Путь | Назначение |
|---|---|
| `vis/interactive_visualizer.py` | просмотр bag |
| `sim/main.py` | MetaDrive lane-keep |
| `core/` | обёртки C++ + IMU/Hough/viz |
| `pyadas/` | `core*.so` после linux `--python` |

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
# эквивалент:
PYTHONPATH=app/src/main/scripts python3 app/src/main/scripts/interactive_visualizer.py /path/to/bag_session
```

Session dir или `.zip` / `.tar.gz`. Нужен собранный `pyadas`.

## Запуск MetaDrive sim

```bash
./run_sim.sh --controller pure_pursuit --show --lanes supercombo --compare-gt
./run_sim.sh --lanes gt --vp-source gt --show
```

С `--show` открывается **Tk UI как у bag visualizer**: камера + траектория + живые слайдеры RPY/Height и Pure Pursuit (`K_dd`, `Ld_*`, `shift`, `L_wb`), тогглы VP / lanes / supercombo. Старые OpenCV-окна: `--show --cv-show`.

| Флаг | Смысл |
|---|---|
| `--controller` | `straight` / `pure_pursuit` / `lateral_pd` |
| `--lanes` | `gt` или `supercombo` |
| `--show` | окно с overlay |
| `--compare-gt` | сравнение с GT lanes |

## Runtime (телефон)

```
Camera/ONNX → vision/lanes ─┐
Sensors / Panda ────────────┼─► Logger ─► ZMQ IN :5555
                            │              ▼
                            │         ZmqBridge → TopicConvert
                            │              │
                            │              ├─ vision/path → LaneKeep*
                            │              ├─ vehicle/chassis → LaneKeep* / Localization*
                            │              ├─ sensors/gps/location (GpsSample ENU) → Localization*
                            │              └─ sensors/imu_raw → ImuCalibService*
                            │                     prior R + quiet-lock → sensors/imu_yaw
                            │                                  ▼
                            │                           Localization*
                            └◄── ZMQ OUT :5556 → BagLogger
* gated by assets/config.json (lane_keep / localization); steer torque only if lane_keep=true
```

Подробнее: [`app/src/main/cpp/src/python/README.md`](app/src/main/cpp/src/python/README.md).
