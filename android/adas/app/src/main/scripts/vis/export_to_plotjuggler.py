#!/usr/bin/env python3
"""ADAS bag → CSV for PlotJuggler (Android session layout).

Modeled after atom/replay/scripts/analyze_bag.py: one row per message with
hierarchical columns (vehicle/…, sensors/imu/…, panda/…, gps/…, vision/…).

Usage:
  python3 export_to_plotjuggler.py /path/to/adas_bags/2026_07_18_09_45_15 -o /tmp/out
  # writes /tmp/out/2026_07_18_09_45_15.csv
  plotjuggler /tmp/out/2026_07_18_09_45_15.csv
"""

from __future__ import annotations

import _path  # noqa: F401

import argparse
import csv
import math
import statistics
import sys
from pathlib import Path
from typing import Any, Callable, Optional

from vis.android_bag_player import AndroidBagPlayer

SOURCE_ORDER = (
    "vehicle",
    "sensors",
    "panda",
    "gps",
    "vision",
    "camera",
    "can",
)


def vals(prefix: str, **fields: Any) -> dict:
    return {f"{prefix}/{k}": v for k, v in fields.items() if v is not None}


class Collector:
    def __init__(self) -> None:
        self.wall_t0_ms: Optional[int] = None
        self.data_t0_ms: Optional[int] = None
        self.rows: list[dict] = []
        self._prev_wall: dict[str, int] = {}
        self._prev_data: dict[str, int] = {}
        self.counts: dict[str, int] = {}
        self.lags_ms: dict[str, list[int]] = {}
        self.zmq_intervals_ms: dict[str, list[int]] = {}
        self.intervals_ms: dict[str, list[int]] = {}

    def add(
        self,
        source: str,
        stream: str,
        prefix: str,
        wall_ms: int,
        data_ms: int,
        extra: dict,
    ) -> None:
        if self.wall_t0_ms is None:
            self.wall_t0_ms = wall_ms
        if self.data_t0_ms is None:
            self.data_t0_ms = data_ms
        stat_key = f"{source}.{stream}"
        self.counts[stat_key] = self.counts.get(stat_key, 0) + 1

        lag_ms = wall_ms - data_ms
        self.lags_ms.setdefault(stat_key, []).append(lag_ms)

        row = {
            "timestamp": (data_ms - self.data_t0_ms) / 1000.0,
            f"{prefix}/zmq_timestamp": (wall_ms - self.wall_t0_ms) / 1000.0,
            f"{prefix}/lag_ms": lag_ms,
        }
        prev_w = self._prev_wall.get(prefix)
        prev_d = self._prev_data.get(prefix)
        if prev_w is not None:
            zmq_interval_ms = wall_ms - prev_w
            row[f"{prefix}/zmq_interval_ms"] = zmq_interval_ms
            self.zmq_intervals_ms.setdefault(stat_key, []).append(zmq_interval_ms)
        if prev_d is not None and data_ms != prev_d:
            interval_ms = data_ms - prev_d
            row[f"{prefix}/interval_ms"] = interval_ms
            self.intervals_ms.setdefault(stat_key, []).append(interval_ms)
        self._prev_wall[prefix] = wall_ms
        self._prev_data[prefix] = data_ms
        row.update(extra)
        self.rows.append(row)

    def print_summary(self) -> None:
        by_source: dict[str, list[tuple[str, int]]] = {}
        for key, count in self.counts.items():
            source, stream = key.split(".", 1)
            by_source.setdefault(source, []).append((stream, count))
        for source in SOURCE_ORDER:
            if source not in by_source:
                continue
            items = ", ".join(f"{s}:{c}" for s, c in sorted(by_source[source]))
            print(f"[{source}] {items}")
        for source in sorted(by_source):
            if source in SOURCE_ORDER:
                continue
            items = ", ".join(f"{s}:{c}" for s, c in sorted(by_source[source]))
            print(f"[{source}] {items}")
        print(f"total rows: {len(self.rows)}")

    def _stat_sort_key(self, key: str) -> tuple:
        source = key.split(".", 1)[0]
        try:
            source_idx = SOURCE_ORDER.index(source)
        except ValueError:
            source_idx = len(SOURCE_ORDER)
        return source_idx, key

    def print_stats_array(self, name: str, values: list[int]) -> None:
        if not values:
            return
        print(f"{name}:")
        print(f"  min={min(values):.1f} ms")
        print(f"  max={max(values):.1f} ms")
        print(f"  median={statistics.median(values):.1f} ms")
        print(f"  mean={statistics.mean(values):.1f} ms")
        if len(values) >= 2:
            print(f"  stdev={statistics.stdev(values):.1f} ms")

    def print_stats(self) -> None:
        keys = sorted(
            set(self.lags_ms) | set(self.zmq_intervals_ms) | set(self.intervals_ms),
            key=self._stat_sort_key,
        )
        for key in keys:
            if key in self.lags_ms:
                self.print_stats_array(f"{key} lags", self.lags_ms[key])
            if key in self.zmq_intervals_ms:
                self.print_stats_array(f"{key} zmq_intervals", self.zmq_intervals_ms[key])
            if key in self.intervals_ms:
                self.print_stats_array(f"{key} intervals", self.intervals_ms[key])

    def _column_rank(self, col: str) -> tuple:
        if col == "timestamp":
            return (0, 0, col)
        top = col.split("/", 1)[0]
        try:
            source_idx = SOURCE_ORDER.index(top)
        except ValueError:
            source_idx = len(SOURCE_ORDER)
        return (1, source_idx, col)

    def write_csv(self, path: Path) -> None:
        self.rows.sort(key=lambda r: r["timestamp"])
        fields = ["timestamp"]
        for row in self.rows:
            fields.extend(k for k in row if k not in fields)
        fields[1:] = sorted(fields[1:], key=self._column_rank)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
            w.writeheader()
            w.writerows(self.rows)
        print(path)


def _data_ms(msg: Any, wall_ms: int) -> int:
    ts = int(getattr(msg, "timestamp", 0) or 0)
    return ts if ts > 0 else wall_ms


def process_vehicle(m: Any, wall_ms: int) -> tuple[int, dict]:
    p = "vehicle"
    ws = m.wheel_speeds
    return _data_ms(m, wall_ms), vals(
        p,
        v_ego=m.v_ego,
        v_ego_raw=m.v_ego_raw,
        a_ego=m.a_ego,
        standstill=int(m.standstill),
        wheel_fl=ws.fl,
        wheel_fr=ws.fr,
        wheel_rl=ws.rl,
        wheel_rr=ws.rr,
        steering_angle_deg=m.steering_angle_deg,
        steering_rate_deg=m.steering_rate_deg,
        steering_torque=m.steering_torque,
        steering_pressed=int(m.steering_pressed),
        yaw_rate=m.yaw_rate,
        gas=m.gas,
        gas_pressed=int(m.gas_pressed),
        brake=m.brake,
        brake_pressed=int(m.brake_pressed),
        gear=m.gear,
        cruise_main=int(m.cruise_main_switch),
        cruise_set=int(m.cruise_set),
        cruise_resume=int(m.cruise_resume),
        cruise_cancel=int(m.cruise_cancel),
        cruise_accel=int(m.cruise_accel),
        cruise_decel=int(m.cruise_decel),
        cruise_gap=m.cruise_gap_adjust,
        acc_status=m.acc_status,
        cruise_available=int(m.cruise_available),
        cruise_engaged=int(m.cruise_engaged),
    )


def process_imu(m: Any, wall_ms: int) -> tuple[int, dict]:
    return _data_ms(m, wall_ms), vals(
        "sensors/imu",
        accel_x=m.accel_x,
        accel_y=m.accel_y,
        accel_z=m.accel_z,
        gyro_x=m.gyro_x,
        gyro_y=m.gyro_y,
        gyro_z=m.gyro_z,
        mag_x=m.mag_x,
        mag_y=m.mag_y,
        mag_z=m.mag_z,
        accuracy=m.accuracy,
        temperature=m.temperature,
    )


def process_panda(m: Any, wall_ms: int) -> tuple[int, dict]:
    return _data_ms(m, wall_ms), vals(
        "panda",
        controls_allowed=int(m.controls_allowed),
        safety_mode=m.safety_mode,
        safety_param=m.safety_param,
        voltage_mv=m.voltage_mv,
        current_ma=m.current_ma,
        tx_blocked=m.tx_blocked,
        heartbeat_lost=int(m.heartbeat_lost),
        ignition_line=int(m.ignition_line),
        ignition_can=int(m.ignition_can),
        power_save=int(m.power_save_enabled),
        alt_exp=m.alternative_experience,
        fault_status=m.fault_status,
        faults=m.faults_pkt,
    )


def process_gps_location(m: Any, wall_ms: int, origin: list) -> tuple[int, dict]:
    """origin: mutable [lat0, lon0] set on first fix."""
    lat, lon = float(m.latitude), float(m.longitude)
    if origin[0] is None:
        origin[0], origin[1] = lat, lon
    # local ENU metres (approx)
    r = 6371000.0
    dlat = math.radians(lat - origin[0])
    dlon = math.radians(lon - origin[1])
    x = dlon * math.cos(math.radians(origin[0])) * r
    y = dlat * r
    return _data_ms(m, wall_ms), vals(
        "gps/location",
        lat=lat,
        lon=lon,
        alt=m.altitude,
        speed=m.speed,
        bearing=m.bearing,
        h_acc=m.horizontal_accuracy,
        v_acc=m.vertical_accuracy,
        sats=m.satellites_used,
        hdop=m.hdop,
        vdop=m.vdop,
        fix=int(m.fix_type),
        x=x,
        y=y,
    )


def process_gps_data(m: Any, wall_ms: int) -> tuple[int, dict]:
    return _data_ms(m, wall_ms), vals(
        "gps/data",
        lat=m.latitude,
        lon=m.longitude,
        alt=m.altitude,
        speed=m.speed,
        bearing=m.bearing,
    )


def _lane_y_at(lane: Any, x_pts: list, x_query: float) -> Optional[float]:
    ys = list(lane.y)
    if not ys or not x_pts or len(ys) != len(x_pts):
        return None
    # nearest sample
    best_i = min(range(len(x_pts)), key=lambda i: abs(x_pts[i] - x_query))
    return float(ys[best_i])


def process_lanes(m: Any, wall_ms: int) -> tuple[int, dict]:
    x_pts = list(m.x)
    names = ("left_far", "left_near", "right_near", "right_far")
    out: dict = {"vision/lanes/frame_id": m.frame_id, "vision/lanes/n_x": len(x_pts)}
    for i, name in enumerate(names):
        if i >= len(m.lanes):
            break
        lane = m.lanes[i]
        out[f"vision/lanes/{name}/prob"] = lane.prob
        for xq, tag in ((10.0, "y10"), (20.0, "y20"), (30.0, "y30")):
            y = _lane_y_at(lane, x_pts, xq)
            if y is not None:
                out[f"vision/lanes/{name}/{tag}"] = y
    # mid path from near lanes
    if len(m.lanes) >= 3:
        yl = _lane_y_at(m.lanes[1], x_pts, 20.0)
        yr = _lane_y_at(m.lanes[2], x_pts, 20.0)
        if yl is not None and yr is not None:
            out["vision/lanes/mid_y20"] = 0.5 * (yl + yr)
    return _data_ms(m, wall_ms), out


def process_intrinsics(m: Any, wall_ms: int) -> tuple[int, dict]:
    fx = fy = cx = cy = None
    if len(m.intrinsic_calibration) >= 4:
        fx, fy, cx, cy = m.intrinsic_calibration[:4]
    return _data_ms(m, wall_ms), vals(
        "camera/intrinsics",
        fx=fx,
        fy=fy,
        cx=cx,
        cy=cy,
        focal_px=m.focal_length_px,
        width=m.capture_width,
        height=m.capture_height,
    )


def process_can(m: Any, wall_ms: int) -> tuple[int, dict]:
    return _data_ms(m, wall_ms), vals("can/rx", n_frames=len(m.frames))


STREAMS: list[tuple[str, str, str, str, Callable]] = [
    # source, stream, topic, prefix, process(msg, wall_ms) -> (data_ms, extra)
    ("vehicle", "state", "vehicle/state", "vehicle", process_vehicle),
    ("sensors", "imu", "sensors/imu", "sensors/imu", process_imu),
    ("panda", "health", "panda/health", "panda", process_panda),
    ("gps", "data", "sensors/gps/data", "gps/data", process_gps_data),
    ("vision", "lanes", "vision/lanes", "vision/lanes", process_lanes),
    (
        "camera",
        "intrinsics",
        "camera/intrinsics",
        "camera/intrinsics",
        process_intrinsics,
    ),
    ("can", "rx", "can/rx", "can/rx", process_can),
]


def collect(player: AndroidBagPlayer, collector: Collector) -> None:
    gps_origin: list = [None, None]

    for source, stream, topic, prefix, process in STREAMS:
        if topic not in player.topics:
            continue
        for wall_ms, msg in player.single_type_generator_with_ts(topic):
            if msg is None:
                continue
            data_ms, extra = process(msg, wall_ms)
            collector.add(source, stream, prefix, wall_ms, data_ms, extra)

    topic = "sensors/gps/location"
    if topic in player.topics:
        for wall_ms, msg in player.single_type_generator_with_ts(topic):
            if msg is None:
                continue
            data_ms, extra = process_gps_location(msg, wall_ms, gps_origin)
            collector.add("gps", "location", "gps/location", wall_ms, data_ms, extra)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("bag", type=Path, help="ADAS bag session dir (or .zip/.tar.gz)")
    ap.add_argument("-o", type=Path, required=True, help="output directory for CSV")
    args = ap.parse_args()

    bag = args.bag.resolve()
    if not bag.exists():
        print(f"Not found: {bag}", file=sys.stderr)
        return 1

    with AndroidBagPlayer(bag, quiet=True) as player:
        if not player.topics:
            print(f"No topics in {bag}", file=sys.stderr)
            return 1
        collector = Collector()
        collect(player, collector)
        if not collector.rows:
            print(f"No rows exported from {bag}", file=sys.stderr)
            return 1
        collector.print_summary()
        collector.print_stats()
        out = args.o.resolve() / f"{player.session_dir.name}.csv"
        collector.write_csv(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
