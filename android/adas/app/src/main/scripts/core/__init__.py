"""Shared ADAS helpers for bag/sim visualizers.

Algorithms (PP, EKF, VP calib, lane-keep) live in C++ (``pyadas``).
Python modules are thin wrappers + IMU/Hough/viz utilities:

  - ``core.lane_keep.LaneKeepController`` → C++ ``LaneKeepService``
  - ``core.online_localizer.OnlineLocalizer`` → C++ ``OnlineLocalizer``
  - ``core.vanishing_point_calib.VanishingPointCalibrator`` → C++ ``CameraCalibService``
  - ``core.supercombo_compare.SupercomboBev`` — ONNX compare (Python)
"""
