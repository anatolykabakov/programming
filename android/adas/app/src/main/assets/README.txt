ADAS assets (app/src/main/assets)

  config.json       — node flags, vehicle, camera priors, zmq.endpoint_in/out
  supercombo.onnx   — openpilot supercombo (bundled at assets root)
  vw_mqb_2010.dbc   — Golf 7 / MQB CAN DB

Model load order (SupercomboOnnxRunner):
  1) /sdcard/adas_models/supercombo.onnx   (optional override)
  2) app filesDir cache
  3) assets/supercombo.onnx

Optional push:
  adb shell mkdir -p /sdcard/adas_models
  adb push openpilot-supercombo-model/supercombo.onnx /sdcard/adas_models/supercombo.onnx
