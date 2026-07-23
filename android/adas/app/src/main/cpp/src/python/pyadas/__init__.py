"""pyadas — C++ ADAS algorithms via pybind11.

Build (desktop)::

  cd app/src/main/cpp
  cmake -B build-linux -DBUILD_FOR_ANDROID=OFF -DBUILD_PYTHON_BINDINGS=ON ...
  cmake --build build-linux --target core

Module lands in ``app/src/main/scripts/pyadas/`` (copied on build).

Host API: ``AdasApp`` — publish inputs → ``step`` → read ``*_state``.
"""

from __future__ import annotations

try:
    from . import core as core
except ModuleNotFoundError:
    core = None
else:
    from .core import (  # noqa: F401
        AdasApp,
        CameraCalibrationState,
        ChassisSample,
        GpsSample,
        ImuSample,
        LaneKeepOutput,
        LanePathMsg,
        LocalizationPose,
        PyAdasApp,
        Vec2,
    )
