"""Path fusion matching C++ ``adas::laneLinesToPath`` (device Y-right).

Use the returned Nx2 polyline with ``AdasApp.publish_lanes`` / PP — same frame
as Android TopicConvert → LaneKeep.
"""

from __future__ import annotations

from typing import Any, List, Optional, Sequence

import numpy as np

from .frames import DEFAULT_MIN_LANE_PROB, PP_Y_SIGN
from .pure_pursuit import plan_to_polyline_ego
from .supercombo_parse import X_IDXS, SupercomboOut


def _soft_lane_prob(p: float, min_p: float) -> float:
    if not (p >= min_p):
        return 0.0
    span = max(1e-3, 1.0 - float(min_p))
    return min(1.0, (float(p) - float(min_p)) / span)


def _interp_y(x: float, xs: np.ndarray, ys: np.ndarray) -> float:
    if xs.size == 0 or xs.size != ys.size:
        return 0.0
    if x <= xs[0]:
        return float(ys[0])
    if x >= xs[-1]:
        return float(ys[-1])
    return float(np.interp(x, xs, ys))


def lane_lines_to_path(
    plan_x: Optional[np.ndarray],
    plan_y: Optional[np.ndarray],
    x_idxs: np.ndarray,
    lane_ys: Sequence[Optional[np.ndarray]],
    lane_probs: Sequence[float],
    min_lane_prob: float = DEFAULT_MIN_LANE_PROB,
) -> Optional[np.ndarray]:
    """Fuse PLAN + near L/R lanes → Nx2 device-frame polyline (Y right+).

    ``lane_ys`` / ``lane_probs`` indexed like proto: 0=leftFar, 1=leftNear,
    2=rightNear, 3=rightFar. Only indices 1 and 2 are used (Android/C++).
    """
    poly: Optional[np.ndarray] = None
    if plan_x is not None and plan_y is not None and len(plan_x) >= 2:
        poly = plan_to_polyline_ego(
            np.asarray(plan_x, dtype=np.float64),
            np.asarray(plan_y, dtype=np.float64),
            y_sign=PP_Y_SIGN,
            x_min=1.0,
        )
        if poly.shape[0] < 2:
            poly = None

    xs = np.asarray(x_idxs, dtype=np.float64)
    have_l = (
        len(lane_ys) > 1 and lane_ys[1] is not None and len(lane_ys[1]) == len(xs) and len(xs) >= 2
    )
    have_r = (
        len(lane_ys) > 2 and lane_ys[2] is not None and len(lane_ys[2]) == len(xs) and len(xs) >= 2
    )
    l_prob = (
        _soft_lane_prob(float(lane_probs[1]) if len(lane_probs) > 1 else 0.0, min_lane_prob)
        if have_l
        else 0.0
    )
    r_prob = (
        _soft_lane_prob(float(lane_probs[2]) if len(lane_probs) > 2 else 0.0, min_lane_prob)
        if have_r
        else 0.0
    )
    d_prob = l_prob + r_prob - l_prob * r_prob

    if d_prob <= 1e-6:
        return poly

    yl = np.asarray(lane_ys[1], dtype=np.float64) if have_l else None
    yr = np.asarray(lane_ys[2], dtype=np.float64) if have_r else None
    lane_xs: List[float] = []
    lane_mid: List[float] = []
    for i in range(len(xs)):
        x = float(xs[i])
        y_l = float(yl[i]) if yl is not None else 0.0
        y_r = float(yr[i]) if yr is not None else 0.0
        if have_l and have_r:
            width = abs(y_l - y_r)
            w = min(4.0, max(2.6, width))
            from_l = y_l + 0.5 * w
            from_r = y_r - 0.5 * w
            lane_y = (l_prob * from_l + r_prob * from_r) / (l_prob + r_prob + 1e-6)
        elif have_l:
            lane_y = y_l + 1.6
        else:
            lane_y = y_r - 1.6
        lane_xs.append(x)
        lane_mid.append(lane_y)

    xs_a = np.asarray(lane_xs, dtype=np.float64)
    ys_a = np.asarray(lane_mid, dtype=np.float64)

    if poly is not None and poly.shape[0] >= 2:
        out = poly.copy()
        for i in range(out.shape[0]):
            y_lane = _interp_y(float(out[i, 0]), xs_a, ys_a)
            out[i, 1] = d_prob * y_lane + (1.0 - d_prob) * out[i, 1]
        return out

    ok = xs_a >= 1.0
    if not np.any(ok):
        return None
    return np.stack([xs_a[ok], ys_a[ok]], axis=1)


def path_from_supercombo(
    out: SupercomboOut,
    min_lane_prob: float = DEFAULT_MIN_LANE_PROB,
) -> Optional[np.ndarray]:
    lane_ys = []
    probs = []
    for i in range(4):
        if i < len(out.lanes):
            lane_ys.append(np.asarray(out.lanes[i].y, dtype=np.float64))
            probs.append(float(out.lanes[i].prob))
        else:
            lane_ys.append(None)
            probs.append(0.0)
    plan_x = out.plan.x if out.plan is not None else None
    plan_y = out.plan.y if out.plan is not None else None
    return lane_lines_to_path(plan_x, plan_y, X_IDXS, lane_ys, probs, min_lane_prob)


def path_from_bag_lanes(
    ll: Any,
    min_lane_prob: float = DEFAULT_MIN_LANE_PROB,
) -> Optional[np.ndarray]:
    """``vision/lanes`` protobuf (or similar) → device-frame PP polyline."""
    plan_x = np.asarray(list(ll.plan_x), dtype=np.float64) if getattr(ll, "plan_x", None) else None
    plan_y = np.asarray(list(ll.plan_y), dtype=np.float64) if getattr(ll, "plan_y", None) else None
    xs = (
        np.asarray(list(ll.x), dtype=np.float64) if getattr(ll, "x", None) and len(ll.x) else X_IDXS
    )
    lane_ys: List[Optional[np.ndarray]] = []
    probs: List[float] = []
    lanes = list(getattr(ll, "lanes", []) or [])
    for i in range(4):
        if i < len(lanes) and lanes[i].y:
            lane_ys.append(np.asarray(list(lanes[i].y), dtype=np.float64))
            probs.append(float(getattr(lanes[i], "prob", 0.0) or 0.0))
        else:
            lane_ys.append(None)
            probs.append(0.0)
    return lane_lines_to_path(plan_x, plan_y, xs, lane_ys, probs, min_lane_prob)


def iso_left_polyline_to_device(poly: np.ndarray) -> np.ndarray:
    """MetaDrive / ISO Y-left Nx2 → device Y-right for PP."""
    out = np.asarray(poly, dtype=np.float64).copy()
    if out.ndim == 2 and out.shape[1] >= 2:
        out[:, 1] = -out[:, 1]
    return out
