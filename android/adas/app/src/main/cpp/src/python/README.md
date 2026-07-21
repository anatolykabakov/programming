# pyadas — C++ ADAS algorithms for Python (sim / bag visualizer)

Uses **`AdasApp` in Simulated mode** (same class as Android realtime app).

## Services → topics (ZMQMessage → ZmqBridge → Android BagLogger)

Single ZMQ pair (native binds):

| Direction | Endpoint | Role |
|-----------|----------|------|
| IN | `tcp://127.0.0.1:5555` | external PUB → native SUB |
| OUT | `tcp://127.0.0.1:5556` | native PUB → external SUB |

Multipart: `[topic][ZMQMessage proto]`.

| Service | Sub (internal) | Pub (via OUT) |
|---------|----------------|---------------|
| `TopicConvertService` | lanes/state/imu/gps/`lane_uv`/`model/camera_odometry` | typed path/chassis/imu/gps/uv/odom |
| `ImuCalibService` | `vehicle/chassis`, `sensors/imu_raw` | `sensors/imu_yaw` |
| `LaneKeepService` | `vehicle/chassis`, `vision/path` | `control/lane_keep`, `controls/steer` |
| `LocalizationService` | chassis, gps, `sensors/imu_yaw` | `localization/pose` |
| `CameraCalibService` | `model/camera_odometry` + chassis (+ optional `lane_uv`) | `calibration/camera` |

**Live calib (Android/sim/bag — flowpilot):** only model pose → C++ `PoseCalibrator`
(`calibrationd`: straight+fast → pitch/yaw from `atan2(trans)`). Java/Python publish
`model/camera_odometry`; no Hough on device.

**VP calib (host optional):** Hough/GT→UV → same `CameraCalibService.update_from_uv`.

Android path: `VisionPipeline` → `vision/lanes` + `model/camera_odometry` → native.

IMU path: `sensors/imu` → `imu_raw` → `ImuCalibService` (bias+R while &lt;0.5 km/h; apply while moving) → `imu_yaw` → `LocalizationService`.

## PyAdasApp (sim / offline)

```python
from pyadas import PyAdasApp

app = PyAdasApp(wheelbase=2.636, pitch0_deg=-6.0, camera_height=1.40)
app.set_camera_intrinsics(930, 930, 640, 360)
app.publish_chassis_xy(t_us, speed_mps=10, steer_rad=0.01)
app.publish_lanes_xy(t_us, [(1,0), (10,0.1), (30,0.2)])
app.step(t_us)
msgs = app.pop_messages()  # lane_keep / localization_pose / camera_calib
```

Aliases: `PyAdasPipeline` → `PyAdasApp`, `AdasPipeline` → `AdasApp`.

## Build (desktop)

Из корня `programming/android/adas`:

```bash
./app/src/main/cpp/scripts/build_cpp.sh -t linux --python
# → app/src/main/scripts/pyadas/core*.so
```

## Run bag / sim (из корня проекта)

```bash
./run_bag_vis.sh /path/to/bag
./run_sim.sh --controller pure_pursuit --show --lanes supercombo
```
