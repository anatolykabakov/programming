#!/usr/bin/env python3
"""Vehicle EKF — thin wrapper over C++ ``pyadas.VehicleEKF``."""

from __future__ import annotations

from typing import Optional, Tuple

from .native import require_cpp


class VehicleEKF:
    """API-compatible shim around C++ VehicleEKF."""

    def __init__(
        self,
        initial_x: float = 0.0,
        initial_y: float = 0.0,
        initial_yaw: float = 0.0,
        initial_v: float = 0.0,
        initial_yaw_rate: float = 0.0,
        wheelbase: float = 2.636,
        gps_noise_pos: float = 5.0,
        imu_noise_yaw_rate: float = 0.02,
        **_ignored,
    ):
        cpp = require_cpp()
        self._ekf = cpp.VehicleEKF(
            float(wheelbase), float(gps_noise_pos), float(imu_noise_yaw_rate)
        )
        self._ekf.reset(
            float(initial_x),
            float(initial_y),
            float(initial_yaw),
            float(initial_v),
            float(initial_yaw_rate),
        )
        self.wheelbase = float(wheelbase)

    def reset(
        self,
        x: float = 0.0,
        y: float = 0.0,
        yaw: float = 0.0,
        v: float = 0.0,
        yaw_rate: float = 0.0,
        **_ignored,
    ) -> None:
        self._ekf.reset(float(x), float(y), float(yaw), float(v), float(yaw_rate))

    def predict(self, v_measured: float, steering_angle: float, dt: float) -> None:
        self._ekf.predict(float(v_measured), float(steering_angle), float(dt))

    def update_gps(self, gps_x: float, gps_y: float, max_innovation: float = 50.0) -> bool:
        return bool(self._ekf.update_gps(float(gps_x), float(gps_y), float(max_innovation)))

    def update_imu(self, yaw_rate_imu: float) -> None:
        self._ekf.update_imu(float(yaw_rate_imu))

    def get_position(self) -> Tuple[float, float]:
        return float(self._ekf.x), float(self._ekf.y)

    def get_yaw(self) -> float:
        return float(self._ekf.yaw)

    def get_velocity(self) -> float:
        return float(self._ekf.v)

    def get_yaw_rate(self) -> float:
        return float(self._ekf.yaw_rate)

    @property
    def x(self) -> float:
        return float(self._ekf.x)

    @property
    def y(self) -> float:
        return float(self._ekf.y)

    @property
    def yaw(self) -> float:
        return float(self._ekf.yaw)

    @property
    def v(self) -> float:
        return float(self._ekf.v)

    @property
    def yaw_rate(self) -> float:
        return float(self._ekf.yaw_rate)

    @property
    def prediction_count(self) -> int:
        return int(self._ekf.prediction_count)

    @property
    def gps_update_count(self) -> int:
        return int(self._ekf.gps_update_count)

    @property
    def imu_update_count(self) -> int:
        return int(self._ekf.imu_update_count)

    def print_statistics(self) -> None:
        """Bag / sim HUD — counters from C++ VehicleEKF."""
        print(
            "EKF stats: "
            f"predict={self.prediction_count} "
            f"gps={self.gps_update_count} "
            f"imu={self.imu_update_count} "
            f"pose=({self.x:.2f},{self.y:.2f}) "
            f"yaw={self.yaw:.3f} rad v={self.v:.2f}"
        )
