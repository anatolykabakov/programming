# Host / Android / Sim parity

Цель: **одинаковый** `steer_rad` (device frame) на телефоне, в bag tools и в MetaDrive
при одинаковых входах (lanes/plan + speed + PP params).

## Канон (как Android hot path)

1. Vision output: **device** X fwd, **Y right+**, Z up.
2. Path: C++ `laneLinesToPath` / Python `core.path_fusion` (PLAN + near L/R blend).
3. PP: C++ `LaneKeepService` via Middleware / `pyadas.AdasApp.publish_lanes`.
4. Defaults: `max_steer_deg=8`, `pp_k_dd=0.4`, `ld∈[3,20]`, `shift=1.40`.

Константы: `scripts/core/frames.py` (`PP_Y_SIGN=1`, `DRAW_Y_SIGN=-1`, `METADRIVE_STEER_FROM_DEVICE=-1`).

## Что выровнено

| Компонент | Было | Стало |
|-----------|------|-------|
| Host ONNX preprocess | `cv2.resize` only | `ModelCalibWarp` + K/RPY |
| Host RNN | zeros every frame | recurrent last-512 like Android |
| Bag PP Y | mixed `±1` | always device (`PP_Y_SIGN=1`) |
| Path | plan-only / naive mid | `path_from_bag_lanes` / `path_from_supercombo` |
| `max_steer_deg` | 40 on host | 8 (C++ default) |
| MetaDrive actuator | raw device δ | `× METADRIVE_STEER_FROM_DEVICE` (logged `steer_rad` still device) |

## Как воспроизвести телефонный PP с bag

```bash
# 1) Bag lanes (рекомендуется — тот же model output, что на phone)
python3 app/src/main/scripts/bag_lane_keep_offline.py /path/to/session

# 2) Interactive: Lanes→Bag + PP on
./run_bag_vis.sh /path/to/session
```

Runtime ONNX на хосте совпадает с телефоном только если совпали **K, RPY, кадр**
(bag JPEG часто 640×360 — подставляйте bag intrinsics; на телефоне warp был с full-res K).

## Оверлей vs control

- Control / CSV / `LaneKeepOutput.steer_rad`: **device Y-right**.
- `project_iso_xyz` / draw: `y_sign=DRAW_Y_SIGN` (−1).
- MetaDrive `env.step` steer: device δ × `METADRIVE_STEER_FROM_DEVICE`.
