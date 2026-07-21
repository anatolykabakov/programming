#!/usr/bin/env python3
"""Interactive ADAS bag visualizer (Android session layout).

Uses AndroidBagPlayer + extractors from visualizer.py.

UI:
  - trajectory (odom / GPS / IMU / EKF)
  - synced camera frame + live supercombo.onnx overlay (plan/lanes/edges)
  - lane source toggle: runtime ONNX vs bag vision/lanes
  - timeline scrub + play/pause
  - live IMU / GPS / odometry readout

Usage:
  python3 vis/interactive_visualizer.py /path/to/2026_07_18_09_45_15
  python3 vis/interactive_visualizer.py -i /path/to/session.zip
  # or from scripts/: python3 interactive_visualizer.py … (shim)
"""

from __future__ import annotations

import _path  # noqa: F401

import argparse
import json
from pathlib import Path
from typing import Any, List, Optional, Tuple

import cv2
import numpy as np
import matplotlib

matplotlib.use("TkAgg")
import matplotlib.pyplot as plt  # noqa: F401
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from vis.android_bag_player import AndroidBagPlayer
from core.gps_utils import calculate_initial_heading_from_gps, gps_to_local_coords
from core.imu_utils import process_imu_for_odometry
from core.lane_projection import (
    CameraIntrinsics,
    intrinsics_from_messages,
)
from core.supercombo_compare import (
    SupercomboBev,
    draw_bag_lanes,
    draw_supercombo_overlay,
    make_overlay_geometry,
)
from core.supercombo_parse import explain_output
from vis.trajectory_calculators import (
    calculate_trajectory,
    calculate_trajectory_ekf,
    calculate_trajectory_imu,
)
from core.lane_keep import (
    DEFAULTS,
    LaneKeepController,
    format_lane_keep_status,
)
from core.lane_keep_viz import draw_lane_keep_overlay
from core.pure_pursuit import plan_to_polyline_ego
from core.viz_params_ui import OverlayUiParams, RpyPpControlBar
from core.vanishing_point_calib import (
    K_from_fx_fy_cx_cy,
    VanishingPointCalibrator,
    draw_vp_debug,
    get_intersection,
    lines_from_image_hough,
)
from vis.visualizer import (
    _UNIT_TO_DEG,
    extract_gps,
    extract_imu,
    extract_vehicle_series,
)


def _nearest_row(series: Optional[np.ndarray], t_ms: float) -> Optional[np.ndarray]:
    """Nearest row in series[:,0] timestamps (ms)."""
    if series is None or len(series) == 0:
        return None
    ts = series[:, 0]
    idx = int(np.searchsorted(ts, t_ms))
    if idx <= 0:
        return series[0]
    if idx >= len(ts):
        return series[-1]
    if abs(ts[idx - 1] - t_ms) <= abs(ts[idx] - t_ms):
        return series[idx - 1]
    return series[idx]


def _format_ms(ms: float) -> str:
    seconds = max(0.0, float(ms) / 1000.0)
    hours = int(seconds // 3600)
    minutes = int((seconds % 3600) // 60)
    secs = int(seconds % 60)
    return f"{hours:02d}:{minutes:02d}:{secs:02d}"


class InteractiveVisualizer:
    """Interactive trajectory + camera viewer for ADAS bag sessions."""

    def __init__(self, master: tk.Tk, bag_path: Optional[str] = None):
        self.master = master
        self.master.title("ADAS Interactive Visualizer")
        self.master.geometry("1400x900")

        self.player: Optional[AndroidBagPlayer] = None
        self.trajectories = {}
        self.timestamps = np.array([], dtype=np.float64)
        self.current_index = 0

        self.imu_data: Optional[np.ndarray] = None
        self.gps_data: Optional[np.ndarray] = None
        self.wheel_data: Optional[np.ndarray] = None
        self.steering_data: Optional[np.ndarray] = None
        self.gear_data: Optional[np.ndarray] = None
        # (timestamp_ms, jpeg_bytes, optional cam_msg for K)
        self.camera_frames: List[Tuple[int, bytes, Any]] = []
        # (timestamp_ms, LaneLines proto from vision/lanes)
        self.bag_lane_frames: List[Tuple[int, Any]] = []
        self.image_timestamps = np.array([], dtype=np.int64)
        self.lane_timestamps = np.array([], dtype=np.int64)
        self.intrinsics: Optional[CameraIntrinsics] = None
        self.intr_msg = None
        self.supercombo = SupercomboBev()
        self.bag_dir: Optional[Path] = None
        # AAD vanishing-point pitch/yaw (roll=0); prior ≈ Golf windshield — C++ CameraCalibService
        self.vp_calib = VanishingPointCalibrator(
            history_len=50,
            estimated_pitch_deg=-6.0,
            estimated_yaw_deg=0.0,
            camera_height_m=1.40,
        )
        self._vp_last_img_index: Optional[int] = None
        self._play_last_wall_ms: Optional[float] = None
        self._overlay_pitch_deg = -6.0
        self._overlay_yaw_deg = 0.0
        self._overlay_roll_deg = 0.0
        self._overlay_height_m = 1.40

        self.playing = False
        self.play_speed = 1.0

        self.create_ui()
        if bag_path:
            self.load_bag(bag_path)

    def create_ui(self) -> None:
        control = ttk.Frame(self.master)
        control.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        ttk.Button(control, text="Open bag", command=self.open_bag).pack(side=tk.LEFT, padx=5)
        ttk.Button(control, text="Play", command=self.play).pack(side=tk.LEFT, padx=5)
        ttk.Button(control, text="Pause", command=self.pause).pack(side=tk.LEFT, padx=5)
        ttk.Button(control, text="Restart", command=self.restart).pack(side=tk.LEFT, padx=5)

        self.supercombo_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            control,
            text="Supercombo overlay",
            variable=self.supercombo_var,
            command=lambda: self.update_display(self.current_index),
        ).pack(side=tk.LEFT, padx=10)

        lane_src = ttk.LabelFrame(control, text="Lanes", padding=(4, 0))
        lane_src.pack(side=tk.LEFT, padx=(4, 8))
        self.lane_source_var = tk.StringVar(value="runtime")
        ttk.Radiobutton(
            lane_src,
            text="Runtime",
            value="runtime",
            variable=self.lane_source_var,
            command=lambda: self.update_display(self.current_index),
        ).pack(side=tk.LEFT, padx=2)
        self.bag_lanes_radio = ttk.Radiobutton(
            lane_src,
            text="Bag",
            value="bag",
            variable=self.lane_source_var,
            command=lambda: self.update_display(self.current_index),
        )
        self.bag_lanes_radio.pack(side=tk.LEFT, padx=2)

        self.vp_calib_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            control,
            text="VP calib (AAD)",
            variable=self.vp_calib_var,
            command=lambda: self.update_display(self.current_index),
        ).pack(side=tk.LEFT, padx=6)
        self.vp_debug_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            control,
            text="VP debug",
            variable=self.vp_debug_var,
            command=lambda: self.update_display(self.current_index),
        ).pack(side=tk.LEFT, padx=4)
        self.pp_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            control,
            text="Pure pursuit",
            variable=self.pp_var,
            command=lambda: self.update_display(self.current_index),
        ).pack(side=tk.LEFT, padx=6)
        ttk.Button(
            control, text="Reset VP", command=lambda: self.reset_vp_calib(load_saved=False)
        ).pack(side=tk.LEFT, padx=4)
        self.vp_status = ttk.Label(control, text="RPY 0 / -6 / 0°")
        self.vp_status.pack(side=tk.LEFT, padx=8)

        ttk.Label(control, text="Speed:").pack(side=tk.LEFT, padx=(20, 5))
        self.speed_var = tk.DoubleVar(value=1.0)
        ttk.Scale(
            control,
            from_=0.1,
            to=5.0,
            orient=tk.HORIZONTAL,
            variable=self.speed_var,
            length=100,
        ).pack(side=tk.LEFT, padx=5)
        self.speed_label = ttk.Label(control, text="1.0x")
        self.speed_label.pack(side=tk.LEFT, padx=5)
        self.speed_var.trace_add("write", self.on_speed_change)

        self.status_label = ttk.Label(control, text="Ready", foreground="green")
        self.status_label.pack(side=tk.RIGHT, padx=10)

        # RPY / PP — shared with MetaDrive sim
        initial = OverlayUiParams(
            roll_deg=0.0,
            pitch_deg=-6.0,
            yaw_deg=0.0,
            height_m=1.40,
            pp_k_dd=0.4,
            pp_ld_min=3.0,
            pp_ld_max=20.0,
            wheelbase=DEFAULTS.wheelbase,
            pp_shift=DEFAULTS.pp_shift,
        )
        self.params_bar = RpyPpControlBar(
            self.master,
            initial,
            on_rpy=self._on_rpy_params,
            on_pp=self._on_pp_params,
        )
        self.roll_var = self.params_bar.roll_var
        self.pitch_var = self.params_bar.pitch_var
        self.yaw_var = self.params_bar.yaw_var
        self.height_var = self.params_bar.height_var
        self.pp_kdd_var = self.params_bar.pp_kdd_var
        self.pp_ld_min_var = self.params_bar.pp_ld_min_var
        self.pp_ld_max_var = self.params_bar.pp_ld_max_var
        self.pp_wb_var = self.params_bar.pp_wb_var
        self.pp_shift_var = self.params_bar.pp_shift_var
        self.pp_status = self.params_bar.pp_status
        self._pp_controller = LaneKeepController(mode="pure_pursuit")

        main = ttk.Frame(self.master)
        main.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        left = ttk.Frame(main)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        right = ttk.Frame(main)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self.fig_trajectory = Figure(figsize=(6, 6), dpi=100)
        self.ax_trajectory = self.fig_trajectory.add_subplot(111)
        self.ax_trajectory.set_title("Vehicle Trajectory")
        self.ax_trajectory.set_xlabel("X (meters)")
        self.ax_trajectory.set_ylabel("Y (meters)")
        self.ax_trajectory.grid(True, alpha=0.3)
        self.ax_trajectory.set_aspect("equal")
        self.canvas_trajectory = FigureCanvasTkAgg(self.fig_trajectory, left)
        self.canvas_trajectory.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        (self.current_pos_marker,) = self.ax_trajectory.plot([], [], "ro", markersize=10)

        ttk.Label(right, text="Camera", font=("Arial", 14, "bold")).pack(pady=5)
        self.camera_canvas = tk.Canvas(right, bg="black", width=640, height=480)
        self.camera_canvas.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        timeline = ttk.LabelFrame(self.master, text="Timeline", padding=10)
        timeline.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        slider_row = ttk.Frame(timeline)
        slider_row.pack(fill=tk.X)
        self.time_var = tk.IntVar(value=0)
        self.timeline_slider = ttk.Scale(
            slider_row,
            from_=0,
            to=100,
            orient=tk.HORIZONTAL,
            variable=self.time_var,
            command=self.on_timeline_change,
        )
        self.timeline_slider.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        self.time_label = ttk.Label(slider_row, text="00:00:00 / 00:00:00", font=("Courier", 10))
        self.time_label.pack(side=tk.RIGHT, padx=10)

        sensors = ttk.LabelFrame(self.master, text="Sensors", padding=10)
        sensors.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)
        imu_f = ttk.Frame(sensors)
        imu_f.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=10)
        gps_f = ttk.Frame(sensors)
        gps_f.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=10)
        odom_f = ttk.Frame(sensors)
        odom_f.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=10)

        ttk.Label(imu_f, text="IMU", font=("Arial", 11, "bold")).pack(anchor=tk.W)
        self.imu_text = tk.Text(imu_f, height=4, width=35, font=("Courier", 9))
        self.imu_text.pack(fill=tk.BOTH, expand=True)
        self.imu_text.config(state=tk.DISABLED)

        ttk.Label(gps_f, text="GPS", font=("Arial", 11, "bold")).pack(anchor=tk.W)
        self.gps_text = tk.Text(gps_f, height=4, width=35, font=("Courier", 9))
        self.gps_text.pack(fill=tk.BOTH, expand=True)
        self.gps_text.config(state=tk.DISABLED)

        ttk.Label(odom_f, text="Odometry", font=("Arial", 11, "bold")).pack(anchor=tk.W)
        self.odom_text = tk.Text(odom_f, height=4, width=35, font=("Courier", 9))
        self.odom_text.pack(fill=tk.BOTH, expand=True)
        self.odom_text.config(state=tk.DISABLED)

        self.master.protocol("WM_DELETE_WINDOW", self.on_close)

    def open_bag(self) -> None:
        path = filedialog.askdirectory(title="Select ADAS bag session directory")
        if not path:
            path = filedialog.askopenfilename(
                title="Or select .zip / .tar.gz",
                filetypes=[
                    ("Archives", "*.zip *.tar.gz *.tgz"),
                    ("All", "*.*"),
                ],
            )
        if path:
            self.load_bag(path)

    def on_close(self) -> None:
        self.playing = False
        if self.player is not None:
            self.player.close()
            self.player = None
        self.master.destroy()

    def load_bag(self, bag_path: str) -> None:
        try:
            self.status_label.config(text="Loading…", foreground="orange")
            self.master.update()
            self.playing = False

            if self.player is not None:
                self.player.close()

            print(f"Loading bag: {bag_path}")
            self.player = AndroidBagPlayer(bag_path)
            self.bag_dir = Path(bag_path)
            if self.bag_dir.is_file():
                self.bag_dir = self.bag_dir.parent

            wheel, steering, gear = extract_vehicle_series(self.player)
            self.wheel_data = np.asarray(wheel, dtype=np.float64) if wheel else None
            self.steering_data = np.asarray(steering, dtype=np.float64) if steering else None
            # gear has mixed types (str name) — keep as object array / list
            self.gear_data = np.array(gear, dtype=object) if gear else None
            self.imu_data = extract_imu(self.player)
            self.gps_data = extract_gps(self.player)
            self._load_camera_frames()
            self._load_bag_lanes()
            self._load_intrinsics()
            self.supercombo.reset()
            self._update_lane_source_ui()
            self.reset_vp_calib(load_saved=True)
            print(explain_output())

            self.trajectories = {}
            self.calculate_trajectories()
            self.plot_trajectories()

            if self.wheel_data is not None and len(self.wheel_data) > 0:
                self.timestamps = self.wheel_data[:, 0]
                self.timeline_slider.config(to=max(0, len(self.timestamps) - 1))
                self.time_var.set(0)
            elif self.camera_frames:
                self.timestamps = self.image_timestamps.astype(np.float64)
                self.timeline_slider.config(to=max(0, len(self.timestamps) - 1))
                self.time_var.set(0)
            else:
                self.timestamps = np.array([], dtype=np.float64)
                messagebox.showwarning(
                    "No vehicle/state",
                    "Bag has no vehicle/state — trajectory/odometry unavailable.\n"
                    "Camera/GPS/IMU still load if present.",
                )

            self.update_display(0)
            n_img = len(self.camera_frames)
            n_imu = 0 if self.imu_data is None else len(self.imu_data)
            n_gps = 0 if self.gps_data is None else len(self.gps_data)
            n_wh = 0 if self.wheel_data is None else len(self.wheel_data)
            n_lanes = len(self.bag_lane_frames)
            self.status_label.config(
                text=f"Loaded: wheel={n_wh} imu={n_imu} gps={n_gps} cam={n_img} bag_lanes={n_lanes}",
                foreground="green",
            )
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load bag:\n{e}")
            self.status_label.config(text=f"Error: {e}", foreground="red")
            import traceback

            traceback.print_exc()

    def _load_camera_frames(self) -> None:
        self.camera_frames = []
        if self.player is None or "sensors/camera/image" not in self.player.topics:
            self.image_timestamps = np.array([], dtype=np.int64)
            return
        for ts, msg in self.player.get_topic_msgs("sensors/camera/image"):
            data = getattr(msg, "image_data", b"") or b""
            if data:
                self.camera_frames.append((int(ts), data, msg))
        self.image_timestamps = np.array([t for t, _, _ in self.camera_frames], dtype=np.int64)
        print(f"Camera frames: {len(self.camera_frames)}")

    def _load_bag_lanes(self) -> None:
        self.bag_lane_frames = []
        if self.player is None or "vision/lanes" not in self.player.topics:
            self.lane_timestamps = np.array([], dtype=np.int64)
            return
        for ts, msg in self.player.get_topic_msgs("vision/lanes"):
            self.bag_lane_frames.append((int(ts), msg))
        self.lane_timestamps = np.array([t for t, _ in self.bag_lane_frames], dtype=np.int64)
        print(f"Bag lane messages: {len(self.bag_lane_frames)}")

    def _update_lane_source_ui(self) -> None:
        has_bag_lanes = len(self.bag_lane_frames) > 0
        if has_bag_lanes:
            self.bag_lanes_radio.config(state=tk.NORMAL)
            return
        self.bag_lanes_radio.config(state=tk.DISABLED)
        if self.lane_source_var.get() == "bag":
            self.lane_source_var.set("runtime")

    def _find_bag_lanes_by_time(self, target_time: float) -> Optional[Any]:
        """Latest vision/lanes message with ts <= target camera/state time."""
        if len(self.lane_timestamps) == 0:
            return None
        idx = int(np.searchsorted(self.lane_timestamps, target_time, side="right")) - 1
        if idx < 0:
            idx = 0
        return self.bag_lane_frames[idx][1]

    def _load_intrinsics(self) -> None:
        self.intr_msg = None
        if self.player is None:
            return

        if "camera/intrinsics" in self.player.topics:
            msgs = self.player.get_topic_msgs("camera/intrinsics")
            if msgs:
                self.intr_msg = msgs[0][1]

        cam0 = self.camera_frames[0][2] if self.camera_frames else None
        self.intrinsics = intrinsics_from_messages(cam0, self.intr_msg)
        print(
            f"Intrinsics: fx={self.intrinsics.fx:.1f} fy={self.intrinsics.fy:.1f} "
            f"cx={self.intrinsics.cx:.1f} cy={self.intrinsics.cy:.1f} "
            f"{self.intrinsics.width}x{self.intrinsics.height}"
        )

    def calculate_trajectories(self) -> None:
        print("\nCalculating trajectories…")
        if self.wheel_data is None or len(self.wheel_data) == 0:
            print("No wheel data")
            return

        initial_yaw = 0.0
        if self.gps_data is not None and len(self.gps_data) >= 2:
            initial_yaw = calculate_initial_heading_from_gps(self.gps_data, n_points=5)
            print(f"Initial GPS heading: {np.degrees(initial_yaw):.1f}°")

        if self.gps_data is not None and len(self.gps_data) > 0:
            x_gps, y_gps = gps_to_local_coords(self.gps_data, origin_idx=0)
            self.trajectories["GPS"] = (x_gps, y_gps)

        wheel = self.wheel_data
        steering = self.steering_data
        gear = self.gear_data

        x_odom, y_odom = calculate_trajectory(
            wheel, steering, gear, wheelbase=2.636, initial_yaw=initial_yaw
        )
        self.trajectories["Odometry"] = (x_odom, y_odom)
        print(f"Odometry: {len(x_odom)} pts")

        if self.imu_data is not None and len(self.imu_data) > 0:
            try:
                imu_processed = process_imu_for_odometry(
                    self.imu_data,
                    wheel,
                    speed_threshold_orientation=0.1,
                    speed_threshold_bias=0.5,
                    time_window_sec=20.0,
                    invert_yaw_rate=True,
                )
                x_imu, y_imu = calculate_trajectory_imu(
                    wheel,
                    imu_processed["yaw_rate"],
                    imu_processed["imu_calibrated"][:, 0],
                    initial_yaw=initial_yaw,
                )
                self.trajectories["IMU"] = (x_imu, y_imu)
                print(f"IMU: {len(x_imu)} pts")

                if self.gps_data is not None and len(self.gps_data) > 0:
                    x_ekf, y_ekf, _ = calculate_trajectory_ekf(
                        wheel,
                        steering,
                        gear,
                        imu_processed["yaw_rate"],
                        imu_processed["imu_calibrated"][:, 0],
                        self.gps_data,
                        wheelbase=2.636,
                        initial_yaw=initial_yaw,
                    )
                    self.trajectories["EKF"] = (x_ekf, y_ekf)
                    print(f"EKF: {len(x_ekf)} pts")
            except Exception as e:
                print(f"IMU/EKF failed: {e}")
                import traceback

                traceback.print_exc()

    def plot_trajectories(self) -> None:
        self.ax_trajectory.clear()
        self.ax_trajectory.set_title("Vehicle Trajectory", fontsize=14, fontweight="bold")
        self.ax_trajectory.set_xlabel("X (meters)")
        self.ax_trajectory.set_ylabel("Y (meters)")
        self.ax_trajectory.grid(True, alpha=0.3, linestyle="--")
        self.ax_trajectory.set_aspect("equal")

        styles = {
            "Odometry": {"color": "blue", "linestyle": "-", "linewidth": 2, "alpha": 0.7},
            "GPS": {"color": "red", "linestyle": "--", "linewidth": 2, "alpha": 0.7},
            "IMU": {"color": "green", "linestyle": "-.", "linewidth": 1.5, "alpha": 0.6},
            "EKF": {"color": "orange", "linestyle": "-", "linewidth": 2.5, "alpha": 0.9},
        }
        for name, (x, y) in self.trajectories.items():
            if x is not None and len(x) > 0:
                self.ax_trajectory.plot(x, y, label=name, **styles.get(name, {}))

        (self.current_pos_marker,) = self.ax_trajectory.plot(
            [],
            [],
            "ro",
            markersize=12,
            label="Current",
            zorder=10,
            markeredgecolor="white",
            markeredgewidth=2,
        )
        if self.trajectories:
            self.ax_trajectory.legend(loc="upper right", fontsize=10, framealpha=0.9)
        self.canvas_trajectory.draw()

    def update_display(self, index: int) -> None:
        if len(self.timestamps) == 0:
            self.update_camera_view(0)
            return

        self.current_index = int(np.clip(index, 0, len(self.timestamps) - 1))
        traj = self.trajectories.get("EKF") or self.trajectories.get("Odometry")
        if traj is not None:
            x, y = traj
            if self.current_index < len(x):
                self.current_pos_marker.set_data([x[self.current_index]], [y[self.current_index]])
                self.canvas_trajectory.draw_idle()

        self.update_camera_view(self.current_index)
        self.update_sensor_data(self.current_index)
        self.update_time_label(self.current_index)

    def _on_pp_params(self, p: OverlayUiParams) -> None:
        self.params_bar.set_pp_status(
            f"K={p.pp_k_dd:.2f} Ld=[{p.pp_ld_min:.1f},{p.pp_ld_max:.1f}] shift={p.pp_shift:.2f}"
        )
        if not self.playing:
            self.update_camera_view(self.current_index)

    def _on_pp_slider(self) -> None:
        self._on_pp_params(self.params_bar.params())

    def _reset_pp_sliders(self) -> None:
        self.params_bar.reset_pp()

    def _sync_pp_controller(self) -> None:
        """Refresh shared LaneKeepController from UI sliders."""
        p = self.params_bar.params()
        self._pp_controller = LaneKeepController(
            mode="pure_pursuit",
            wheelbase=p.wheelbase,
            pp_k_dd=p.pp_k_dd,
            pp_ld_min=p.pp_ld_min,
            pp_ld_max=p.pp_ld_max,
            pp_shift=p.pp_shift,
        )

    def _ego_speed_mps(self, index: int) -> float:
        """Best-effort ego speed (m/s) from wheels or GPS."""
        if self.wheel_data is not None and index < len(self.wheel_data):
            wh = self.wheel_data[index]
            # km/h average of four wheels
            v_kmh = float(np.mean(wh[1:5]))
            return max(0.0, v_kmh / 3.6)
        if len(self.timestamps) > 0 and index < len(self.timestamps):
            gps = _nearest_row(self.gps_data, float(self.timestamps[index]))
            if gps is not None:
                return max(0.0, float(gps[4]))
        return 0.0

    def _on_rpy_params(self, p: OverlayUiParams) -> None:
        self._overlay_roll_deg = p.roll_deg
        self._overlay_pitch_deg = p.pitch_deg
        self._overlay_yaw_deg = p.yaw_deg
        self._overlay_height_m = p.height_m
        if not self.playing:
            self.update_camera_view(self.current_index)

    def _on_rpy_slider(self) -> None:
        self._on_rpy_params(self.params_bar.params())

    def _reset_rpy_sliders(self) -> None:
        self.params_bar.reset_rpy()

    def _sync_rpy_sliders_from_overlay(self) -> None:
        self.params_bar.set_rpy(
            roll_deg=self._overlay_roll_deg,
            pitch_deg=self._overlay_pitch_deg,
            yaw_deg=self._overlay_yaw_deg,
            height_m=self._overlay_height_m,
            notify=False,
        )

    def reset_vp_calib(self, load_saved: bool = False) -> None:
        """Reset online VP history; optionally load session ``calib_rpy.json``."""
        self.vp_calib.reset()
        self.vp_calib.estimated_pitch_deg = -6.0
        self.vp_calib.estimated_yaw_deg = 0.0
        self._overlay_pitch_deg = -6.0
        self._overlay_yaw_deg = 0.0
        self._overlay_roll_deg = 0.0
        self._overlay_height_m = 1.40
        self._vp_last_img_index = None
        if load_saved and self.bag_dir is not None:
            path = self.bag_dir / "calib_rpy.json"
            if path.is_file():
                try:
                    data = json.loads(path.read_text())
                    pitch = None
                    yaw = 0.0
                    roll = 0.0
                    if "pitch_deg" in data:
                        pitch = float(data["pitch_deg"])
                    elif "pitch" in data:
                        pitch = float(np.rad2deg(data["pitch"]))
                    if "yaw_deg" in data:
                        yaw = float(data["yaw_deg"])
                    elif "yaw" in data:
                        yaw = float(np.rad2deg(data["yaw"]))
                    if "roll_deg" in data:
                        roll = float(data["roll_deg"])
                    elif "roll" in data:
                        roll = float(np.rad2deg(data["roll"]))
                    # Looking-down windshield: reject saved +pitch (Hough junk)
                    if pitch is not None and pitch <= 2.0:
                        self.vp_calib.estimated_pitch_deg = pitch
                        self.vp_calib.estimated_yaw_deg = yaw
                        self._overlay_pitch_deg = pitch
                        self._overlay_yaw_deg = yaw
                        self._overlay_roll_deg = roll
                        self.vp_calib.calibration_success = bool(
                            data.get("calibration_success", True)
                        )
                        print(
                            f"Loaded VP calib from {path}: "
                            f"roll={roll:.2f}° pitch={pitch:.2f}° yaw={yaw:.2f}°"
                        )
                    else:
                        print(
                            f"Ignored {path} pitch={pitch} (expect looking-down ≤2°); "
                            "using Golf prior -6°"
                        )
                except Exception as e:
                    print(f"Failed to load {path}: {e}")
        self._sync_rpy_sliders_from_overlay()
        self._update_vp_status_label()
        if hasattr(self, "camera_frames") and self.camera_frames:
            self.update_display(self.current_index)

    def _update_vp_status_label(self) -> None:
        ok = self.vp_calib.calibration_success
        n = self.vp_calib.history_pending
        tag = "OK" if ok else f"…{n}/{self.vp_calib.history_len}"
        self.vp_status.config(
            text=(
                f"VP {tag}  R={self._overlay_roll_deg:.1f} "
                f"P={self._overlay_pitch_deg:.1f} Y={self._overlay_yaw_deg:.1f}°"
            ),
            foreground="green" if ok else "orange",
        )

    def _maybe_update_vp_calib(
        self, img: np.ndarray, fx: float, fy: float, cx: float, cy: float, img_index: int
    ) -> Tuple[Optional[Tuple[float, float]], Optional[Tuple], Optional[Tuple]]:
        """Run AAD Hough VP update once per camera frame index. Returns (vp, line_l, line_r)."""
        line_l = line_r = vp = None
        if not self.vp_calib_var.get():
            return None, None, None

        K = K_from_fx_fy_cx_cy(fx, fy, cx, cy)
        # Always fit lines for debug; only accumulate history on new frames / play
        line_l, line_r, _ = lines_from_image_hough(img)
        if line_l is not None and line_r is not None:
            vp = get_intersection(line_l, line_r)

        advance = self._vp_last_img_index is None or img_index != self._vp_last_img_index
        if advance and line_l is not None and line_r is not None:
            self._vp_last_img_index = img_index
            if self.vp_calib.update_from_lines(line_l, line_r, K):
                print(
                    f"VP calib commit #{self.vp_calib.n_updates}: "
                    f"pitch={self.vp_calib.estimated_pitch_deg:.2f}° "
                    f"yaw={self.vp_calib.estimated_yaw_deg:.2f}°"
                )
                self._overlay_pitch_deg = self.vp_calib.estimated_pitch_deg
                self._overlay_yaw_deg = self.vp_calib.estimated_yaw_deg
                self._sync_rpy_sliders_from_overlay()
                if self.bag_dir is not None:
                    out = self.bag_dir / "calib_rpy.json"
                    payload = self.vp_calib.to_dict()
                    payload["roll_deg"] = self._overlay_roll_deg
                    payload["roll"] = float(np.deg2rad(self._overlay_roll_deg))
                    payload["pitch"] = float(np.deg2rad(self.vp_calib.estimated_pitch_deg))
                    payload["yaw"] = float(np.deg2rad(self.vp_calib.estimated_yaw_deg))
                    try:
                        out.write_text(json.dumps(payload, indent=2))
                    except OSError:
                        pass
        self._update_vp_status_label()
        return vp, line_l, line_r

    def update_camera_view(self, index: int) -> None:
        if not self.camera_frames:
            return
        if len(self.timestamps) > 0 and index < len(self.timestamps):
            t = float(self.timestamps[index])
            img_index = self.find_closest_image_by_time(t)
        else:
            t = float(self.camera_frames[0][0])
            img_index = 0
        if img_index is None:
            return

        ts_cam, jpeg, cam_msg = self.camera_frames[img_index]
        img = cv2.imdecode(np.frombuffer(jpeg, np.uint8), cv2.IMREAD_COLOR)
        if img is None:
            return

        h, w = img.shape[:2]
        if self.intrinsics is not None:
            K = intrinsics_from_messages(cam_msg, self.intr_msg)
            fx, fy, cx, cy = K.fx, K.fy, K.cx, K.cy
            if abs(fx / max(fy, 1e-6) - 1.0) > 0.15:
                fy = fx
        else:
            fx = fy = 0.5 * w / np.tan(np.radians(35.0))
            cx, cy = 0.5 * w, 0.5 * h

        vp, line_l, line_r = self._maybe_update_vp_calib(img, fx, fy, cx, cy, img_index)
        # Sliders are source of truth for drawing (VP commits update pitch/yaw)
        roll_deg = (
            float(self.roll_var.get()) if hasattr(self, "roll_var") else self._overlay_roll_deg
        )
        pitch_deg = (
            float(self.pitch_var.get()) if hasattr(self, "pitch_var") else self._overlay_pitch_deg
        )
        yaw_deg = float(self.yaw_var.get()) if hasattr(self, "yaw_var") else self._overlay_yaw_deg
        height_m = (
            float(self.height_var.get()) if hasattr(self, "height_var") else self._overlay_height_m
        )

        if self.supercombo_var.get():
            geom = make_overlay_geometry(
                fx,
                fy,
                cx,
                cy,
                w,
                h,
                camera_height=height_m,
                pitch_deg=pitch_deg,
                yaw_deg=yaw_deg,
                roll_deg=roll_deg,
            )
            use_bag_lanes = self.lane_source_var.get() == "bag" and len(self.bag_lane_frames) > 0
            out = self.supercombo.infer(img, cache_key=img_index)

            if out is not None:
                if use_bag_lanes:
                    bag_lanes = self._find_bag_lanes_by_time(float(ts_cam))
                    if bag_lanes is not None:
                        # Bag lanes/plan are already ISO Y-left (Android negated).
                        # Do not redraw runtime plan/lanes — they use raw Y-right + y_sign=-1.
                        draw_bag_lanes(img, bag_lanes, geom, w, h, y_sign=1.0)
                        cv2.putText(
                            img,
                            f"bag vision/lanes  plan#{getattr(bag_lanes, 'plan_hyp', -1)}",
                            (8, 20),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            0.4,
                            (255, 255, 255),
                            1,
                            cv2.LINE_AA,
                        )
                    else:
                        draw_supercombo_overlay(
                            img,
                            out,
                            geom,
                            w,
                            h,
                            y_sign=-1.0,
                            lane_tag="runtime (no bag sync)",
                        )
                else:
                    draw_supercombo_overlay(
                        img,
                        out,
                        geom,
                        w,
                        h,
                        y_sign=-1.0,
                        lane_tag="runtime supercombo",
                    )
            else:
                cv2.putText(
                    img,
                    f"supercombo ERR: {(self.supercombo.error or '')[:50]}",
                    (8, h - 12),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.45,
                    (0, 0, 255),
                    1,
                    cv2.LINE_AA,
                )

            if self.pp_var.get() and out is not None:
                poly = plan_to_polyline_ego(out.plan.x, out.plan.y, y_sign=-1.0)
                speed = self._ego_speed_mps(index)
                self._sync_pp_controller()
                lk = self._pp_controller.compute_from_polyline(speed, poly)
                img = draw_lane_keep_overlay(
                    img,
                    lk,
                    fx=fx,
                    fy=fy,
                    cx=cx,
                    cy=cy,
                    w=w,
                    h=h,
                    geom=geom,
                    pitch_deg=pitch_deg,
                    yaw_deg=yaw_deg,
                    roll_deg=roll_deg,
                    camera_height=height_m,
                    waypoint_shift=float(self.pp_shift_var.get()),
                    y_sign=1.0,
                    draw_bev=False,
                    draw_footer=False,
                )
                self.pp_status.config(text=format_lane_keep_status(lk))
        elif self.pp_var.get():
            # PP needs plan from supercombo
            self.pp_status.config(text="PP needs Supercombo overlay")

        # Sync dt readout for debugging lag
        if len(self.timestamps) > 0 and index < len(self.timestamps):
            dt = ts_cam - float(self.timestamps[index])
            cv2.putText(
                img,
                f"cam-state dt={dt:+.0f}ms  img#{img_index}  "
                f"RPY={roll_deg:.1f}/{pitch_deg:.1f}/{yaw_deg:.1f}  h={height_m:.2f}m",
                (8, h - 28),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.4,
                (200, 200, 200),
                1,
                cv2.LINE_AA,
            )

        if self.vp_debug_var.get():
            draw_vp_debug(
                img,
                line_l,
                line_r,
                vp if vp is not None else self.vp_calib.last_vp,
                pitch_deg,
                yaw_deg,
                self.vp_calib.calibration_success,
            )

        from PIL import Image, ImageTk

        canvas_width = max(1, self.camera_canvas.winfo_width())
        canvas_height = max(1, self.camera_canvas.winfo_height())
        h, w = img.shape[:2]
        scale = min(canvas_width / w, canvas_height / h)
        new_w, new_h = max(1, int(w * scale)), max(1, int(h * scale))
        img_rgb = cv2.cvtColor(cv2.resize(img, (new_w, new_h)), cv2.COLOR_BGR2RGB)

        photo = ImageTk.PhotoImage(image=Image.fromarray(img_rgb))
        self.camera_canvas.delete("all")
        self.camera_canvas.create_image(
            (canvas_width - new_w) // 2,
            (canvas_height - new_h) // 2,
            image=photo,
            anchor=tk.NW,
        )
        self.camera_canvas.image = photo

    def update_sensor_data(self, index: int) -> None:
        if len(self.timestamps) == 0:
            return
        t = float(self.timestamps[index])

        imu = _nearest_row(self.imu_data, t)
        if imu is not None:
            text = (
                f"Accel: ax={imu[1]:6.2f} ay={imu[2]:6.2f} az={imu[3]:6.2f} m/s²\n"
                f"Gyro:  gx={imu[4]:6.3f} gy={imu[5]:6.3f} gz={imu[6]:6.3f} rad/s\n"
                f"Mag:   mx={imu[7]:6.1f} my={imu[8]:6.1f} mz={imu[9]:6.1f}"
            )
            self._set_text(self.imu_text, text)

        gps = _nearest_row(self.gps_data, t)
        if gps is not None:
            text = (
                f"Lat:   {gps[1]:12.8f}°\n"
                f"Lon:   {gps[2]:12.8f}°\n"
                f"Alt:   {gps[3]:8.2f} m\n"
                f"Speed: {gps[4]:6.2f} m/s"
            )
            self._set_text(self.gps_text, text)

        if self.wheel_data is not None and index < len(self.wheel_data):
            wh = self.wheel_data[index]
            # stored as vr,vl,hr,hl km/h (= fr,fl,rr,rl)
            text = (
                f"Wheels km/h: FR={wh[1]:5.1f} FL={wh[2]:5.1f}\n"
                f"             RR={wh[3]:5.1f} RL={wh[4]:5.1f}\n"
            )
            if self.steering_data is not None and index < len(self.steering_data):
                st = self.steering_data[index]
                deg = st[1] * _UNIT_TO_DEG
                if st[2] != 0:
                    deg = -deg
                text += f"Steer: {deg:7.1f} deg\n"
            if self.gear_data is not None and index < len(self.gear_data):
                name = str(self.gear_data[index][1]).upper()
                short = {
                    "PARK": "P",
                    "REVERSE": "R",
                    "NEUTRAL": "N",
                    "DRIVE": "D",
                    "SPORT": "S",
                    "ECO": "E",
                }.get(name, name[:1] if name else "?")
                text += f"Gear:  {short} ({name})"
            self._set_text(self.odom_text, text)

    @staticmethod
    def _set_text(widget: tk.Text, text: str) -> None:
        widget.config(state=tk.NORMAL)
        widget.delete(1.0, tk.END)
        widget.insert(1.0, text)
        widget.config(state=tk.DISABLED)

    def update_time_label(self, index: int) -> None:
        if len(self.timestamps) == 0:
            return
        elapsed = self.timestamps[index] - self.timestamps[0]
        duration = self.timestamps[-1] - self.timestamps[0]
        self.time_label.config(text=f"{_format_ms(elapsed)} / {_format_ms(duration)}")

    def find_closest_image_by_time(self, target_time: float) -> Optional[int]:
        """Latest camera frame with ts <= target (avoids showing a future image)."""
        if len(self.image_timestamps) == 0:
            return None
        idx = int(np.searchsorted(self.image_timestamps, target_time, side="right")) - 1
        if idx < 0:
            return 0
        if idx >= len(self.image_timestamps):
            return len(self.image_timestamps) - 1
        return idx

    def on_timeline_change(self, value) -> None:
        try:
            self.update_display(int(float(value)))
        except Exception as e:
            print(f"timeline error: {e}")

    def on_speed_change(self, *args) -> None:
        self.play_speed = float(self.speed_var.get())
        self.speed_label.config(text=f"{self.play_speed:.1f}x")

    def play(self) -> None:
        if not self.trajectories and len(self.timestamps) == 0:
            messagebox.showwarning("Warning", "Load a bag first")
            return
        self.playing = True
        self._play_last_wall_ms = None
        self.animate()

    def pause(self) -> None:
        self.playing = False
        self._play_last_wall_ms = None

    def restart(self) -> None:
        self.playing = False
        self._play_last_wall_ms = None
        self.supercombo.reset()
        self.time_var.set(0)
        self.update_display(0)

    def animate(self) -> None:
        if not self.playing:
            return
        import time

        wall_ms = time.monotonic() * 1000.0
        tick_ms = 50.0
        if self._play_last_wall_ms is None:
            self._play_last_wall_ms = wall_ms
            bag_dt = tick_ms * self.play_speed
        else:
            wall_dt = max(1.0, wall_ms - self._play_last_wall_ms)
            self._play_last_wall_ms = wall_ms
            # Real-time: 1.0x → advance bag by wall_dt milliseconds
            bag_dt = wall_dt * self.play_speed

        current = int(self.time_var.get())
        max_val = int(float(self.timeline_slider.cget("to")))
        if current >= max_val or len(self.timestamps) == 0:
            self.playing = False
            self.status_label.config(text="Playback finished", foreground="blue")
            return

        t_now = float(self.timestamps[current])
        t_target = t_now + bag_dt
        # Advance timeline index to cover bag time (vehicle/state clock)
        nxt = current
        while nxt < max_val and float(self.timestamps[nxt]) < t_target:
            nxt += 1
        if nxt == current and current < max_val:
            nxt = current + 1
        nxt = min(nxt, max_val)
        self.time_var.set(nxt)
        self.update_display(nxt)
        self.master.after(int(tick_ms), self.animate)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("bag", nargs="?", help="Session dir or .zip/.tar.gz")
    ap.add_argument("-i", "--input", help="Alias for bag path")
    args = ap.parse_args()
    bag = args.bag or args.input

    root = tk.Tk()
    InteractiveVisualizer(root, bag_path=bag)
    print("Interactive visualizer started (Open bag / Play / Pause / Restart)")
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
