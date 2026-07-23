#!/usr/bin/env python3
"""Offline lane-keep via Simulated AdasApp (C++ Pure Pursuit).

From bag ``vision/lanes`` + ``vehicle/state``, recompute desired steer with the
**same path fusion as Android** (``laneLinesToPath``) and compare to measured
steering_angle_deg (no CAN TX).

Writes lane_keep_compare.csv and optional plot.
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

import numpy as np

import _path  # noqa: F401

from core.frames import DEFAULT_MAX_STEER_DEG
from core.lane_keep import LaneKeepController
from core.path_fusion import path_from_bag_lanes
from vis.bag_io import iter_aligned, list_topics, load_topic_messages


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("session", type=Path)
    ap.add_argument("-o", "--out", type=Path, default=None)
    ap.add_argument("--max-dt", type=int, default=80)
    ap.add_argument("--wheelbase", type=float, default=2.636)
    ap.add_argument("--steer-ratio", type=float, default=15.7)
    ap.add_argument("--pp-k-dd", type=float, default=0.4)
    ap.add_argument("--pp-ld-min", type=float, default=3.0)
    ap.add_argument("--pp-ld-max", type=float, default=20.0)
    ap.add_argument("--pp-shift", type=float, default=1.4)
    ap.add_argument("--max-steer-deg", type=float, default=DEFAULT_MAX_STEER_DEG)
    ap.add_argument("--min-lane-prob", type=float, default=0.3)
    ap.add_argument("--plot", action="store_true")
    args = ap.parse_args()

    session = args.session.resolve()
    print("Topics:", list_topics(session))
    lanes = load_topic_messages(session, "vision/lanes")
    state = load_topic_messages(session, "vehicle/state")
    if not state:
        state = load_topic_messages(session, "carState")
    if not lanes or not state:
        raise SystemExit("Need vision/lanes and vehicle/state")

    ctrl = LaneKeepController(
        mode="pure_pursuit",
        wheelbase=args.wheelbase,
        pp_k_dd=args.pp_k_dd,
        pp_ld_min=args.pp_ld_min,
        pp_ld_max=args.pp_ld_max,
        pp_shift=args.pp_shift,
        max_steer_deg=args.max_steer_deg,
    )

    rows = []
    for row in iter_aligned(lanes, {"state": state}, max_dt_ms=args.max_dt):
        ll, st = row["primary"], row["state"]
        if st is None:
            continue
        poly = path_from_bag_lanes(ll, min_lane_prob=args.min_lane_prob)
        if poly is None or poly.shape[0] < 2:
            continue

        v = max(float(st.v_ego), 0.0)
        lk = ctrl.compute_from_polyline(v, poly)
        # Device-frame δ → SWA (VW left+ uses steer_sign on phone; here compare
        # geometric road-wheel angle * ratio without sign flip unless requested).
        steer_cmd_deg = float(np.degrees(lk.steer_rad) * args.steer_ratio)
        steer_meas = float(st.steering_angle_deg)

        rows.append(
            {
                "t": row["t"],
                "v_ego": v,
                "yaw_rate": float(st.yaw_rate),
                "steer_cmd_deg": steer_cmd_deg,
                "steer_meas_deg": steer_meas,
                "steer_err_deg": steer_cmd_deg - steer_meas,
                "lookahead_m": float(lk.pure_pursuit.lookahead_m) if lk.pure_pursuit else 0.0,
                "curvature": float(lk.curvature),
                "status": lk.status,
            }
        )

    if not rows:
        raise SystemExit("No aligned samples")

    out = args.out or (session / "lane_keep_compare.csv")
    with open(out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"Wrote {len(rows)} rows → {out}")

    if args.plot:
        import matplotlib.pyplot as plt

        t = [r["t"] for r in rows]
        plt.figure(figsize=(10, 4))
        plt.plot(t, [r["steer_meas_deg"] for r in rows], label="meas SWA")
        plt.plot(t, [r["steer_cmd_deg"] for r in rows], label="PP*ratio (device δ)")
        plt.xlabel("t")
        plt.ylabel("deg")
        plt.legend()
        plt.tight_layout()
        plt.show()


if __name__ == "__main__":
    main()
