#!/usr/bin/env python3
"""Dynamic camera pitch/yaw calibration from vanishing point (AAD).

Port of Algorithms-for-Automated-Driving
  ``code/solutions/camera_calibration/calibrated_lane_detector.py``:

  - Fit left/right lane lines in the image: ``v = m*u + c``
  - Vanishing point = line intersection
  - ``pitch, yaw = get_py_from_vp(u_i, v_i, K)``  (roll assumed 0)

Does **not** estimate height or roll (same limitations as AAD / openpilot).
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import List, Optional, Sequence, Tuple

import cv2
import numpy as np

Line2 = Tuple[float, float]  # (m, c) for v = m*u + c, or poly1d coeffs


def get_intersection(line1: Line2, line2: Line2) -> Optional[Tuple[float, float]]:
    """AAD ``get_intersection``: intersection of v=m1*u+c1 and v=m2*u+c2."""
    m1, c1 = line1
    m2, c2 = line2
    if abs(m1 - m2) < 1e-9:
        return None
    u_i = (c2 - c1) / (m1 - m2)
    v_i = m1 * u_i + c1
    return float(u_i), float(v_i)


def get_py_from_vp(u_i: float, v_i: float, K: np.ndarray) -> Tuple[float, float]:
    """AAD ``get_py_from_vp``: pitch, yaw (radians) from vanishing point + K.

    Assumes roll = 0. Pitch sign matches AAD CameraGeometry (negative = looking down).
    """
    p_infinity = np.array([u_i, v_i, 1.0], dtype=np.float64)
    K_inv = np.linalg.inv(np.asarray(K, dtype=np.float64))
    r3 = K_inv @ p_infinity
    r3 /= np.linalg.norm(r3)
    yaw = -np.arctan2(r3[0], r3[2])
    pitch = np.arcsin(np.clip(r3[1], -1.0, 1.0))
    return float(pitch), float(yaw)


def fit_line_v_of_u(
    us: np.ndarray,
    vs: np.ndarray,
    mean_residuals_thresh: float = 25.0,
) -> Optional[Line2]:
    """AAD ``_fit_line_v_of_u``: polyfit v(u), reject curved / noisy fits."""
    us = np.asarray(us, dtype=np.float64).reshape(-1)
    vs = np.asarray(vs, dtype=np.float64).reshape(-1)
    mask = np.isfinite(us) & np.isfinite(vs)
    us, vs = us[mask], vs[mask]
    if us.size < 8:
        return None
    coeffs, residuals, _, _, _ = np.polyfit(us, vs, deg=1, full=True)
    if residuals.size == 0:
        return None
    mean_residuals = float(residuals[0] / len(us))
    if mean_residuals > mean_residuals_thresh:
        return None
    m, c = float(coeffs[0]), float(coeffs[1])
    return m, c


def fit_line_v_of_u_ransac(
    us: np.ndarray,
    vs: np.ndarray,
    thresh_px: float = 8.0,
    min_inliers: int = 12,
    max_iters: int = 80,
) -> Optional[Line2]:
    """Robust v=m*u+c fit for cluttered Hough endpoints."""
    us = np.asarray(us, dtype=np.float64).reshape(-1)
    vs = np.asarray(vs, dtype=np.float64).reshape(-1)
    mask = np.isfinite(us) & np.isfinite(vs)
    us, vs = us[mask], vs[mask]
    n = us.size
    if n < min_inliers:
        return None
    rng = np.random.default_rng(0)
    best_inliers = None
    best_count = 0
    for _ in range(max_iters):
        i0, i1 = rng.choice(n, size=2, replace=False)
        u0, v0 = us[i0], vs[i0]
        u1, v1 = us[i1], vs[i1]
        if abs(u1 - u0) < 1e-3:
            continue
        m = (v1 - v0) / (u1 - u0)
        c = v0 - m * u0
        resid = np.abs(vs - (m * us + c))
        inl = resid < thresh_px
        cnt = int(inl.sum())
        if cnt > best_count:
            best_count = cnt
            best_inliers = inl
    if best_inliers is None or best_count < min_inliers:
        return None
    coeffs = np.polyfit(us[best_inliers], vs[best_inliers], deg=1)
    return float(coeffs[0]), float(coeffs[1])


def lines_from_image_hough(
    bgr: np.ndarray,
    cut_v_frac: float = 0.42,
) -> Tuple[Optional[Line2], Optional[Line2], Optional[np.ndarray]]:
    """Extract left/right lane-ish lines via Canny + Hough + RANSAC.

    Returns (line_left, line_right, debug_edges) where lines are (m,c) for v=m*u+c.
    """
    h, w = bgr.shape[:2]
    cut_v = int(cut_v_frac * h)
    gray = cv2.cvtColor(bgr, cv2.COLOR_BGR2GRAY)
    gray = cv2.GaussianBlur(gray, (5, 5), 0)
    edges = cv2.Canny(gray, 40, 120)
    edges[:cut_v, :] = 0

    segs = cv2.HoughLinesP(
        edges,
        1,
        np.pi / 180.0,
        threshold=25,
        minLineLength=max(18, h // 14),
        maxLineGap=45,
    )
    if segs is None:
        return None, None, edges

    left_u, left_v, right_u, right_v = [], [], [], []
    for x1, y1, x2, y2 in segs[:, 0]:
        if y1 < cut_v and y2 < cut_v:
            continue
        dx, dy = float(x2 - x1), float(y2 - y1)
        if abs(dx) < 2:
            continue
        slope = dy / dx  # dv/du (v down)
        if abs(slope) < 0.12 or abs(slope) > 6.0:
            continue
        length = float(np.hypot(dx, dy))
        weight = max(1, int(round(length / 20.0)))
        mx = 0.5 * (x1 + x2)
        # Converging lanes: left half slope < 0, right half slope > 0 (image coords)
        if mx < w * 0.5 and slope < 0:
            left_u.extend([x1, x2] * weight)
            left_v.extend([y1, y2] * weight)
        elif mx >= w * 0.5 and slope > 0:
            right_u.extend([x1, x2] * weight)
            right_v.extend([y1, y2] * weight)

    line_l = fit_line_v_of_u_ransac(np.array(left_u), np.array(left_v)) if left_u else None
    line_r = fit_line_v_of_u_ransac(np.array(right_u), np.array(right_v)) if right_u else None
    if line_l is not None and line_r is not None and line_l[0] * line_r[0] >= 0:
        # Same-sign slopes → not a converging pair
        return None, None, edges
    return line_l, line_r, edges


def lines_from_projected_lanes(
    uv_left: np.ndarray,
    uv_right: np.ndarray,
    mean_residuals_thresh: float = 25.0,
) -> Tuple[Optional[Line2], Optional[Line2]]:
    """Fit v(u) lines from Nx2 image polylines (e.g. projected ego lanes)."""

    def _fit(uv: np.ndarray) -> Optional[Line2]:
        uv = np.asarray(uv, dtype=np.float64)
        if uv.ndim != 2 or uv.shape[0] < 8:
            return None
        ok = np.isfinite(uv).all(axis=1)
        return fit_line_v_of_u(uv[ok, 0], uv[ok, 1], mean_residuals_thresh)

    return _fit(uv_left), _fit(uv_right)


@dataclass
class VanishingPointCalibrator:
    """Online AAD calibrator: accumulate pitch/yaw from VP, then take the mean."""

    history_len: int = 50
    mean_residuals_thresh: float = 25.0
    estimated_pitch_deg: float = -5.0
    estimated_yaw_deg: float = 0.0
    calibration_success: bool = False
    pitch_yaw_history: List[List[float]] = field(default_factory=list)
    last_vp: Optional[Tuple[float, float]] = None
    n_updates: int = 0

    def reset(self) -> None:
        self.pitch_yaw_history.clear()
        self.calibration_success = False
        self.last_vp = None
        self.n_updates = 0

    def update_from_lines(self, line_left: Line2, line_right: Line2, K: np.ndarray) -> bool:
        """One frame update. Returns True when a new mean estimate was committed."""
        vp = get_intersection(line_left, line_right)
        if vp is None:
            return False
        u_i, v_i = vp
        cx, cy = float(K[0, 2]), float(K[1, 2])
        fx, fy = float(K[0, 0]), float(K[1, 1])
        # Reject VP far outside a generous image box
        if not (-0.5 * cx <= u_i <= 2.5 * cx and -0.5 * cy <= v_i <= 2.5 * cy):
            return False
        pitch, yaw = get_py_from_vp(u_i, v_i, K)
        # Reject physically implausible windshield poses / Hough junk
        if abs(np.rad2deg(pitch)) > 25.0 or abs(np.rad2deg(yaw)) > 15.0:
            return False
        # Looking-down camera: road VP is at/above principal point (v <= cy).
        # Hough clutter often intersects below cy → false +pitch and lifts overlay.
        if v_i > cy + 0.05 * max(fy, 1.0):
            return False
        if abs(v_i - cy) > 0.45 * max(fy, 1.0):
            return False
        self.last_vp = (u_i, v_i)
        return self.add_to_pitch_yaw_history(pitch, yaw)

    def add_to_pitch_yaw_history(self, pitch: float, yaw: float) -> bool:
        """AAD ``add_to_pitch_yaw_history``."""
        self.pitch_yaw_history.append([pitch, yaw])
        if len(self.pitch_yaw_history) <= self.history_len:
            return False
        py = np.asarray(self.pitch_yaw_history, dtype=np.float64)
        self.estimated_pitch_deg = float(np.rad2deg(np.mean(py[:, 0])))
        self.estimated_yaw_deg = float(np.rad2deg(np.mean(py[:, 1])))
        self.calibration_success = True
        self.n_updates += 1
        self.pitch_yaw_history = []
        return True

    def update_from_image(self, bgr: np.ndarray, K: np.ndarray) -> bool:
        line_l, line_r, _ = lines_from_image_hough(bgr)
        if line_l is None or line_r is None:
            return False
        return self.update_from_lines(line_l, line_r, K)

    def to_dict(self) -> dict:
        return {
            "roll_deg": 0.0,
            "pitch_deg": self.estimated_pitch_deg,
            "yaw_deg": self.estimated_yaw_deg,
            "calibration_success": self.calibration_success,
            "n_updates": self.n_updates,
            "last_vp": list(self.last_vp) if self.last_vp else None,
            "method": "AAD vanishing point (get_py_from_vp)",
            "note": "roll=0 and height not estimated (same as AAD)",
        }


def K_from_fx_fy_cx_cy(fx: float, fy: float, cx: float, cy: float) -> np.ndarray:
    return np.array([[fx, 0.0, cx], [0.0, fy, cy], [0.0, 0.0, 1.0]], dtype=np.float64)


def draw_vp_debug(
    img: np.ndarray,
    line_left: Optional[Line2],
    line_right: Optional[Line2],
    vp: Optional[Tuple[float, float]],
    pitch_deg: float,
    yaw_deg: float,
    ok: bool,
) -> None:
    h, w = img.shape[:2]

    def _draw_line(line: Line2, color) -> None:
        m, c = line
        u0, u1 = 0, w - 1
        v0, v1 = int(m * u0 + c), int(m * u1 + c)
        cv2.line(img, (u0, v0), (u1, v1), color, 1, cv2.LINE_AA)

    if line_left is not None:
        _draw_line(line_left, (0, 255, 255))
    if line_right is not None:
        _draw_line(line_right, (0, 255, 255))
    if vp is not None:
        u, v = int(round(vp[0])), int(round(vp[1]))
        cv2.drawMarker(img, (u, v), (0, 0, 255), cv2.MARKER_CROSS, 16, 2)
    status = "VP calib OK" if ok else "VP calibrating…"
    cv2.putText(
        img,
        f"{status}  pitch={pitch_deg:.2f} yaw={yaw_deg:.2f} deg",
        (8, h - 28),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.45,
        (0, 255, 0) if ok else (0, 200, 255),
        1,
        cv2.LINE_AA,
    )
