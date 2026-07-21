#!/usr/bin/env python3
"""C++ backends via ``pyadas`` — source of truth for algorithms.

Build::

    cd app/src/main/cpp
    cmake -B build-linux -DBUILD_FOR_ANDROID=OFF -DBUILD_PYTHON_BINDINGS=ON ...
    cmake --build build-linux -j
"""

from __future__ import annotations

from typing import Any, List, Optional, Tuple

import numpy as np

_cpp = None
try:
    from pyadas import core as _cpp  # type: ignore
except Exception:
    _cpp = None


def cpp_available() -> bool:
    return _cpp is not None


def require_cpp():
    if _cpp is None:
        raise RuntimeError(
            "pyadas C++ module required (BUILD_PYTHON_BINDINGS=ON). "
            "Python algorithm duplicates were removed — C++ is the source of truth."
        )
    return _cpp


class NativeLaneKeep:
    def __init__(self, **kwargs: Any):
        cpp = require_cpp()
        self._svc = cpp.LaneKeepService(
            wheelbase=float(kwargs.get("wheelbase", 2.636)),
            desired_speed=float(kwargs.get("desired_speed", 12.0)),
            max_steer_deg=float(kwargs.get("max_steer_deg", 40.0)),
            pp_k_dd=float(kwargs.get("pp_k_dd", 0.4)),
            pp_ld_min=float(kwargs.get("pp_ld_min", 3.0)),
            pp_ld_max=float(kwargs.get("pp_ld_max", 20.0)),
            pp_shift=float(kwargs.get("pp_shift", 1.4)),
        )

    def step(self, speed_mps: float, polyline: np.ndarray) -> Any:
        poly = np.asarray(polyline, dtype=np.float64)
        pairs: List[Tuple[float, float]] = [(float(x), float(y)) for x, y in poly]
        return self._svc.step(float(speed_mps), pairs)


class NativeLocalizer:
    def __init__(self, **kwargs: Any):
        cpp = require_cpp()
        self._svc = cpp.LocalizationService(
            wheelbase=float(kwargs.get("wheelbase", 2.636)),
            gps_noise_pos=float(kwargs.get("gps_noise_pos", 0.5)),
            gps_update_interval=float(kwargs.get("gps_update_interval", 0.2)),
        )

    def reset(
        self,
        x: float = 0.0,
        y: float = 0.0,
        yaw: float = 0.0,
        v: float = 0.0,
        yaw_rate: float = 0.0,
    ) -> None:
        self._svc.reset_pose(x, y, yaw, v, yaw_rate)

    def step(
        self,
        dt: float,
        speed_mps: float,
        steer_rad: float,
        yaw_rate: Optional[float] = None,
        gps_xy: Optional[Tuple[float, float]] = None,
        ref_xy: Optional[Tuple[float, float]] = None,
    ) -> Tuple[float, float, float]:
        gx = gy = rx = ry = None
        if gps_xy is not None:
            gx, gy = float(gps_xy[0]), float(gps_xy[1])
        if ref_xy is not None:
            rx, ry = float(ref_xy[0]), float(ref_xy[1])
        return tuple(
            self._svc.step(
                float(dt),
                float(speed_mps),
                float(steer_rad),
                yaw_rate if yaw_rate is None else float(yaw_rate),
                gx,
                gy,
                rx,
                ry,
            )
        )
