#!/usr/bin/env python3
"""Latency: capture/vision → steer/lane_keep publish (BOOTTIME ms stamps).

Requires bags recorded after capture_ts_ms / vision_ts_ms / publish_ts_ms were added.

  latency_capture_ms = publish_ts_ms - capture_ts_ms   # full pipeline
  latency_vision_ms  = publish_ts_ms - vision_ts_ms    # ONNX done → control

Usage:
  python3 bag_control_latency.py /path/to/session
"""

from __future__ import annotations

import argparse
import statistics
import sys
from pathlib import Path
from typing import List, Optional, Tuple

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR / "vis"))
sys.path.insert(0, str(SCRIPT_DIR / "proto"))

from bag_io import load_topic_messages  # noqa: E402


def _latencies(session: Path, topic: str, *, capture: bool) -> List[float]:
    out: List[float] = []
    field = "capture_ts_ms" if capture else "vision_ts_ms"
    for _, payload, zmq in load_topic_messages(session, topic):
        if payload is None:
            continue
        start = int(getattr(payload, field, 0) or 0)
        publish = int(getattr(payload, "publish_ts_ms", 0) or 0)
        if publish <= 0:
            publish = int(getattr(payload, "timestamp", 0) or 0) or int(zmq.timestamp)
        if start <= 0 or publish <= 0:
            continue
        dt = publish - start
        if dt < 0 or dt > 5000:  # ignore clock glitches / stale >5s
            continue
        out.append(float(dt))
    return out


def summarize(name: str, xs: List[float]) -> None:
    if not xs:
        print(f"{name}: no samples (rebuild app / new bag with stamps?)")
        return
    xs_sorted = sorted(xs)
    p95 = xs_sorted[int(0.95 * (len(xs_sorted) - 1))]
    print(
        f"{name}: n={len(xs)}  median={statistics.median(xs):.1f} ms  "
        f"mean={statistics.mean(xs):.1f} ms  p95={p95:.1f} ms  max={max(xs):.1f} ms"
    )


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("session", type=Path)
    args = ap.parse_args()
    session = args.session.resolve()

    print(f"session: {session}")
    print("latency_capture = publish − capture  (camera → control publish)")
    print("latency_vision  = publish − vision   (ONNX done → control publish)")
    print()
    for topic in ("controls/steer", "control/lane_keep"):
        summarize(f"{topic} capture→publish", _latencies(session, topic, capture=True))
        summarize(f"{topic} vision→publish", _latencies(session, topic, capture=False))
        print()


if __name__ == "__main__":
    main()
