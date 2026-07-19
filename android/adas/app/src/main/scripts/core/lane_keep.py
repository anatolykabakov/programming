#!/usr/bin/env python3
"""Shared lane keeping for interactive visualizer and MetaDrive sim."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Dict, Optional

import numpy as np

from .pure_pursuit import PurePursuit, PurePursuitResult


@dataclass(frozen=True)
class LaneKeepDefaults:
    """Defaults shared by visualizer and sim."""

    speed_mps: float = 12.0
    max_steer_deg: float = 40.0
    wheelbase: float = 2.636
    pp_k_dd: float = 0.4
    pp_ld_min: float = 3.0
    pp_ld_max: float = 20.0
    pp_shift: float = 1.40
    pd_la: float = 15.0
    pd_ky: float = 0.1
    pd_kpsi: float = 0.5
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
    """Lane center in road frame: X forward, Y left."""
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


def lateral_pd_steer_rad(
    polyline: np.ndarray,
    speed_mps: float,
    lookahead_m: float = 15.0,
    ky: float = 0.1,
    kpsi: float = 0.5,
    wheelbase: float = 2.636,
) -> tuple[float, float, float, float]:
    """Same law as bag_lane_keep_offline.py → road-wheel steer [rad]."""
    xs = polyline[:, 0]
    ys = polyline[:, 1]
    la = float(lookahead_m)
    e_y = float(np.interp(la, xs, ys))
    y_near = float(np.interp(max(5.0, la * 0.5), xs, ys))
    y_far = float(np.interp(la * 1.5, xs, ys))
    e_psi = float(np.arctan2(y_far - y_near, la))
    v = max(float(speed_mps), 1.0)
    curv = -(ky * e_y + kpsi * e_psi) / (v * v)
    steer_rad = float(curv * wheelbase)
    return steer_rad, e_y, e_psi, curv


def format_lane_keep_status(lk: LaneKeepResult) -> str:
    """One-line HUD / UI status for lane keeping."""
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
    if lk.mode == "lateral_pd":
        return (
            f"PD δ={np.rad2deg(lk.steer_rad):.1f}° "
            f"e_y={lk.e_y:.2f} e_psi={np.rad2deg(lk.e_psi):+.1f}° "
            f"κ={lk.curvature:.4f}"
        )
    return f"{lk.mode} δ={np.rad2deg(lk.steer_rad):+.1f}°  {lk.status}"


class LaneKeepController:
    """Lane keeping from centerline polyline or MetaDrive lane boundaries."""

    MODES = ("straight", "pure_pursuit", "lateral_pd")

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
        pd_la: float = DEFAULTS.pd_la,
        pd_ky: float = DEFAULTS.pd_ky,
        pd_kpsi: float = DEFAULTS.pd_kpsi,
        speed_kp: float = DEFAULTS.speed_kp,
    ):
        if mode not in self.MODES:
            raise ValueError(f"Unknown mode {mode!r}, expected one of {self.MODES}")
        self.mode = mode
        self.desired_speed = float(desired_speed)
        self.max_steer_rad = float(np.deg2rad(max_steer_deg))
        self.wheelbase = float(wheelbase)
        self.pp = PurePursuit(
            K_dd=pp_k_dd,
            wheel_base=wheelbase,
            waypoint_shift=pp_shift,
            ld_min=pp_ld_min,
            ld_max=pp_ld_max,
        )
        self.pd_la = float(pd_la)
        self.pd_ky = float(pd_ky)
        self.pd_kpsi = float(pd_kpsi)
        self.speed_kp = float(speed_kp)
        self.last_result: Optional[LaneKeepResult] = None

    @property
    def waypoint_shift(self) -> float:
        return float(self.pp.waypoint_shift)

    def _speed_action(self, speed_mps: float) -> tuple[float, float]:
        err = self.desired_speed - max(0.0, float(speed_mps))
        if err > 0.5:
            return min(0.85, self.speed_kp * err), 0.0
        if err < -1.0:
            return 0.0, min(0.5, 0.05 * (-err))
        return 0.0, 0.0

    def _normalize_steer(self, steer_rad: float) -> float:
        if self.max_steer_rad <= 1e-6:
            return 0.0
        return float(np.clip(steer_rad / self.max_steer_rad, -1.0, 1.0))

    def _compute_steer_from_polyline(
        self,
        poly: np.ndarray,
        speed_mps: float,
    ) -> tuple[float, float, float, float, Optional[PurePursuitResult]]:
        steer_rad = 0.0
        e_y = e_psi = curv = 0.0
        pp_result: Optional[PurePursuitResult] = None

        if self.mode == "pure_pursuit":
            pp_result = self.pp.compute(poly, speed_mps)
            steer_rad = float(pp_result.steer_rad)
            curv = float(pp_result.curvature)
        elif self.mode == "lateral_pd":
            steer_rad, e_y, e_psi, curv = lateral_pd_steer_rad(
                poly,
                speed_mps,
                lookahead_m=self.pd_la,
                ky=self.pd_ky,
                kpsi=self.pd_kpsi,
                wheelbase=self.wheelbase,
            )
        return steer_rad, e_y, e_psi, curv, pp_result

    def compute_from_polyline(
        self,
        speed_mps: float,
        polyline: Optional[np.ndarray],
    ) -> LaneKeepResult:
        """Run controller on an existing ego-frame polyline (visualizer plan / sim centerline)."""
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

        steer_rad, e_y, e_psi, curv, pp_result = self._compute_steer_from_polyline(poly, speed_mps)
        result = LaneKeepResult(
            mode=self.mode,
            steer_rad=steer_rad,
            steer_norm=self._normalize_steer(steer_rad),
            throttle=throttle,
            brake=brake,
            polyline=poly,
            e_y=e_y,
            e_psi=e_psi,
            curvature=curv,
            pure_pursuit=pp_result,
            status="ok",
        )
        self.last_result = result
        return result

    def compute(self, speed_mps: float, lanes: Optional[Dict[str, Any]]) -> LaneKeepResult:
        """MetaDrive-style entry: build centerline from lane boundaries, then control."""
        if self.mode == "straight" or lanes is None:
            return self.compute_from_polyline(speed_mps, None)

        poly = build_centerline_polyline(lanes.get("left_road"), lanes.get("right_road"))
        return self.compute_from_polyline(speed_mps, poly)

    def get_control(self, speed_mps: float, lanes: Optional[Dict[str, Any]] = None) -> list[float]:
        result = self.compute(speed_mps, lanes)
        return [result.steer_norm, result.throttle, result.brake]
