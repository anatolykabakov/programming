#!/usr/bin/env python3
"""
MetaDrive simulation for lane-keeping algorithm tests.

Controllers (same defaults as interactive_visualizer / bag_lane_keep_offline):
  - straight       : baseline, no lateral control
  - pure_pursuit   : AAD Pure Pursuit on lane centerline / plan
  - lateral_pd     : lateral PD on centerline offset + heading

Lane source:
  - gt         : MetaDrive ground-truth lane boundaries
  - supercombo : live supercombo.onnx lanes (+ plan for PP)

Usage:
  python3 -m sim.main --controller pure_pursuit --show --lanes supercombo --compare-gt
  python3 -m sim.main --lanes gt --vp-source gt --show
  python3 -m sim.main --lanes supercombo --show --cv-show   # legacy OpenCV windows
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path
from typing import Any, Dict, Optional, Tuple

import _path  # noqa: F401

import cv2
import numpy as np

try:
    from metadrive.component.sensors.rgb_camera import RGBCamera
    from metadrive.envs.metadrive_env import MetaDriveEnv
    from metadrive.utils import setup_logger
except ImportError as e:
    print(f"Error importing MetaDrive: {e}")
    print("Install MetaDrive simulator:")
    print("  pip uninstall metadrive")
    print("  pip install metadrive-simulator")
    print("  # or: git clone https://github.com/metadriverse/metadrive && pip install -e .")
    sys.exit(1)

from core.online_localizer import OnlineVehicleEkf, draw_trajectory_panel
from core.lane_keep import LaneKeepController, LaneKeepResult
from core.lane_keep_viz import draw_lane_keep_overlay
from core.gt_compare import GtCompareAccumulator, compare_to_gt
from core.pure_pursuit import plan_to_polyline_ego
from core.native import cpp_available

from core.supercombo_compare import (
    DEFAULT_MODEL,
    SupercomboBev,
    draw_supercombo_overlay,
    make_overlay_geometry,
    supercombo_lanes_to_ego,
)
from core.supercombo_parse import SupercomboOut
from core.vanishing_point_calib import (
    K_from_fx_fy_cx_cy,
    VanishingPointCalibrator,
    draw_vp_debug,
    get_intersection,
    lines_from_image_hough,
    lines_from_projected_lanes,
)
from core.viz_params_ui import OverlayUiParams
from sim.camera_utils import CameraGeometry, CameraParams
from sim.controller import SimpleController
from sim.observation_parser import ObservationParser
from sim.sim_ui import SimLiveUi


def save_data(img, obs_data, frame, out_dir: Path, overlay_img=None):
    prefix = out_dir / f"frame_{frame:05d}"
    np.savetxt(f"{prefix}_left_line_road.txt", obs_data["lanes"]["left_road"])
    np.savetxt(f"{prefix}_right_line_road.txt", obs_data["lanes"]["right_road"])
    np.savetxt(f"{prefix}_left_line_world.txt", obs_data["lanes"]["left_world"])
    np.savetxt(f"{prefix}_right_line_world.txt", obs_data["lanes"]["right_world"])

    img = (overlay_img if overlay_img is not None else img).copy()
    if img.dtype != np.uint8:
        img = (np.clip(img, 0.0, 1.0) * 255.0).astype(np.uint8)
    cv2.imwrite(f"{prefix}.jpg", img)


class MetaDriveSimulator:
    """MetaDrive loop with lane-keeping controller + optional logging."""

    def __init__(self, args):
        setup_logger(True)
        self.args = args
        self.out_dir = Path(args.out_dir)
        self.out_dir.mkdir(parents=True, exist_ok=True)

        config = {
            "use_render": not args.no_render,
            "manual_control": False,
            "num_scenarios": args.num_scenarios,
            "start_seed": args.seed,
            "traffic_density": args.traffic_density,
            "random_traffic": args.traffic_density > 0,
            "decision_repeat": 1,
            "physics_world_step_size": 0.01,
            "horizon": 1_000_000,
            "image_observation": True,
            "show_terrain": True,
            "sensors": dict(rgb=[RGBCamera, args.width, args.height]),
            "vehicle_config": {"image_source": "rgb"},
        }

        self.env = MetaDriveEnv(config)
        self.env.reset()

        self.camera_params = CameraParams(self.env)
        self.camera_geometry = CameraGeometry(
            self.camera_params.intrinsics,
            self.camera_params.extrinsics,
        )
        np.savetxt(self.out_dir / "intrinsics.txt", self.camera_params.intrinsics)
        np.savetxt(self.out_dir / "extrinsics.txt", self.camera_params.extrinsics)

        # Seed AAD overlay from MetaDrive camera extrinsics (not Golf priors)
        aad = self.camera_params.aad_overlay_params()
        self.pitch_deg = (
            float(aad["pitch_deg"]) if args.pitch_deg is None else float(args.pitch_deg)
        )
        self.yaw_deg = float(aad["yaw_deg"]) if args.yaw_deg is None else float(args.yaw_deg)
        self.roll_deg = float(aad["roll_deg"]) if args.roll_deg is None else float(args.roll_deg)
        self.camera_height = (
            float(aad["camera_height"]) if args.camera_height is None else float(args.camera_height)
        )
        self.cam_x = float(aad["cam_x"]) if args.cam_x is None else float(args.cam_x)
        self.cam_y_left = float(aad["cam_y_left"])
        print(
            f"MD camera → ISO x={self.cam_x:.2f} y_left={self.cam_y_left:.2f} h={self.camera_height:.2f}  "
            f"R/P/Y={self.roll_deg:.2f}/{self.pitch_deg:.2f}/{self.yaw_deg:.2f}°  "
            f"(panda offset={self.camera_params.xyz_panda}, hpr={self.camera_params.hpr})"
        )
        with (self.out_dir / "camera_aad.json").open("w") as f:
            json.dump(
                {
                    **aad,
                    "pitch_deg_overlay": self.pitch_deg,
                    "yaw_deg_overlay": self.yaw_deg,
                    "roll_deg_overlay": self.roll_deg,
                    "camera_height_overlay": self.camera_height,
                    "cam_x_overlay": self.cam_x,
                    "xyz_panda": self.camera_params.xyz_panda.tolist(),
                    "xyz_iso": self.camera_params.xyz_iso.tolist(),
                    "hpr_panda": self.camera_params.hpr.tolist(),
                    "mount_offset": self.camera_params.mount_offset.tolist(),
                    "mount_hpr": self.camera_params.mount_hpr.tolist(),
                },
                f,
                indent=2,
            )

        self.observation_parser = ObservationParser(self.env)
        self.observation_parser.set_camera_mount(
            self.camera_params.mount_offset, self.camera_params.mount_hpr
        )
        if args.controller == "straight":
            self.controller = SimpleController(desired_speed=args.speed)
        else:
            self.controller = LaneKeepController(
                mode=args.controller,
                desired_speed=args.speed,
                max_steer_deg=args.max_steer_deg,
                wheelbase=args.wheelbase,
                pp_k_dd=args.pp_k_dd,
                pp_ld_min=args.pp_ld_min,
                pp_ld_max=args.pp_ld_max,
                pp_shift=args.pp_shift,
                pd_la=args.pd_la,
                pd_ky=args.pd_ky,
                pd_kpsi=args.pd_kpsi,
            )

        # Online EKF + dead-reckoning (bag-style trajectory panel)
        cfg = self.env.config
        self._sim_dt = float(cfg.get("physics_world_step_size", 0.02)) * float(
            cfg.get("decision_repeat", 1)
        )
        self.ekf_tracker: Optional[OnlineVehicleEkf] = None
        if args.traj:
            self.ekf_tracker = OnlineVehicleEkf(
                wheelbase=args.wheelbase,
                gps_noise_pos=args.ekf_gps_noise,
                gps_update_interval=args.ekf_gps_interval,
                gps_meas_noise=args.ekf_gps_meas_noise,
            )
            print(
                f"EKF traj ON  dt={self._sim_dt:.3f}s  "
                f"gps_interval={args.ekf_gps_interval}s  "
                f"gps_meas_noise={args.ekf_gps_meas_noise}m"
            )

        self.supercombo: Optional[SupercomboBev] = None
        if args.lanes == "supercombo" or args.draw_supercombo:
            self.supercombo = SupercomboBev(Path(args.supercombo_model))
            if not self.supercombo._ensure():
                print(f"WARNING: supercombo unavailable: {self.supercombo.error}")
            else:
                print(f"Supercombo → {self.supercombo.model_path}")

        # Dynamic AAD VP calibration (seeds from MetaDrive extrinsics above)
        self._init_pitch = self.pitch_deg
        self._init_yaw = self.yaw_deg
        self._init_roll = self.roll_deg
        self._init_height = self.camera_height
        self._ui: Optional[SimLiveUi] = None
        self._pp_enabled = self.args.controller != "straight"
        self.vp_calib = VanishingPointCalibrator(
            history_len=args.vp_history,
            estimated_pitch_deg=self.pitch_deg,
            estimated_yaw_deg=self.yaw_deg,
            camera_height_m=self.camera_height,
        )
        self.vp_enabled = bool(args.vp_calib)
        self.vp_debug = bool(args.vp_debug)
        self._vp_last_lines: Tuple[Optional[Tuple[float, float]], Optional[Tuple[float, float]]] = (
            None,
            None,
        )
        self._load_calib(args.calib)
        if self.vp_enabled:
            print(
                f"VP calib ON  source={args.vp_source}  "
                f"init R/P/Y={self.roll_deg:.1f}/{self.pitch_deg:.1f}/{self.yaw_deg:.1f}°  "
                f"h={self.camera_height:.2f}m cam_x={self.cam_x:.2f}"
            )

        self.frame = 0
        self._last_sc: Optional[SupercomboOut] = None
        self._last_gt_cmp = None
        self._gt_acc: Optional[GtCompareAccumulator] = None
        if args.compare_gt:
            self._gt_acc = GtCompareAccumulator()
            # Always show GT lanes when comparing
            if not args.draw_gt_lanes:
                args.draw_gt_lanes = True
        self._log_writer = None
        self._log_file = None
        self._log_path: Path | None = None
        if args.log_csv:
            log_path = Path(args.log_csv)
            if not log_path.is_absolute():
                log_path = self.out_dir / log_path
            self._log_path = log_path
            self._log_file = log_path.open("w", newline="")
            fields = [
                "frame",
                "mode",
                "lane_source",
                "speed_mps",
                "steer_rad",
                "steer_norm",
                "throttle",
                "brake",
                "e_y",
                "e_psi_rad",
                "curvature",
                "lookahead_m",
                "target_x",
                "target_y",
                "status",
                "pos_x",
                "pos_y",
                "heading_deg",
                "pitch_deg",
                "yaw_deg",
                "vp_ok",
            ]
            if args.compare_gt:
                fields += [
                    "steer_gt_rad",
                    "dsteer_rad",
                    "gt_ey",
                    "path_bias",
                    "path_rmse",
                    "dy_at_ld",
                    "tgt_y_gt",
                    "dtgt_y",
                ]
            self._log_writer = csv.DictWriter(self._log_file, fieldnames=fields)
            self._log_writer.writeheader()

        cmp_tag = f"  compare_gt=True  pp_on={args.pp_on}" if args.compare_gt else ""
        traj_tag = f"  traj={bool(self.ekf_tracker)}"
        cpp_tag = f"  cpp={'ON' if cpp_available() else 'off'}"
        print(
            f"Controller={args.controller}  lanes={args.lanes}  speed={args.speed} m/s  "
            f"traffic_density={args.traffic_density}  "
            f"vp={self.vp_enabled}  show={args.show}  save_every={args.save_every}  "
            f"overlay={args.overlay}{cmp_tag}{traj_tag}{cpp_tag}"
        )
        print(f"Output → {self.out_dir.resolve()}")
        if args.show:
            if args.cv_show:
                cv2.namedWindow("MetaDrive | camera + PP", cv2.WINDOW_NORMAL)
                cv2.resizeWindow("MetaDrive | camera + PP", args.width, args.height)
                print("OpenCV keys: q/Esc quit  r reset VP  d VP debug")
            else:
                print("Tk UI: live RPY/PP sliders (bag-style). Close window to quit.")
            if args.compare_gt:
                print(
                    "GT compare: white=GT centerline / GT target ring; "
                    "cyan=ctrl path; red=ctrl PP target"
                )
        if args.traj and args.cv_show and (args.show or args.overlay):
            cv2.namedWindow("MetaDrive | trajectory", cv2.WINDOW_NORMAL)
            cv2.resizeWindow("MetaDrive | trajectory", 480, 480)
            print("Trajectory: blue=GT  green=Odom  orange=EKF")

    def _load_calib(self, path: Optional[str]) -> None:
        if not path:
            return
        p = Path(path)
        if not p.is_file():
            print(f"WARNING: calib file not found: {p}")
            return
        try:
            data = json.loads(p.read_text())
            if "pitch_deg" in data:
                self.pitch_deg = float(data["pitch_deg"])
            if "yaw_deg" in data:
                self.yaw_deg = float(data["yaw_deg"])
            if "roll_deg" in data:
                self.roll_deg = float(data["roll_deg"])
            if "camera_height" in data:
                self.camera_height = float(data["camera_height"])
            if "cam_x" in data:
                self.cam_x = float(data["cam_x"])
            if "cam_y_left" in data:
                self.cam_y_left = float(data["cam_y_left"])
            self.vp_calib.estimated_pitch_deg = self.pitch_deg
            self.vp_calib.estimated_yaw_deg = self.yaw_deg
            self.vp_calib.calibration_success = bool(data.get("calibration_success", True))
            print(
                f"Loaded calib {p}: P={self.pitch_deg:.2f} Y={self.yaw_deg:.2f} h={self.camera_height:.2f}"
            )
        except Exception as e:
            print(f"WARNING: failed to load calib {p}: {e}")

    def _save_calib(self) -> None:
        out = self.out_dir / "calib_rpy.json"
        payload = self.vp_calib.to_dict()
        payload["roll_deg"] = self.roll_deg
        payload["camera_height"] = self.camera_height
        payload["cam_x"] = self.cam_x
        payload["cam_y_left"] = self.cam_y_left
        payload["pitch"] = float(np.deg2rad(self.pitch_deg))
        payload["yaw"] = float(np.deg2rad(self.yaw_deg))
        payload["roll"] = float(np.deg2rad(self.roll_deg))
        try:
            out.write_text(json.dumps(payload, indent=2))
        except OSError as e:
            print(f"WARNING: could not write {out}: {e}")

    def _road_to_uv(self, xy: np.ndarray) -> np.ndarray:
        if xy is None or len(xy) < 2:
            return np.empty((0, 2), dtype=np.float64)
        xyz = np.stack(
            [xy[:, 0], xy[:, 1], np.zeros(len(xy), dtype=np.float64)],
            axis=1,
        )
        return self.camera_geometry.xyz_to_uv(xyz)

    def _update_vp_calib(
        self,
        bgr: np.ndarray,
        gt_lanes: Dict[str, Any],
    ) -> None:
        if not self.vp_enabled:
            return
        K = self.camera_params.intrinsics
        line_l = line_r = None
        if self.args.vp_source == "gt":
            uv_l = self._road_to_uv(gt_lanes.get("left_road"))
            uv_r = self._road_to_uv(gt_lanes.get("right_road"))
            line_l, line_r = lines_from_projected_lanes(uv_l, uv_r)
        else:
            line_l, line_r, _ = lines_from_image_hough(bgr)

        self._vp_last_lines = (line_l, line_r)
        if line_l is None or line_r is None:
            return
        if self.vp_calib.update_from_lines(line_l, line_r, K):
            self.pitch_deg = self.vp_calib.estimated_pitch_deg
            self.yaw_deg = self.vp_calib.estimated_yaw_deg
            self._save_calib()
            print(
                f"VP calib #{self.vp_calib.n_updates}: "
                f"pitch={self.pitch_deg:.2f}° yaw={self.yaw_deg:.2f}°"
            )
            if self._ui is not None:
                self._ui.sync_rpy_from_sim()

    def _reset_vp(self) -> None:
        self.vp_calib.reset()
        self.pitch_deg = float(self._init_pitch)
        self.yaw_deg = float(self._init_yaw)
        self.roll_deg = float(self._init_roll)
        self.camera_height = float(self._init_height)
        self.vp_calib.estimated_pitch_deg = self.pitch_deg
        self.vp_calib.estimated_yaw_deg = self.yaw_deg
        print(f"VP reset → P={self.pitch_deg:.1f} Y={self.yaw_deg:.1f} (MD extrinsics)")

    def apply_overlay_rpy(self, p: OverlayUiParams) -> None:
        self.roll_deg = float(p.roll_deg)
        self.pitch_deg = float(p.pitch_deg)
        self.yaw_deg = float(p.yaw_deg)
        self.camera_height = float(p.height_m)
        # Keep VP estimates in sync with manual override
        self.vp_calib.estimated_pitch_deg = self.pitch_deg
        self.vp_calib.estimated_yaw_deg = self.yaw_deg

    def apply_pp_params(self, p: OverlayUiParams) -> None:
        p = p.clamped_pp()
        self.args.pp_k_dd = float(p.pp_k_dd)
        self.args.pp_ld_min = float(p.pp_ld_min)
        self.args.pp_ld_max = float(p.pp_ld_max)
        self.args.wheelbase = float(p.wheelbase)
        self.args.pp_shift = float(p.pp_shift)
        if self._pp_enabled and isinstance(self.controller, LaneKeepController):
            self._rebuild_lane_keep_controller(mode=self.controller.mode)

    def _rebuild_lane_keep_controller(self, mode: str) -> None:
        self.controller = LaneKeepController(
            mode=mode,
            desired_speed=self.args.speed,
            max_steer_deg=self.args.max_steer_deg,
            wheelbase=self.args.wheelbase,
            pp_k_dd=self.args.pp_k_dd,
            pp_ld_min=self.args.pp_ld_min,
            pp_ld_max=self.args.pp_ld_max,
            pp_shift=self.args.pp_shift,
            pd_la=self.args.pd_la,
            pd_ky=self.args.pd_ky,
            pd_kpsi=self.args.pd_kpsi,
        )

    def set_lane_source(self, source: str) -> None:
        if source not in ("gt", "supercombo"):
            return
        self.args.lanes = source
        if source == "supercombo" and self.supercombo is None:
            self.supercombo = SupercomboBev(Path(self.args.supercombo_model))
            if not self.supercombo._ensure():
                print(f"WARNING: supercombo unavailable: {self.supercombo.error}")
            else:
                print(f"Supercombo → {self.supercombo.model_path}")
        print(f"Lane source → {source}")

    def set_pp_enabled(self, enabled: bool) -> None:
        self._pp_enabled = bool(enabled)
        if not enabled:
            self.controller = SimpleController(desired_speed=self.args.speed)
            print("Controller → straight (PP off)")
            return
        mode = self.args.controller if self.args.controller != "straight" else "pure_pursuit"
        self._rebuild_lane_keep_controller(mode=mode)
        print(f"Controller → {mode}")

    def _to_uint8(self, img: np.ndarray) -> np.ndarray:
        if img.dtype == np.uint8:
            return img
        return (np.clip(img, 0.0, 1.0) * 255.0).astype(np.uint8)

    def _run_supercombo(self, camera_image: np.ndarray) -> Optional[SupercomboOut]:
        if self.supercombo is None:
            return None
        bgr = self._to_uint8(camera_image)
        # MetaDrive RGB camera is often RGB float; convert to BGR for OpenCV/ONNX path
        if camera_image.ndim == 3 and camera_image.shape[2] == 3:
            # perceive() returns RGB; OpenCV expects BGR
            bgr = cv2.cvtColor(bgr, cv2.COLOR_RGB2BGR)
        out = self.supercombo.infer(bgr, cache_key=self.frame)
        self._last_sc = out
        return out

    def _control_lanes(
        self,
        gt_lanes: Dict[str, Any],
        sc: Optional[SupercomboOut],
    ) -> Dict[str, Any]:
        """Lane dict for the controller (gt or supercombo near-lanes)."""
        if self.args.lanes != "supercombo" or sc is None:
            return gt_lanes
        sc_lanes = supercombo_lanes_to_ego(
            sc,
            min_lane_prob=self.args.min_lane_prob,
            y_sign=-1.0,
        )
        if len(sc_lanes["left_road"]) < 2 or len(sc_lanes["right_road"]) < 2:
            return gt_lanes  # fallback
        return {
            **gt_lanes,
            "left_road": sc_lanes["left_road"],
            "right_road": sc_lanes["right_road"],
        }

    def _compute_control(
        self,
        speed: float,
        lanes: Dict[str, Any],
        sc: Optional[SupercomboOut],
    ) -> tuple[Optional[LaneKeepResult], list[float]]:
        if not isinstance(self.controller, LaneKeepController):
            return None, self.controller.get_control(speed, lanes)

        # Supercombo plan path (optional). Default PP uses lane centerline via compute().
        if (
            self.args.lanes == "supercombo"
            and sc is not None
            and self.controller.mode == "pure_pursuit"
            and self.args.pp_on == "plan"
        ):
            poly = plan_to_polyline_ego(
                sc.plan.x,
                sc.plan.y,
                y_sign=-1.0,
                recenter=bool(self.args.recenter_plan),
            )
            lk = self.controller.compute_from_polyline(speed, poly)
            return lk, [lk.steer_norm, lk.throttle, lk.brake]

        lk = self.controller.compute(speed, lanes)
        return lk, [lk.steer_norm, lk.throttle, lk.brake]

    def _make_viz_image(
        self,
        camera_image: np.ndarray,
        lanes: dict,
        lk: LaneKeepResult | None,
        sc: Optional[SupercomboOut],
        gt_lanes: Optional[dict] = None,
    ) -> np.ndarray:
        need_viz = self.args.show or self.args.overlay
        if not need_viz:
            return self._to_uint8(camera_image)

        bgr = self._to_uint8(camera_image)
        if camera_image.ndim == 3 and camera_image.shape[2] == 3:
            bgr = cv2.cvtColor(bgr, cv2.COLOR_RGB2BGR)

        # Online VP update on the BGR frame used for overlay
        self._update_vp_calib(bgr, gt_lanes or lanes)

        K = self.camera_params.intrinsics
        fx, fy = float(K[0, 0]), float(K[1, 1])
        cx, cy = float(K[0, 2]), float(K[1, 2])
        h, w = bgr.shape[:2]
        geom = make_overlay_geometry(
            fx,
            fy,
            cx,
            cy,
            w,
            h,
            camera_height=self.camera_height,
            pitch_deg=self.pitch_deg,
            yaw_deg=self.yaw_deg,
            roll_deg=self.roll_deg,
            cam_x=self.cam_x,
            cam_y_left=self.cam_y_left,
        )

        if (self.args.draw_supercombo or self.args.lanes == "supercombo") and sc is not None:
            draw_sc = True
            if self._ui is not None:
                draw_sc = self._ui.draw_supercombo()
            if draw_sc:
                draw_supercombo_overlay(
                    bgr,
                    sc,
                    geom,
                    w,
                    h,
                    y_sign=-1.0,
                    min_lane_prob=self.args.min_lane_prob,
                )
        elif sc is None and self.supercombo is not None and self.supercombo.error:
            cv2.putText(
                bgr,
                f"supercombo ERR: {self.supercombo.error[:48]}",
                (8, 20),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.4,
                (0, 0, 255),
                1,
                cv2.LINE_AA,
            )

        if lk is not None:
            gt_poly = gt_lk = None
            if self._last_gt_cmp is not None:
                gt_poly = self._last_gt_cmp.gt_poly
                gt_lk = self._last_gt_cmp.lk_gt
            draw_lanes = None
            want_gt = self.args.draw_gt_lanes
            if self._ui is not None:
                want_gt = self._ui.draw_gt_lanes()
            if want_gt:
                draw_lanes = gt_lanes if gt_lanes is not None else lanes
            draw_bev = bool(self.args.bev)
            draw_footer = True
            if self._ui is not None:
                draw_bev = self._ui.draw_bev()
                draw_footer = False  # bag-style HUD
            bgr = draw_lane_keep_overlay(
                bgr,
                lk,
                fx=fx,
                fy=fy,
                cx=cx,
                cy=cy,
                w=w,
                h=h,
                lanes=draw_lanes,
                pitch_deg=self.pitch_deg,
                yaw_deg=self.yaw_deg,
                roll_deg=self.roll_deg,
                camera_height=self.camera_height,
                waypoint_shift=self.args.pp_shift,
                draw_bev=draw_bev,
                draw_footer=draw_footer,
                geom=geom,
                gt_poly=gt_poly,
                gt_lk=gt_lk,
            )

            if self._last_gt_cmp is not None and np.isfinite(self._last_gt_cmp.path_bias):
                cmp = self._last_gt_cmp
                cv2.putText(
                    bgr,
                    f"GT cmp  bias={cmp.path_bias:+.3f}m  rmse={cmp.path_rmse:.3f}  "
                    f"ey={cmp.gt_ey:+.2f}  dδ={np.rad2deg(cmp.dsteer_rad):+.2f}°",
                    (8, 60),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.4,
                    (255, 255, 255),
                    1,
                    cv2.LINE_AA,
                )

        vp_tag = (
            "OK"
            if self.vp_calib.calibration_success
            else (f"…{self.vp_calib.history_pending}/{self.vp_calib.history_len}")
        )
        cv2.putText(
            bgr,
            f"lanes={self.args.lanes}  VP {vp_tag}  "
            f"R/P/Y={self.roll_deg:.1f}/{self.pitch_deg:.1f}/{self.yaw_deg:.1f}°",
            (8, 40),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.4,
            (0, 255, 0) if self.vp_calib.calibration_success else (0, 200, 255),
            1,
            cv2.LINE_AA,
        )

        if self.vp_debug:
            line_l, line_r = self._vp_last_lines
            vp = None
            if line_l is not None and line_r is not None:
                vp = get_intersection(line_l, line_r)
            draw_vp_debug(
                bgr,
                line_l,
                line_r,
                vp if vp is not None else self.vp_calib.last_vp,
                self.pitch_deg,
                self.yaw_deg,
                self.vp_calib.calibration_success,
            )
        return bgr

    def _log_step(self, odometry, lk_result):
        if self._log_writer is None:
            return
        pp = lk_result.pure_pursuit if lk_result else None
        tgt_x = tgt_y = ""
        la = ""
        if pp is not None:
            la = f"{pp.lookahead_m:.3f}"
            if pp.target_ego is not None:
                tgt_x = f"{pp.target_ego[0]:.3f}"
                tgt_y = f"{pp.target_ego[1]:.3f}"
        row = {
            "frame": self.frame,
            "mode": lk_result.mode if lk_result else self.args.controller,
            "lane_source": self.args.lanes,
            "speed_mps": f"{odometry['speed']:.3f}",
            "steer_rad": f"{lk_result.steer_rad:.5f}" if lk_result else "0",
            "steer_norm": f"{lk_result.steer_norm:.5f}" if lk_result else "0",
            "throttle": f"{lk_result.throttle:.3f}" if lk_result else "",
            "brake": f"{lk_result.brake:.3f}" if lk_result else "",
            "e_y": f"{lk_result.e_y:.4f}" if lk_result else "",
            "e_psi_rad": f"{lk_result.e_psi:.5f}" if lk_result else "",
            "curvature": f"{lk_result.curvature:.6f}" if lk_result else "",
            "lookahead_m": la,
            "target_x": tgt_x,
            "target_y": tgt_y,
            "status": lk_result.status if lk_result else "",
            "pos_x": f"{odometry['position'][0]:.2f}",
            "pos_y": f"{odometry['position'][1]:.2f}",
            "heading_deg": f"{np.rad2deg(odometry['heading']):.1f}",
            "pitch_deg": f"{self.pitch_deg:.3f}",
            "yaw_deg": f"{self.yaw_deg:.3f}",
            "vp_ok": int(self.vp_calib.calibration_success),
        }
        cmp = self._last_gt_cmp
        if self.args.compare_gt:

            def _f(v: float, fmt: str = ".5f") -> str:
                return format(v, fmt) if v is not None and np.isfinite(v) else ""

            if cmp is not None:
                row.update(
                    {
                        "steer_gt_rad": _f(cmp.steer_gt_rad),
                        "dsteer_rad": _f(cmp.dsteer_rad),
                        "gt_ey": _f(cmp.gt_ey, ".4f"),
                        "path_bias": _f(cmp.path_bias, ".4f"),
                        "path_rmse": _f(cmp.path_rmse, ".4f"),
                        "dy_at_ld": _f(cmp.dy_at_ld, ".4f"),
                        "tgt_y_gt": _f(cmp.tgt_y_gt, ".4f"),
                        "dtgt_y": _f(cmp.dtgt_y, ".4f"),
                    }
                )
            else:
                for k in (
                    "steer_gt_rad",
                    "dsteer_rad",
                    "gt_ey",
                    "path_bias",
                    "path_rmse",
                    "dy_at_ld",
                    "tgt_y_gt",
                    "dtgt_y",
                ):
                    row[k] = ""
        self._log_writer.writerow(row)

    def _step_ekf(self, odometry: Dict[str, Any], steer_rad: float) -> Optional[np.ndarray]:
        """Advance online EKF; return trajectory panel image if visualizing."""
        if self.ekf_tracker is None:
            return None
        pos = odometry["position"]
        gt_x, gt_y = float(pos[0]), float(pos[1])
        gt_yaw = float(odometry["heading"])
        yaw_rate = float(odometry.get("yaw_rate") or 0.0)
        speed = float(odometry["speed"])

        self.ekf_tracker.step(
            gt_x=gt_x,
            gt_y=gt_y,
            gt_yaw=gt_yaw,
            speed_mps=speed,
            steer_rad=float(steer_rad),
            yaw_rate=yaw_rate,
            dt=self._sim_dt,
        )
        if not (self.args.show or self.args.overlay):
            return None
        panel = draw_trajectory_panel(self.ekf_tracker.buffers, size=480)
        e_ekf, e_odom = self.ekf_tracker.position_errors()
        if np.isfinite(e_ekf):
            cv2.putText(
                panel,
                f"RMSE EKF={e_ekf:.2f}m  Odom={e_odom:.2f}m",
                (8, panel.shape[0] - 12),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.4,
                (220, 220, 220),
                1,
                cv2.LINE_AA,
            )
        return panel

    def run(self):
        try:
            if self.args.show and not self.args.cv_show:
                self._ui = SimLiveUi(self)

            while True:
                if self._ui is not None and not self._ui.pump():
                    break

                obs_data = self.observation_parser.make_perception_data()
                odometry = obs_data["odometry"]
                gt_lanes = obs_data["lanes"]
                camera_image = obs_data["camera_image"]

                sc = None
                if camera_image is not None and self.supercombo is not None:
                    sc = self._run_supercombo(camera_image)

                lanes = self._control_lanes(gt_lanes, sc)
                lk, action = self._compute_control(odometry["speed"], lanes, sc)

                steer_rad = float(lk.steer_rad) if lk is not None else 0.0
                traj_img = self._step_ekf(odometry, steer_rad)

                self._last_gt_cmp = None
                if self.args.compare_gt and isinstance(self.controller, LaneKeepController):
                    self._last_gt_cmp = compare_to_gt(
                        self.controller,
                        odometry["speed"],
                        gt_lanes,
                        lk,
                    )
                    if self._gt_acc is not None:
                        self._gt_acc.add(
                            self._last_gt_cmp,
                            pitch_deg=self.pitch_deg,
                            yaw_deg=self.yaw_deg,
                        )

                self._log_step(odometry, lk)

                if camera_image is not None:
                    # Keep VP alive even without --show/--overlay
                    if self.vp_enabled and not (self.args.show or self.args.overlay):
                        bgr = self._to_uint8(camera_image)
                        if camera_image.ndim == 3 and camera_image.shape[2] == 3:
                            bgr = cv2.cvtColor(bgr, cv2.COLOR_RGB2BGR)
                        self._update_vp_calib(bgr, gt_lanes)

                    viz_img = self._make_viz_image(camera_image, lanes, lk, sc, gt_lanes=gt_lanes)

                    if self._ui is not None:
                        status = self._status_line(odometry, lk, sc)
                        self._ui.show_frame(viz_img, traj_img, status=status)
                        if not self._ui.pump():
                            break
                    elif self.args.show:
                        cv2.imshow("MetaDrive | camera + PP", viz_img)
                        if traj_img is not None:
                            cv2.imshow("MetaDrive | trajectory", traj_img)
                        key = cv2.waitKey(1) & 0xFF
                        if key in (ord("q"), 27):
                            break
                        if key == ord("r"):
                            self._reset_vp()
                        if key == ord("d"):
                            self.vp_debug = not self.vp_debug
                            print(f"VP debug → {self.vp_debug}")

                    if self.frame % self.args.save_every == 0:
                        save_data(
                            camera_image,
                            obs_data,
                            self.frame,
                            self.out_dir,
                            overlay_img=viz_img if self.args.overlay else None,
                        )
                        if traj_img is not None and self.args.overlay:
                            cv2.imwrite(
                                str(self.out_dir / f"traj_{self.frame:05d}.jpg"),
                                traj_img,
                            )

                if self.frame % self.args.print_every == 0:
                    print(self._status_line(odometry, lk, sc, prefix=f"Frame {self.frame}: "))

                self.env.step(action)
                self.frame += 1

        except KeyboardInterrupt:
            print("\nSimulation interrupted by user")
        finally:
            self.cleanup()

    def _status_line(
        self,
        odometry: Dict[str, Any],
        lk: Optional[LaneKeepResult],
        sc: Optional[SupercomboOut],
        prefix: str = "",
    ) -> str:
        steer_deg = np.rad2deg(lk.steer_rad) if lk else 0.0
        extra = ""
        if lk and lk.pure_pursuit is not None:
            extra = f" Ld={lk.pure_pursuit.lookahead_m:.1f}m"
        elif lk and lk.mode == "lateral_pd":
            extra = f" e_y={lk.e_y:.2f} e_psi={np.rad2deg(lk.e_psi):+.1f}°"
        sc_tag = ""
        if sc is not None:
            probs = ",".join(f"{l.prob:.2f}" for l in sc.lanes)
            sc_tag = f" sc_p=[{probs}]"
        vp_tag = (
            f" VP P={self.pitch_deg:.1f} Y={self.yaw_deg:.1f}"
            f"{' OK' if self.vp_calib.calibration_success else ''}"
        )
        cmp_tag = ""
        if self._last_gt_cmp is not None and np.isfinite(self._last_gt_cmp.path_bias):
            c = self._last_gt_cmp
            cmp_tag = (
                f" bias={c.path_bias:+.3f}m dδ={np.rad2deg(c.dsteer_rad):+.2f}°"
                f" ey={c.gt_ey:+.2f}"
            )
        ekf_tag = ""
        if self.ekf_tracker is not None:
            e_ekf, e_odom = self.ekf_tracker.position_errors()
            if np.isfinite(e_ekf):
                ekf_tag = f" ekf_rmse={e_ekf:.2f}m odom_rmse={e_odom:.2f}m"
        return (
            f"{prefix}pos=({odometry['position'][0]:.1f}, {odometry['position'][1]:.1f}) "
            f"v={odometry['speed']:.1f} m/s "
            f"steer={steer_deg:+.1f}°{extra} "
            f"lanes={self.args.lanes}"
            f"{sc_tag}{vp_tag}{cmp_tag}{ekf_tag}"
        )

    def cleanup(self):
        if self.ekf_tracker is not None and self.ekf_tracker.buffers.gt_x:
            e_ekf, e_odom = self.ekf_tracker.position_errors()
            print(
                f"\n=== EKF trajectory ===\n"
                f"  points={len(self.ekf_tracker.buffers.gt_x)}  "
                f"RMSE EKF={e_ekf:.3f} m  Odom={e_odom:.3f} m"
            )
            try:
                panel = draw_trajectory_panel(self.ekf_tracker.buffers, size=640)
                out_img = self.out_dir / "trajectory_ekf.jpg"
                cv2.imwrite(str(out_img), panel)
                b = self.ekf_tracker.buffers
                np.savez(
                    self.out_dir / "trajectory_ekf.npz",
                    gt_x=np.asarray(b.gt_x),
                    gt_y=np.asarray(b.gt_y),
                    odom_x=np.asarray(b.odom_x),
                    odom_y=np.asarray(b.odom_y),
                    ekf_x=np.asarray(b.ekf_x),
                    ekf_y=np.asarray(b.ekf_y),
                )
                print(f"Trajectory → {out_img.resolve()}")
                if self.ekf_tracker.ekf is not None:
                    self.ekf_tracker.ekf.print_statistics()
            except Exception as e:
                print(f"WARNING: failed to save trajectory: {e}")
        if self._gt_acc is not None and self._gt_acc.path_bias:
            summary = self._gt_acc.summary()
            text = self._gt_acc.format_summary()
            print("\n" + text)
            out = self.out_dir / "gt_compare_summary.json"
            try:
                out.write_text(json.dumps(summary, indent=2))
                print(f"GT compare summary → {out.resolve()}")
            except Exception as e:
                print(f"WARNING: failed to write {out}: {e}")
        if self._log_file is not None:
            self._log_file.close()
            if self._log_path is not None:
                print(f"Log → {self._log_path.resolve()}")
        if self._ui is not None:
            try:
                self._ui.request_quit()
            except Exception:
                pass
            self._ui = None
        if self.args.show or self.args.traj:
            try:
                cv2.destroyAllWindows()
            except Exception:
                pass
        print("Closing environment...")
        self.env.close()
        print("Done.")


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--no-render", action="store_true", help="Disable MetaDrive 3D window")
    p.add_argument(
        "--show",
        action="store_true",
        help="Live Tk UI (bag-style): camera + trajectory + RPY/PP sliders",
    )
    p.add_argument(
        "--cv-show",
        action="store_true",
        help="Use OpenCV windows instead of Tk (legacy; with --show)",
    )
    p.add_argument(
        "--traj",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Top-down GT / bicycle-odom / EKF trajectory (bag-style). Default on. "
        "Disable with --no-traj.",
    )
    p.add_argument(
        "--ekf-gps-interval",
        type=float,
        default=0.2,
        help="Seconds between EKF GPS updates (MetaDrive pose as GPS)",
    )
    p.add_argument(
        "--ekf-gps-noise",
        type=float,
        default=0.5,
        help="EKF GPS measurement noise σ (m) used in R_gps",
    )
    p.add_argument(
        "--ekf-gps-meas-noise",
        type=float,
        default=0.0,
        help="Optional noise added to GT pose before GPS update (m, 0=perfect GPS)",
    )
    p.add_argument(
        "--controller",
        choices=LaneKeepController.MODES,
        default="pure_pursuit",
        help="Lane keeping algorithm",
    )
    p.add_argument(
        "--lanes",
        choices=("gt", "supercombo"),
        default="supercombo",
        help="Lane source for control: MetaDrive GT or live supercombo.onnx",
    )
    p.add_argument(
        "--pp-on",
        choices=("lanes", "plan"),
        default="lanes",
        help="When --lanes supercombo: PP on lane centerline (default) or model plan. "
        "Plan often has a left lateral bias on MetaDrive images.",
    )
    p.add_argument(
        "--recenter-plan",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="When --pp-on=plan, zero near-field lateral bias of the plan "
        "(default: on). Disable with --no-recenter-plan.",
    )
    p.add_argument(
        "--supercombo-model",
        type=str,
        default=str(DEFAULT_MODEL),
        help="Path to supercombo.onnx",
    )
    p.add_argument("--min-lane-prob", type=float, default=0.3)
    p.add_argument(
        "--draw-supercombo",
        action="store_true",
        default=True,
        help="Draw supercombo plan/lanes/edges on camera (default on)",
    )
    p.add_argument("--no-draw-supercombo", action="store_false", dest="draw_supercombo")
    p.add_argument(
        "--draw-gt-lanes",
        action="store_true",
        help="Also draw MetaDrive GT lane boundaries on overlay",
    )
    p.add_argument(
        "--compare-gt",
        action="store_true",
        help="Shadow GT centerline + GT Pure Pursuit vs control path: log bias/rmse/"
        "dsteer, draw white GT path, write gt_compare_summary.json (for calib/PP tune)",
    )
    p.add_argument("--out-dir", default="run", help="Output directory for frames/logs")
    p.add_argument("--log-csv", default="lane_keep.csv", help="CSV log filename (in out-dir)")
    p.add_argument("--save-every", type=int, default=30, help="Save frame every N steps")
    p.add_argument("--print-every", type=int, default=30, help="Print status every N steps")
    p.add_argument("--overlay", action="store_true", help="Draw lane/PP overlay on saved frames")
    p.add_argument("--bev", action="store_true", default=True, help="BEV inset on overlay")
    p.add_argument("--no-bev", action="store_false", dest="bev")

    p.add_argument("--speed", type=float, default=12.0, help="Target speed m/s")
    p.add_argument("--max-steer-deg", type=float, default=40.0)
    p.add_argument("--wheelbase", type=float, default=2.636)

    # Pure pursuit (interactive_visualizer defaults)
    p.add_argument("--pp-k-dd", type=float, default=0.4)
    p.add_argument("--pp-ld-min", type=float, default=3.0)
    p.add_argument("--pp-ld-max", type=float, default=20.0)
    p.add_argument("--pp-shift", type=float, default=1.40)

    # Lateral PD (bag_lane_keep_offline defaults)
    p.add_argument("--pd-la", type=float, default=15.0)
    p.add_argument("--pd-ky", type=float, default=0.1)
    p.add_argument("--pd-kpsi", type=float, default=0.5)

    # Overlay camera — default None → read from MetaDrive extrinsics
    p.add_argument(
        "--pitch-deg",
        type=float,
        default=None,
        help="AAD pitch (None = from MetaDrive camera)",
    )
    p.add_argument("--yaw-deg", type=float, default=None)
    p.add_argument("--roll-deg", type=float, default=None)
    p.add_argument("--camera-height", type=float, default=None)
    p.add_argument(
        "--cam-x",
        type=float,
        default=None,
        help="Camera forward offset in ISO ego frame (None = from MetaDrive)",
    )
    p.add_argument(
        "--vp-calib",
        action="store_true",
        default=True,
        help="Online AAD vanishing-point pitch/yaw calibration (default on)",
    )
    p.add_argument("--no-vp-calib", action="store_false", dest="vp_calib")
    p.add_argument(
        "--vp-source",
        choices=("image", "gt"),
        default="image",
        help="VP lines from Hough on camera (image) or MetaDrive GT lanes (gt)",
    )
    p.add_argument("--vp-history", type=int, default=50, help="VP samples before commit")
    p.add_argument("--vp-debug", action="store_true", help="Draw VP lines / intersection")
    p.add_argument(
        "--calib",
        type=str,
        default=None,
        help="Load initial calib_rpy.json (pitch/yaw/roll/height)",
    )

    p.add_argument("--seed", type=int, default=42)
    p.add_argument("--num-scenarios", type=int, default=100)
    p.add_argument(
        "--traffic-density",
        type=float,
        default=0.0,
        help="Background traffic density (0 = no other vehicles)",
    )
    p.add_argument("--width", type=int, default=640)
    p.add_argument("--height", type=int, default=480)
    return p


def main():
    args = build_arg_parser().parse_args()
    try:
        MetaDriveSimulator(args).run()
    except Exception as e:
        print(f"Error running simulation: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
