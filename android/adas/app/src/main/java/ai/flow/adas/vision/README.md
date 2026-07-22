# Vision: openpilot supercombo ONNX (simple wrapper)

Based on openpilot ~v0.8.x `driving.cc` output layout (same ONNX as
`openpilot-supercombo-model/supercombo.onnx`, out=6409).

## Classes

| Class | Role |
|-------|------|
| `vision/SupercomboOnnxRunner` | Preprocess + ONNX Runtime infer + parse **plan / lanes / edges** |
| `vision/LaneLines` | 4 lanes + 2 edges + best PLAN path at ego xyz |
| `vision/LaneOverlayView` | Yellow lanes / red edges / **green PLAN** on camera |
| `vision/VisionPipeline` | Background thread → overlay + `Logger` (`vision/lanes` protobuf → ZMQ IN) |

## Output parse (important)

Do **not** use the GitHub demo’s `ll_t` / `ll_t2` grouping. Official layout:

| Slice | Meaning |
|-------|---------|
| `[0:4955)` | PLAN — 5 MHP trajectories (33×15 mean + std + logit) |
| `[4955:5483)` | LANES — 4×33×**(y,z)** means, then stds |
| `[5483:5491)` | lane probs — `sigmoid(prob[i*2+1])` |
| `[5491:5755)` | ROAD EDGES — 2×33×(y,z) means, then stds |

Green overlay is the **best PLAN hypothesis**, not the midpoint of near lanes.

## Build / install APK

From the ADAS project root (`programming/android/adas`):

```bash
./build_project.sh              # debug
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Model

Bundled at **`assets/supercombo.onnx`** (no `models/` dir). Asset name comes from
`assets/config.json` → `supercombo_asset`.

Load order: `/sdcard/adas_models/<name>` → app filesDir cache → assets root.

```bash
adb shell mkdir -p /sdcard/adas_models
adb push openpilot-supercombo-model/supercombo.onnx /sdcard/adas_models/supercombo.onnx
```

Node feature flags + camera extrinsic priors: `assets/config.json` (see `AdasConfig`).

## Notes

- Needs 2 frames before first result (temporal stack).
- Traffic convention input defaults to RHT `[1, 0]`.
- Bag proto `vision/lanes` also logs `plan_x/y/z` + `plan_hyp`.
- Lateral axis: this ONNX raw Y is **right-positive**; parse negates to openpilot
  **Y-left**, overlay uses `u = cx - fx·Y/X`.
