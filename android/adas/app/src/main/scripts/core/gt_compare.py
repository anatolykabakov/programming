#!/usr/bin/env python3
"""Compare control path / Pure Pursuit against MetaDrive GT centerline.

Used to tune camera calib (pitch/yaw) and PP (Ld, shift, path source):
  - ``path_bias``  : mean(y_ctrl − y_gt) — perception lateral offset (m, + = left)
  - ``path_rmse``  : RMSE of the same over a longitudinal window
  - ``gt_ey``      : GT centerline Y near ego — vehicle cross-track (m, + = center left of car)
  - ``dsteer``     : steer_ctrl − steer_gt (rad) — closed-loop command error vs GT PP
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Sequence

import numpy as np

from .lane_keep import LaneKeepController, LaneKeepResult, build_centerline_polyline


def interp_polyline_y(poly: Optional[np.ndarray], x: float) -> Optional[float]:
    if poly is None:
        return None
    p = np.asarray(poly, dtype=np.float64)
    if p.ndim != 2 or p.shape[0] < 2:
        return None
    order = np.argsort(p[:, 0])
    xs, ys = p[order, 0], p[order, 1]
    if x < xs[0] - 1e-6 or x > xs[-1] + 1e-6:
        return None
    return float(np.interp(x, xs, ys))


def path_lateral_errors(
    ctrl_poly: Optional[np.ndarray],
    gt_poly: Optional[np.ndarray],
    x_lo: float = 5.0,
    x_hi: float = 25.0,
    n: int = 21,
) -> tuple[float, float, int]:
    """Return (bias, rmse, n_valid) of y_ctrl − y_gt over [x_lo, x_hi]."""
    if ctrl_poly is None or gt_poly is None:
        return float("nan"), float("nan"), 0
    xs = np.linspace(x_lo, x_hi, n)
    dys: List[float] = []
    for x in xs:
        yc = interp_polyline_y(ctrl_poly, float(x))
        yg = interp_polyline_y(gt_poly, float(x))
        if yc is None or yg is None:
            continue
        dys.append(yc - yg)
    if not dys:
        return float("nan"), float("nan"), 0
    arr = np.asarray(dys, dtype=np.float64)
    return float(np.mean(arr)), float(np.sqrt(np.mean(arr * arr))), int(arr.size)


@dataclass
class GtCompareFrame:
    """One-step comparison against GT centerline / GT Pure Pursuit."""

    gt_poly: Optional[np.ndarray]
    ctrl_poly: Optional[np.ndarray]
    lk_gt: Optional[LaneKeepResult]
    gt_ey: float = float("nan")  # GT center Y at x≈5 m
    path_bias: float = float("nan")
    path_rmse: float = float("nan")
    path_n: int = 0
    dy_at_ld: float = float("nan")  # y_ctrl − y_gt at applied Ld (or 8 m)
    steer_gt_rad: float = float("nan")
    dsteer_rad: float = float("nan")
    tgt_y_gt: float = float("nan")
    tgt_y_ctrl: float = float("nan")
    dtgt_y: float = float("nan")


@dataclass
class GtCompareAccumulator:
    """Running stats for end-of-run tuning summary."""

    path_bias: List[float] = field(default_factory=list)
    path_rmse: List[float] = field(default_factory=list)
    gt_ey: List[float] = field(default_factory=list)
    dsteer_deg: List[float] = field(default_factory=list)
    dtgt_y: List[float] = field(default_factory=list)
    pitch_deg: List[float] = field(default_factory=list)
    yaw_deg: List[float] = field(default_factory=list)

    def add(self, frame: GtCompareFrame, *, pitch_deg: float, yaw_deg: float) -> None:
        if np.isfinite(frame.path_bias):
            self.path_bias.append(frame.path_bias)
        if np.isfinite(frame.path_rmse):
            self.path_rmse.append(frame.path_rmse)
        if np.isfinite(frame.gt_ey):
            self.gt_ey.append(frame.gt_ey)
        if np.isfinite(frame.dsteer_rad):
            self.dsteer_deg.append(float(np.rad2deg(frame.dsteer_rad)))
        if np.isfinite(frame.dtgt_y):
            self.dtgt_y.append(frame.dtgt_y)
        self.pitch_deg.append(float(pitch_deg))
        self.yaw_deg.append(float(yaw_deg))

    def summary(self) -> Dict[str, Any]:
        def _stats(vals: Sequence[float]) -> Dict[str, float]:
            if not vals:
                return {}
            a = np.asarray(vals, dtype=np.float64)
            return {
                "mean": float(np.mean(a)),
                "std": float(np.std(a)),
                "p50": float(np.median(a)),
                "p95": float(np.percentile(a, 95)),
                "n": int(a.size),
            }

        out: Dict[str, Any] = {
            "path_bias_m": _stats(self.path_bias),
            "path_rmse_m": _stats(self.path_rmse),
            "gt_ey_m": _stats(self.gt_ey),
            "dsteer_deg": _stats(self.dsteer_deg),
            "dtgt_y_m": _stats(self.dtgt_y),
            "pitch_deg": _stats(self.pitch_deg),
            "yaw_deg": _stats(self.yaw_deg),
        }
        hints: List[str] = []
        pb = out["path_bias_m"]
        if pb:
            m = pb["mean"]
            if abs(m) > 0.05:
                side = "left" if m > 0 else "right"
                hints.append(
                    f"path_bias={m:+.3f} m ({side}): SC path is systematically "
                    f"{side} of GT — try --pp-on lanes/plan, recenter, or yaw trim "
                    f"(~{np.rad2deg(np.arctan2(m, 20.0)):.2f}° at 20 m)."
                )
        ds = out["dsteer_deg"]
        if ds and abs(ds["mean"]) > 0.3:
            side = "left" if ds["mean"] > 0 else "right"
            hints.append(
                f"dsteer={ds['mean']:+.2f}° mean: control steers more {side} than GT PP — "
                f"tune --pp-k-dd / --pp-ld-min / --pp-shift, or fix path bias first."
            )
        ey = out["gt_ey_m"]
        if ey and abs(ey["mean"]) > 0.15:
            side = "left of center" if ey["mean"] > 0 else "right of center"
            hints.append(
                f"gt_ey={ey['mean']:+.3f} m: ego sits {side} in closed loop "
                f"(tracking lag / bias)."
            )
        yw = out["yaw_deg"]
        if yw and abs(yw["mean"]) > 2.0:
            hints.append(
                f"VP yaw mean={yw['mean']:+.1f}°: large — prefer --vp-source gt or "
                f"--no-vp-calib with MetaDrive extrinsics for fair path compare."
            )
        out["hints"] = hints
        return out

    def format_summary(self) -> str:
        s = self.summary()
        lines = ["=== GT compare summary (tune calib / PP) ==="]

        def _line(name: str, key: str, unit: str) -> None:
            st = s.get(key) or {}
            if not st:
                return
            lines.append(
                f"  {name}: mean={st['mean']:+.4f}{unit}  "
                f"p50={st['p50']:+.4f}  p95={st['p95']:+.4f}  "
                f"std={st['std']:.4f}  n={st['n']}"
            )

        _line("path_bias (ctrl−GT)", "path_bias_m", " m")
        _line("path_rmse", "path_rmse_m", " m")
        _line("gt_ey (GT Y@5m)", "gt_ey_m", " m")
        _line("dsteer (ctrl−GT PP)", "dsteer_deg", "°")
        _line("dtgt_y", "dtgt_y_m", " m")
        _line("pitch", "pitch_deg", "°")
        _line("yaw", "yaw_deg", "°")
        for h in s.get("hints") or []:
            lines.append(f"  hint: {h}")
        return "\n".join(lines)


def compare_to_gt(
    controller: LaneKeepController,
    speed_mps: float,
    gt_lanes: Dict[str, Any],
    ctrl_lk: Optional[LaneKeepResult],
    *,
    x_bias_lo: float = 5.0,
    x_bias_hi: float = 25.0,
) -> GtCompareFrame:
    """Shadow GT centerline + GT PP; compare to the applied control result."""
    gt_poly = build_centerline_polyline(
        gt_lanes.get("left_road"),
        gt_lanes.get("right_road"),
    )
    lk_gt: Optional[LaneKeepResult] = None
    if gt_poly is not None and controller.mode in ("pure_pursuit", "lateral_pd"):
        lk_gt = controller.compute_from_polyline(speed_mps, gt_poly)

    ctrl_poly = ctrl_lk.polyline if ctrl_lk is not None else None
    bias, rmse, n = path_lateral_errors(ctrl_poly, gt_poly, x_bias_lo, x_bias_hi)

    gt_ey = interp_polyline_y(gt_poly, 5.0)
    if gt_ey is None:
        gt_ey = float("nan")

    ld = 8.0
    tgt_y_ctrl = tgt_y_gt = float("nan")
    steer_gt = float("nan")
    dsteer = float("nan")
    if ctrl_lk is not None and ctrl_lk.pure_pursuit is not None:
        ld = float(ctrl_lk.pure_pursuit.lookahead_m)
        if ctrl_lk.pure_pursuit.target_ego is not None:
            tgt_y_ctrl = float(ctrl_lk.pure_pursuit.target_ego[1])
    if lk_gt is not None:
        steer_gt = float(lk_gt.steer_rad)
        if ctrl_lk is not None:
            dsteer = float(ctrl_lk.steer_rad - lk_gt.steer_rad)
        if lk_gt.pure_pursuit is not None and lk_gt.pure_pursuit.target_ego is not None:
            tgt_y_gt = float(lk_gt.pure_pursuit.target_ego[1])

    yc = interp_polyline_y(ctrl_poly, ld)
    yg = interp_polyline_y(gt_poly, ld)
    dy_ld = float(yc - yg) if yc is not None and yg is not None else float("nan")
    dtgt = (
        float(tgt_y_ctrl - tgt_y_gt)
        if np.isfinite(tgt_y_ctrl) and np.isfinite(tgt_y_gt)
        else float("nan")
    )

    return GtCompareFrame(
        gt_poly=gt_poly,
        ctrl_poly=ctrl_poly,
        lk_gt=lk_gt,
        gt_ey=float(gt_ey),
        path_bias=bias,
        path_rmse=rmse,
        path_n=n,
        dy_at_ld=dy_ld,
        steer_gt_rad=steer_gt,
        dsteer_rad=dsteer,
        tgt_y_gt=tgt_y_gt,
        tgt_y_ctrl=tgt_y_ctrl,
        dtgt_y=dtgt,
    )
