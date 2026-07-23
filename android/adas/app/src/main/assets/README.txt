ADAS assets (app/src/main/assets)

  config.json       — node feature flags + vehicle + camera calib priors
  supercombo.onnx   — vision supercombo model (bundled at assets root)
  vw_mqb_2010.dbc   — Golf 7 / MQB CAN DB

Model load order (SupercomboOnnxRunner):
  1) /sdcard/adas_models/supercombo.onnx   (optional override)
  2) app filesDir cache
  3) assets/supercombo.onnx

Optional push:
  adb shell mkdir -p /sdcard/adas_models
  adb push /path/to/supercombo.onnx /sdcard/adas_models/supercombo.onnx
