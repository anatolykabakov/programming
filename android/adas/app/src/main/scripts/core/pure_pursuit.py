#!/usr/bin/env python3
"""Pure pursuit — C++ ``pyadas.PurePursuit`` + Python viz helpers.

Algorithm lives in C++ (``utils/pure_pursuit``). This module wraps the result
type and drawing utilities for bag/sim visualizers.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional, Tuple

import cv2
import numpy as np

from .lane_projection import CameraGeometry, project_iso_xyz


@dataclass
class PurePursuitResult:
    lookahead_m: float
    target_ra: Optional[np.ndarray]  # rear-axle frame (x>0 forward, y left)
    target_ego: Optional[np.ndarray]  # camera/ego frame (same axes)
    alpha_rad: float
    steer_rad: float
    speed_mps: float
    polyline_ego: np.ndarray  # Nx2 ego (x fwd, y left)
    wheel_base: float = 2.636

    @property
    def curvature(self) -> float:
        """Bicycle-model curvature κ = tan(δ) / L  [1/m]."""
        return float(np.tan(self.steer_rad) / max(self.wheel_base, 1e-6))

    @property
    def radius_m(self) -> float:
        k = self.curvature
        if abs(k) < 1e-6:
            return float("inf")
        return float(1.0 / k)


class PurePursuit:
    """AAD PurePursuit: Ld = clip(K_dd * v, ld_min, ld_max)."""

    def __init__(
        self,
        K_dd: float = 0.4,
        wheel_base: float = 2.636,
        waypoint_shift: float = 1.4,
        ld_min: float = 3.0,
        ld_max: float = 20.0,
    ):
        self.K_dd = float(K_dd)
        self.wheel_base = float(wheel_base)
        self.waypoint_shift = float(waypoint_shift)
        self.ld_min = float(ld_min)
        self.ld_max = float(ld_max)

    def compute(self, polyline_ego: np.ndarray, speed_mps: float) -> PurePursuitResult:
        """``polyline_ego``: Nx2 in camera/ego frame (X forward, Y left). C++ only."""
        from .native import require_cpp

        cpp = require_cpp()
        poly = np.asarray(polyline_ego, dtype=np.float64)
        pairs = [(float(x), float(y)) for x, y in poly]
        backend = cpp.PurePursuit(
            self.K_dd, self.wheel_base, self.waypoint_shift, self.ld_min, self.ld_max
        )
        r = backend.compute(pairs, float(speed_mps))
        target_ra = None
        target_ego = None
        if r.has_target:
            target_ego = np.array([r.target_x, r.target_y], dtype=np.float64)
            target_ra = np.array([r.target_x + self.waypoint_shift, r.target_y], dtype=np.float64)
        return PurePursuitResult(
            lookahead_m=float(r.lookahead_m),
            target_ra=target_ra,
            target_ego=target_ego,
            alpha_rad=float(r.alpha_rad),
            steer_rad=float(r.steer_rad),
            speed_mps=float(r.speed_mps),
            polyline_ego=poly,
            wheel_base=self.wheel_base,
        )


def plan_to_polyline_ego(
    plan_x: np.ndarray,
    plan_y: np.ndarray,
    y_sign: float = -1.0,
    x_min: float = 0.5,
    recenter: bool = False,
    recenter_at_x: float = 5.0,
) -> np.ndarray:
    """Supercombo plan (Y-right raw) → Nx2 ego polyline (X fwd, Y left).

    If ``recenter``, subtract lateral offset at ``recenter_at_x`` so a constant
    plan bias (common on OOD / sim cameras) does not produce steady steering
    on a straight road. Curvature of the plan is preserved.
    """
    xs = np.asarray(plan_x, dtype=np.float64)
    ys = y_sign * np.asarray(plan_y, dtype=np.float64)
    ok = np.isfinite(xs) & np.isfinite(ys) & (xs >= x_min)
    poly = np.stack([xs[ok], ys[ok]], axis=1)
    if not recenter or poly.shape[0] < 2:
        return poly
    order = np.argsort(poly[:, 0])
    poly = poly[order]
    x0 = float(np.clip(recenter_at_x, poly[0, 0], poly[-1, 0]))
    y_ref = float(np.interp(x0, poly[:, 0], poly[:, 1]))
    poly = poly.copy()
    poly[:, 1] -= y_ref
    return poly


def bicycle_arc_points(
    curvature: float,
    length_m: float = 40.0,
    n: int = 40,
) -> np.ndarray:
    """Instantaneous bicycle path in rear-axle frame (X fwd, Y left), Nx2."""
    s = np.linspace(0.0, length_m, n)
    k = float(curvature)
    if abs(k) < 1e-6:
        return np.stack([s, np.zeros_like(s)], axis=1)
    # x = (1/κ) sin(κs),  y = (1/κ)(1 − cos(κs))  — left positive for κ>0
    x = np.sin(k * s) / k
    y = (1.0 - np.cos(k * s)) / k
    return np.stack([x, y], axis=1)


def draw_steering_wheel(
    bgr: np.ndarray,
    steer_rad: float,
    center: Optional[Tuple[int, int]] = None,
    radius: int = 48,
    steer_ratio: float = 15.7,
) -> None:
    """HUD steering wheel; rim rotates by road-wheel·steer_ratio (visual)."""
    h, w = bgr.shape[:2]
    if center is None:
        center = (w - radius - 16, h - radius - 40)
    cx, cy = center
    # Visual angle: steering-wheel degrees (clamped so graphic stays readable)
    wheel_deg = float(np.rad2deg(steer_rad) * steer_ratio)
    wheel_deg = float(np.clip(wheel_deg, -120.0, 120.0))
    ang = np.deg2rad(wheel_deg)

    # Soft backdrop
    overlay = bgr.copy()
    cv2.circle(overlay, (cx, cy), radius + 6, (30, 30, 30), -1, cv2.LINE_AA)
    cv2.addWeighted(overlay, 0.55, bgr, 0.45, 0, bgr)

    # Rim
    cv2.circle(bgr, (cx, cy), radius, (220, 220, 220), 3, cv2.LINE_AA)
    cv2.circle(bgr, (cx, cy), int(radius * 0.35), (180, 180, 180), 2, cv2.LINE_AA)

    def _rot(px: float, py: float) -> Tuple[int, int]:
        c, s = np.cos(ang), np.sin(ang)
        # image y down: positive steer (left) → counter-clockwise on screen
        xr = c * px + s * py
        yr = -s * px + c * py
        return int(round(cx + xr)), int(round(cy + yr))

    # Three spokes + top marker
    for a0 in (90.0, 210.0, 330.0):
        a = np.deg2rad(a0)
        p0 = _rot(0.0, 0.0)
        p1 = _rot(radius * 0.92 * np.cos(a), -radius * 0.92 * np.sin(a))
        cv2.line(bgr, p0, p1, (200, 200, 200), 2, cv2.LINE_AA)

    # Hub + top tick (shows rotation)
    top = _rot(0.0, -radius * 0.85)
    cv2.circle(bgr, (cx, cy), 5, (0, 200, 255), -1, cv2.LINE_AA)
    cv2.circle(bgr, top, 5, (0, 0, 255), -1, cv2.LINE_AA)

    label = f"{np.rad2deg(steer_rad):+.1f}° road"
    cv2.putText(
        bgr,
        label,
        (cx - radius, cy + radius + 18),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.4,
        (0, 200, 255),
        1,
        cv2.LINE_AA,
    )
    cv2.putText(
        bgr,
        f"SW {wheel_deg:+.0f}°",
        (cx - radius, cy + radius + 34),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.4,
        (200, 200, 200),
        1,
        cv2.LINE_AA,
    )


def draw_pure_pursuit(
    bgr: np.ndarray,
    pp: PurePursuitResult,
    geom: CameraGeometry,
    waypoint_shift: float,
    y_sign_for_project: float = 1.0,
    steer_ratio: float = 15.7,
) -> None:
    """Draw lookahead, target, curvature arc, and steering-wheel HUD."""
    h, w = bgr.shape[:2]
    ld = pp.lookahead_m
    # Rear axle in ego/camera frame
    cx_ra = -waypoint_shift
    cy_ra = 0.0

    # Lookahead circle on ground around rear axle
    thetas = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    circ_x = cx_ra + ld * np.cos(thetas)
    circ_y = cy_ra + ld * np.sin(thetas)  # Y left
    circ_pts = project_iso_xyz(
        circ_x,
        circ_y,
        np.zeros_like(circ_x),
        geom,
        w,
        h,
        x_min=0.3,
        y_sign=y_sign_for_project,
    )
    if len(circ_pts) >= 2:
        for a, b in zip(circ_pts, circ_pts[1:]):
            cv2.line(bgr, a, b, (255, 128, 0), 1, cv2.LINE_AA)
        cv2.line(bgr, circ_pts[-1], circ_pts[0], (255, 128, 0), 1, cv2.LINE_AA)

    # Curvature arc (bicycle path from rear axle)
    kappa = pp.curvature
    arc_len = min(max(ld * 1.5, 15.0), 50.0)
    arc_ra = bicycle_arc_points(kappa, length_m=arc_len, n=48)
    arc_x = arc_ra[:, 0] + cx_ra
    arc_y = arc_ra[:, 1] + cy_ra
    arc_pts = project_iso_xyz(
        arc_x,
        arc_y,
        np.zeros_like(arc_x),
        geom,
        w,
        h,
        x_min=0.3,
        y_sign=y_sign_for_project,
    )
    if len(arc_pts) >= 2:
        for a, b in zip(arc_pts, arc_pts[1:]):
            cv2.line(bgr, a, b, (255, 0, 255), 3, cv2.LINE_AA)

    # Rear axle marker
    ra_pts = project_iso_xyz(
        np.array([cx_ra]),
        np.array([cy_ra]),
        np.zeros(1),
        geom,
        w,
        h,
        x_min=-5.0,
        y_sign=y_sign_for_project,
        margin=200.0,
    )
    if ra_pts:
        cv2.drawMarker(bgr, ra_pts[0], (255, 128, 0), cv2.MARKER_TILTED_CROSS, 12, 2)

    if pp.target_ego is not None:
        tx, ty = float(pp.target_ego[0]), float(pp.target_ego[1])
        tgt = project_iso_xyz(
            np.array([tx]),
            np.array([ty]),
            np.zeros(1),
            geom,
            w,
            h,
            x_min=0.3,
            y_sign=y_sign_for_project,
        )
        if tgt:
            cv2.circle(bgr, tgt[0], 7, (0, 0, 255), -1, cv2.LINE_AA)
            cv2.circle(bgr, tgt[0], 9, (255, 255, 255), 1, cv2.LINE_AA)
            if ra_pts:
                cv2.line(bgr, ra_pts[0], tgt[0], (0, 0, 255), 2, cv2.LINE_AA)

    draw_steering_wheel(bgr, pp.steer_rad, steer_ratio=steer_ratio)

    steer_deg = float(np.rad2deg(pp.steer_rad))
    alpha_deg = float(np.rad2deg(pp.alpha_rad))
    if np.isfinite(pp.radius_m):
        curv_txt = f"κ={kappa:.4f}/m  R={pp.radius_m:.1f}m"
    else:
        curv_txt = "κ=0  R=∞"
    status = (
        f"PP Ld={ld:.1f}m  v={pp.speed_mps:.1f}  "
        f"α={alpha_deg:.1f}°  δ={steer_deg:.1f}°  {curv_txt}"
    )
    if pp.target_ego is None:
        status += "  (no target)"
    cv2.putText(
        bgr,
        status,
        (8, 40),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.45,
        (0, 165, 255),
        1,
        cv2.LINE_AA,
    )
    cv2.putText(
        bgr,
        "magenta=curvature arc  orange=Ld  red=target",
        (8, 58),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.4,
        (200, 150, 200),
        1,
        cv2.LINE_AA,
    )
