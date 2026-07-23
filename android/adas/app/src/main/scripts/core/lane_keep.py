#!/usr/bin/env python3
"""Lane keeping for bag/sim visualizers via Simulated ``pyadas.AdasApp``.

``pure_pursuit`` / ``straight`` only — publish chassis/lanes, ``step``, read state.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Dict, Optional

import numpy as np
from pyadas import core as pyadas

from .pure_pursuit import PurePursuitResult, pp_result_from_output


@dataclass(frozen=True)
class LaneKeepDefaults:
    speed_mps: float = 12.0
    max_steer_deg: float = 8.0
    wheelbase: float = 2.636
    pp_k_dd: float = 0.4
    pp_ld_min: float = 3.0
    pp_ld_max: float = 20.0
    pp_shift: float = 1.40
    speed_kp: float = 0.08


DEFAULTS = LaneKeepDefaults()


@dataclass
class LaneKeepResult:
    mode: str
    steer_rad: float
    steer_norm: float
    throttle: float
    brake: float
    polyline: Optional[np.ndarray]
    e_y: float = 0.0
    e_psi: float = 0.0
    curvature: float = 0.0
    pure_pursuit: Optional[PurePursuitResult] = None
    status: str = "ok"


def build_centerline_polyline(
    left_road: np.ndarray,
    right_road: np.ndarray,
    x_min: float = 1.0,
    x_max: float = 40.0,
    n: int = 48,
) -> Optional[np.ndarray]:
    left = np.asarray(left_road, dtype=np.float64)
    right = np.asarray(right_road, dtype=np.float64)
    if left.ndim != 2 or right.ndim != 2 or left.shape[0] < 2 or right.shape[0] < 2:
        return None

    x_lo = max(x_min, float(max(left[:, 0].min(), right[:, 0].min())))
    x_hi = min(x_max, float(min(left[:, 0].max(), right[:, 0].max())))
    if x_hi <= x_lo + 1.0:
        return None

    xs = np.linspace(x_lo, x_hi, n)
    y_left = np.interp(xs, left[:, 0], left[:, 1])
    y_right = np.interp(xs, right[:, 0], right[:, 1])
    return np.stack([xs, 0.5 * (y_left + y_right)], axis=1)


def format_lane_keep_status(lk: LaneKeepResult) -> str:
    if lk.mode == "pure_pursuit" and lk.pure_pursuit is not None:
        pp = lk.pure_pursuit
        tgt = (
            f"({pp.target_ego[0]:.1f},{pp.target_ego[1]:.1f})"
            if pp.target_ego is not None
            else "none"
        )
        return (
            f"Ld={pp.lookahead_m:.1f}m δ={np.rad2deg(lk.steer_rad):.1f}° "
            f"κ={lk.curvature:.4f}/m tgt={tgt}"
        )
    return f"{lk.mode} δ={np.rad2deg(lk.steer_rad):+.1f}°  {lk.status}"


class LaneKeepController:
    """Sim/viz glue: publish inputs → AdasApp.step → pop_messages(LaneKeepOutput)."""

    MODES = ("straight", "pure_pursuit")

    def __init__(
        self,
        mode: str = "pure_pursuit",
        desired_speed: float = DEFAULTS.speed_mps,
        max_steer_deg: float = DEFAULTS.max_steer_deg,
        wheelbase: float = DEFAULTS.wheelbase,
        pp_k_dd: float = DEFAULTS.pp_k_dd,
        pp_ld_min: float = DEFAULTS.pp_ld_min,
        pp_ld_max: float = DEFAULTS.pp_ld_max,
        pp_shift: float = DEFAULTS.pp_shift,
        speed_kp: float = DEFAULTS.speed_kp,
        app: Any = None,
    ):
        if mode not in self.MODES:
            raise ValueError(f"Unknown mode {mode!r}, expected one of {self.MODES}")
        self.mode = mode
        self.desired_speed = float(desired_speed)
        self.max_steer_rad = float(np.deg2rad(max_steer_deg))
        self.wheelbase = float(wheelbase)
        self.pp_shift = float(pp_shift)
        self.speed_kp = float(speed_kp)
        self._t_us = 0
        self._owns_app = app is None
        self._app = app or pyadas.AdasApp(wheelbase=float(wheelbase))
        self.apply_pp_params(
            pp_k_dd=pp_k_dd,
            pp_ld_min=pp_ld_min,
            pp_ld_max=pp_ld_max,
            pp_shift=pp_shift,
            max_steer_deg=max_steer_deg,
        )
        self.last_result: Optional[LaneKeepResult] = None

    def apply_pp_params(
        self,
        *,
        pp_k_dd: float,
        pp_ld_min: float,
        pp_ld_max: float,
        pp_shift: float,
        max_steer_deg: float,
    ) -> None:
        self.pp_shift = float(pp_shift)
        self.max_steer_rad = float(np.deg2rad(max_steer_deg))
        self._app.set_lane_keep_pp(
            float(pp_k_dd), float(pp_ld_min), float(pp_ld_max), float(pp_shift)
        )
        self._app.set_lane_keep_max_steer_deg(float(max_steer_deg))

    @property
    def waypoint_shift(self) -> float:
        return self.pp_shift

    @property
    def app(self) -> Any:
        return self._app

    def _speed_action(self, speed_mps: float) -> tuple[float, float]:
        err = self.desired_speed - max(0.0, float(speed_mps))
        if err > 0.5:
            return min(0.85, self.speed_kp * err), 0.0
        if err < -1.0:
            return 0.0, min(0.5, 0.05 * (-err))
        return 0.0, 0.0

    def compute_from_polyline(
        self,
        speed_mps: float,
        polyline: Optional[np.ndarray],
    ) -> LaneKeepResult:
        throttle, brake = self._speed_action(speed_mps)

        if self.mode == "straight" or polyline is None:
            result = LaneKeepResult(
                mode=self.mode,
                steer_rad=0.0,
                steer_norm=0.0,
                throttle=throttle,
                brake=brake,
                polyline=None if polyline is None else np.asarray(polyline, dtype=np.float64),
                status="straight" if self.mode == "straight" else "no_polyline",
            )
            self.last_result = result
            return result

        poly = np.asarray(polyline, dtype=np.float64)
        if poly.ndim != 2 or poly.shape[0] < 2:
            result = LaneKeepResult(
                mode=self.mode,
                steer_rad=0.0,
                steer_norm=0.0,
                throttle=throttle,
                brake=brake,
                polyline=None,
                status="no_polyline",
            )
            self.last_result = result
            return result

        pairs = [(float(x), float(y)) for x, y in poly]
        self._t_us += 50_000
        self._app.publish_chassis(self._t_us, float(speed_mps), 0.0)
        self._app.publish_lanes(self._t_us, pairs)
        self._app.step(self._t_us)
        out = None
        for msg in self._app.pop_messages():
            if isinstance(msg, pyadas.LaneKeepOutput):
                out = msg
        if out is None:
            result = LaneKeepResult(
                mode=self.mode,
                steer_rad=0.0,
                steer_norm=0.0,
                throttle=throttle,
                brake=brake,
                polyline=poly,
                status="no_output",
            )
            self.last_result = result
            return result
        # Use geometric PP steer_rad. out.steer_norm may be LatControlPid torque
        # command (real-car path) and is wrong for MetaDrive / host actuators.
        steer_rad = float(out.steer_rad)
        steer_norm = float(
            np.clip(steer_rad / self.max_steer_rad, -1.0, 1.0) if self.max_steer_rad > 1e-6 else 0.0
        )
        result = LaneKeepResult(
            mode=self.mode,
            steer_rad=steer_rad,
            steer_norm=steer_norm,
            throttle=throttle,
            brake=brake,
            polyline=poly,
            curvature=float(out.curvature),
            pure_pursuit=pp_result_from_output(
                out,
                poly,
                speed_mps,
                waypoint_shift=self.pp_shift,
                wheel_base=self.wheelbase,
            ),
            status=str(out.status),
        )
        self.last_result = result
        return result

    def compute(self, speed_mps: float, lanes: Optional[Dict[str, Any]]) -> LaneKeepResult:
        """``left_road`` / ``right_road`` must already be **device Y-right**."""
        if self.mode == "straight" or lanes is None:
            return self.compute_from_polyline(speed_mps, None)
        poly = build_centerline_polyline(lanes.get("left_road"), lanes.get("right_road"))
        return self.compute_from_polyline(speed_mps, poly)

    def get_control(self, speed_mps: float, lanes: Optional[Dict[str, Any]] = None) -> list[float]:
        """Returns [steer_norm, throttle, brake] in **device** frame (right+)."""
        result = self.compute(speed_mps, lanes)
        return [result.steer_norm, result.throttle, result.brake]
