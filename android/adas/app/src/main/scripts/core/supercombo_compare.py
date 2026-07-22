#!/usr/bin/env python3
"""Temporary: run openpilot supercombo.onnx and draw BEV (same as original demo)."""

from __future__ import annotations

from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import cv2
import numpy as np

from .lane_projection import CameraGeometry, project_iso_xyz
from .supercombo_parse import (
    X_IDXS,
    parse_supercombo,
    SupercomboOut,
    explain_output,
)

SUPERCOMBO_DIR = Path("/home/anatoly/atom/openpilot-supercombo-model")
DEFAULT_MODEL = SUPERCOMBO_DIR / "supercombo.onnx"


def parse_image_yuv(frame_yuv_i420: np.ndarray) -> np.ndarray:
    H = (frame_yuv_i420.shape[0] * 2) // 3
    W = frame_yuv_i420.shape[1]
    parsed = np.zeros((6, H // 2, W // 2), dtype=np.uint8)
    parsed[0] = frame_yuv_i420[0:H:2, 0::2]
    parsed[1] = frame_yuv_i420[1:H:2, 0::2]
    parsed[2] = frame_yuv_i420[0:H:2, 1::2]
    parsed[3] = frame_yuv_i420[1:H:2, 1::2]
    parsed[4] = frame_yuv_i420[H : H + H // 4].reshape((-1, H // 2, W // 2))
    parsed[5] = frame_yuv_i420[H + H // 4 : H + H // 2].reshape((-1, H // 2, W // 2))
    return parsed


def make_overlay_geometry(
    fx: float,
    fy: float,
    cx: float,
    cy: float,
    w: int,
    h: int,
    camera_height: float = 1.22,
    pitch_deg: float = -5.0,
    yaw_deg: float = 0.0,
    roll_deg: float = 0.0,
    cam_x: float = 0.0,
    cam_y_left: float = 0.0,
) -> CameraGeometry:
    """Build AAD CameraGeometry for projecting model ego XYZ into the image."""
    if fy <= 1.0 or (fx > 1.0 and abs(fx / max(fy, 1e-6) - 1.0) > 0.15):
        fy = fx
    K = np.array([[fx, 0.0, cx], [0.0, fy, cy], [0.0, 0.0, 1.0]], dtype=np.float64)
    return CameraGeometry(
        height=camera_height,
        pitch_deg=pitch_deg,
        yaw_deg=yaw_deg,
        roll_deg=roll_deg,
        cam_x=cam_x,
        cam_y_left=cam_y_left,
        image_width=w,
        image_height=h,
        intrinsic_matrix=K,
    )


def project_xyz(
    xs: np.ndarray,
    ys: np.ndarray,
    zs: np.ndarray,
    fx: float,
    fy: float,
    cx: float,
    cy: float,
    camera_height: float,
    y_sign: float,
    w: int,
    h: int,
    x_min: float = 1.5,
    pitch_deg: float = -5.0,
    yaw_deg: float = 0.0,
    geom: Optional[CameraGeometry] = None,
) -> List[Tuple[int, int]]:
    """Ego (X fwd, Y left, Z up) → image via AAD CameraGeometry (pitch/yaw)."""
    if geom is None:
        geom = make_overlay_geometry(
            fx, fy, cx, cy, w, h, camera_height, pitch_deg=pitch_deg, yaw_deg=yaw_deg
        )
    return project_iso_xyz(xs, ys, zs, geom, w, h, x_min=x_min, y_sign=y_sign)


def draw_pts(img: np.ndarray, pts: List[Tuple[int, int]], color, thickness: int) -> None:
    for a, b in zip(pts[:-1], pts[1:]):
        cv2.line(img, a, b, color, thickness, lineType=cv2.LINE_AA)


def draw_runtime_lanes(
    img: np.ndarray,
    out: SupercomboOut,
    geom: CameraGeometry,
    w: int,
    h: int,
    y_sign: float = -1.0,
    min_lane_prob: float = 0.3,
) -> None:
    """Draw lane lines from live supercombo inference."""
    for lane in out.lanes:
        if lane.prob < min_lane_prob:
            continue
        pts = project_iso_xyz(
            X_IDXS,
            lane.y,
            np.zeros_like(lane.y),
            geom,
            w,
            h,
            y_sign=y_sign,
        )
        draw_pts(img, pts, (0, 255, 255), 2)


def draw_bag_lanes(
    img: np.ndarray,
    lane_msg: Any,
    geom: CameraGeometry,
    w: int,
    h: int,
    y_sign: float = 1.0,
    min_lane_prob: float = 0.3,
    draw_edges: bool = True,
    draw_plan: bool = True,
) -> None:
    """Draw lane lines from bag ``vision/lanes``.

    Android stores ISO Y-left (already negated), so default ``y_sign=1``.
    """
    xs = np.asarray(list(lane_msg.x), dtype=np.float64) if lane_msg.x else X_IDXS.copy()
    for lane in lane_msg.lanes:
        if float(lane.prob) < min_lane_prob:
            continue
        y = np.asarray(list(lane.y), dtype=np.float64)
        if y.size != xs.size:
            continue
        pts = project_iso_xyz(xs, y, np.zeros_like(y), geom, w, h, y_sign=y_sign)
        draw_pts(img, pts, (0, 255, 255), 2)

    if draw_edges:
        for edge in getattr(lane_msg, "edges", []):
            y = np.asarray(list(edge.y), dtype=np.float64)
            if y.size != xs.size:
                continue
            pts = project_iso_xyz(xs, y, np.zeros_like(y), geom, w, h, y_sign=y_sign)
            draw_pts(img, pts, (0, 0, 255), 2)

    if draw_plan and getattr(lane_msg, "plan_x", None) and getattr(lane_msg, "plan_y", None):
        px = np.asarray(list(lane_msg.plan_x), dtype=np.float64)
        py = np.asarray(list(lane_msg.plan_y), dtype=np.float64)
        if px.size >= 2 and px.size == py.size:
            pts = project_iso_xyz(px, py, np.zeros_like(py), geom, w, h, x_min=0.5, y_sign=y_sign)
            draw_pts(img, pts, (0, 255, 0), 3)


def supercombo_lanes_to_ego(
    out: SupercomboOut,
    *,
    min_lane_prob: float = 0.3,
    y_sign: float = -1.0,
    x_min: float = 1.0,
    x_max: float = 40.0,
) -> Dict[str, np.ndarray]:
    """Convert supercombo near-lanes to MetaDrive-style ``left_road`` / ``right_road``.

    Output polylines are ego ISO: X forward, Y left. Raw ONNX Y is right-positive,
    so ``y_sign=-1`` flips to left-positive (same as ``plan_to_polyline_ego``).
    """
    # LANE_NAMES: leftFar, leftNear, rightNear, rightFar
    by_name = {lane.name: lane for lane in out.lanes}
    left = by_name.get("leftNear")
    right = by_name.get("rightNear")
    result: Dict[str, np.ndarray] = {
        "left_road": np.empty((0, 2), dtype=np.float64),
        "right_road": np.empty((0, 2), dtype=np.float64),
    }
    xs = X_IDXS
    ok = (xs >= x_min) & (xs <= x_max) & np.isfinite(xs)
    if left is not None and left.prob >= min_lane_prob:
        y = y_sign * np.asarray(left.y, dtype=np.float64)
        m = ok & np.isfinite(y)
        result["left_road"] = np.stack([xs[m], y[m]], axis=1)
    if right is not None and right.prob >= min_lane_prob:
        y = y_sign * np.asarray(right.y, dtype=np.float64)
        m = ok & np.isfinite(y)
        result["right_road"] = np.stack([xs[m], y[m]], axis=1)
    return result


def draw_supercombo_overlay(
    img: np.ndarray,
    out: SupercomboOut,
    geom: CameraGeometry,
    w: int,
    h: int,
    *,
    y_sign: float = -1.0,
    min_lane_prob: float = 0.3,
    draw_plan: bool = True,
    draw_edges: bool = True,
    draw_lanes: bool = True,
    lane_tag: str = "runtime",
) -> None:
    """Draw plan (green), lanes (yellow), road edges (red) — same as visualizer."""
    if draw_edges:
        for edge in out.edges:
            pts = project_iso_xyz(
                X_IDXS,
                edge.y,
                np.zeros_like(edge.y),
                geom,
                w,
                h,
                y_sign=y_sign,
            )
            draw_pts(img, pts, (0, 0, 255), 2)

    if draw_lanes:
        draw_runtime_lanes(img, out, geom, w, h, y_sign=y_sign, min_lane_prob=min_lane_prob)

    if draw_plan:
        pts = project_iso_xyz(
            out.plan.x,
            out.plan.y,
            out.plan.z,
            geom,
            w,
            h,
            x_min=0.5,
            y_sign=y_sign,
        )
        draw_pts(img, pts, (0, 255, 0), 3)

    probs = [f"{lane.prob:.2f}" for lane in out.lanes]
    cv2.putText(
        img,
        f"supercombo plan#{out.plan.hyp_index}  lanes={lane_tag}  p={probs}",
        (8, 20),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.4,
        (255, 255, 255),
        1,
        cv2.LINE_AA,
    )


class SupercomboBev:
    """Lazy ONNX runner; correct v0.8-style parse (plan + lanes + edges)."""

    def __init__(self, model_path: Path = DEFAULT_MODEL):
        self.model_path = Path(model_path)
        self._session = None
        self._names: Optional[Tuple[str, str, str, str, str]] = None
        self._prev: Optional[np.ndarray] = None
        self._prev_key: Optional[int] = None
        self._cache_key: Optional[int] = None
        self._cache_out: Optional[SupercomboOut] = None
        self.error: Optional[str] = None

    def _ensure(self) -> bool:
        if self._session is not None:
            return True
        if not self.model_path.is_file():
            self.error = f"model missing: {self.model_path}"
            return False
        try:
            import onnxruntime as ort

            self._session = ort.InferenceSession(
                str(self.model_path), providers=["CPUExecutionProvider"]
            )
            ins = self._session.get_inputs()
            outs = self._session.get_outputs()
            self._names = (
                ins[0].name,
                ins[1].name,
                ins[2].name,
                ins[3].name,
                outs[0].name,
            )
            self.error = None
            return True
        except Exception as e:
            self.error = str(e)
            return False

    def reset(self) -> None:
        self._prev = None
        self._prev_key = None
        self._cache_key = None
        self._cache_out = None

    def infer(self, bgr: np.ndarray, cache_key: Optional[int] = None) -> Optional[SupercomboOut]:
        if cache_key is not None and cache_key == self._cache_key and self._cache_out is not None:
            return self._cache_out
        if not self._ensure():
            return None

        assert self._session is not None and self._names is not None
        img = cv2.resize(bgr, (512, 256))
        yuv = cv2.cvtColor(img, cv2.COLOR_BGR2YUV_I420)
        parsed = parse_image_yuv(yuv).astype(np.float32)
        # Temporal pair must be consecutive frames. Skipping (fast play / scrub)
        # makes lanes lag and warp — duplicate current instead of a stale _prev.
        consecutive = (
            cache_key is not None and self._prev_key is not None and cache_key == self._prev_key + 1
        )
        if self._prev is None or not consecutive:
            prev = parsed
        else:
            prev = self._prev
        data = np.stack([prev, parsed], axis=0).reshape(1, 12, 128, 256)
        self._prev = parsed
        self._prev_key = cache_key

        desire = np.zeros((1, 8), np.float32)
        traffic = np.array([[1.0, 0.0]], np.float32)
        state = np.zeros((1, 512), np.float32)
        in_imgs, in_desire, in_traffic, in_state, out_name = self._names
        (out,) = self._session.run(
            [out_name],
            {
                in_imgs: data,
                in_desire: desire,
                in_traffic: traffic,
                in_state: state,
            },
        )
        parsed_out = parse_supercombo(out.reshape(-1))
        if cache_key is not None:
            self._cache_key = cache_key
            self._cache_out = parsed_out
        return parsed_out

    def overlay_on_image(
        self,
        bgr: np.ndarray,
        fx: float,
        fy: float,
        cx: float,
        cy: float,
        camera_height: float = 1.22,
        cache_key: Optional[int] = None,
        y_sign: float = -1.0,
        min_lane_prob: float = 0.3,
        pitch_deg: float = -5.0,
        yaw_deg: float = 0.0,
        roll_deg: float = 0.0,
        geom: Optional[CameraGeometry] = None,
    ) -> np.ndarray:
        """Draw PLAN (green), lanes (yellow), road edges (red) on camera image.

        ``y_sign=-1``: this ONNX matches Android ``u=cx+fx*Y/X`` (Y positive right).
        ``project_iso_xyz`` assumes openpilot ISO Y-left, so we flip once.
        """
        h_img, w_img = bgr.shape[:2]
        out = self.infer(bgr, cache_key=cache_key)
        if out is None:
            cv2.putText(
                bgr,
                f"supercombo ERR: {(self.error or '')[:50]}",
                (8, h_img - 12),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.45,
                (0, 0, 255),
                1,
                cv2.LINE_AA,
            )
            return bgr

        if geom is None:
            geom = make_overlay_geometry(
                fx,
                fy,
                cx,
                cy,
                w_img,
                h_img,
                camera_height,
                pitch_deg=pitch_deg,
                yaw_deg=yaw_deg,
                roll_deg=roll_deg,
            )

        # road edges (z≈noise in this ONNX — project on ground like LaneOverlayView)
        for edge in out.edges:
            pts = project_iso_xyz(
                X_IDXS,
                edge.y,
                np.zeros_like(edge.y),
                geom,
                w_img,
                h_img,
                y_sign=y_sign,
            )
            draw_pts(bgr, pts, (0, 0, 255), 2)

        draw_runtime_lanes(
            bgr,
            out,
            geom,
            w_img,
            h_img,
            y_sign=y_sign,
            min_lane_prob=min_lane_prob,
        )

        # planned path (best of 5 MHP) — this is the primary model output
        pts = project_iso_xyz(
            out.plan.x,
            out.plan.y,
            out.plan.z,
            geom,
            w_img,
            h_img,
            x_min=0.5,
            y_sign=y_sign,
        )
        draw_pts(bgr, pts, (0, 255, 0), 3)

        probs = [f"{l.prob:.2f}" for l in out.lanes]
        cv2.putText(
            bgr,
            f"plan hyp#{out.plan.hyp_index}  lanes p={probs}",
            (8, 20),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.4,
            (255, 255, 255),
            1,
            cv2.LINE_AA,
        )
        cv2.putText(
            bgr,
            "green=PLAN  yellow=lanes  red=edges",
            (8, h_img - 12),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.45,
            (0, 255, 0),
            1,
            cv2.LINE_AA,
        )
        return bgr
