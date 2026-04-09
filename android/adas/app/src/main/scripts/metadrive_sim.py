"""
MetaDrive Simulation for ADAS Lane Detection and Control
Simplified version without external dependencies
"""

import argparse
import numpy as np
import cv2
import sys
import os

try:
    from metadrive.envs.metadrive_env import MetaDriveEnv
    from metadrive.utils import setup_logger
    from metadrive.component.sensors.rgb_camera import RGBCamera
except ImportError as e:
    print(f"Error importing MetaDrive: {e}")
    print("Make sure MetaDrive is installed: pip install metadrive")
    sys.exit(1)


class CameraGeometry:
    """Camera geometry for projecting lane polygons onto camera image"""

    def __init__(self, Rt, width, height, fov_deg=65):
        self.K = self._get_intrinsic_matrix(fov_deg, width, height)
        self.Rt = Rt  # world to camera transformation matrix

    @staticmethod
    def _get_intrinsic_matrix(field_of_view_deg, width, height):
        """Calculate camera intrinsic matrix"""
        degree_to_rad = lambda deg: deg * np.pi / 180
        field_of_view_rad = degree_to_rad(field_of_view_deg)
        alpha = (width / 2.0) / np.tan(field_of_view_rad / 2.0)
        cx = width / 2.0
        cy = height / 2.0
        return np.array([[alpha, 0, cx], [0, alpha, cy], [0, 0, 1.0]])

    def world_to_camera(self, polyline_world):
        """
        Project polyline from road coordinates to image pixel coordinates

        Args:
            polyline_world: Nx2 or Nx3 array of (x, y) or (x, y, z) points in road frame

        Returns:
            Nx2 array of (u, v) pixel coordinates
        """
        # Extract coordinates and add z=0 for road surface
        x = polyline_world[:, 0]  # forward (X in road frame)
        y = polyline_world[
            :, 1
        ]  # left (Y in road frame, negative = right side, positive = left side)
        z = (
            np.zeros(len(polyline_world)) if polyline_world.shape[1] == 2 else polyline_world[:, 2]
        )  # up (Z in road frame)

        # Формируем однородные координаты в road frame
        homvec = np.stack((x, y, z, np.ones_like(x)))  # Shape: (4, N)

        # Project: K @ Rt @ homvec gives [u*w, v*w, w]
        pl_uv_cam = (self.K @ self.Rt[:3, :] @ homvec).T

        # Normalize by w to get u, v
        u = pl_uv_cam[:, 0] / pl_uv_cam[:, 2]
        v = pl_uv_cam[:, 1] / pl_uv_cam[:, 2]

        return np.stack((u, v)).T


class SimpleController:
    """Simple vehicle control - drive straight"""

    def __init__(self, desired_speed=20.0):
        self.desired_speed = desired_speed

    def get_control(self, speed, detected_lines=None):
        """Compute throttle and steering - keep going straight"""
        # Maintain desired speed
        throttle = 0.5 if speed < self.desired_speed else 0.0

        # Always drive straight (no steering correction)
        steer = 0.0
        brake = 0.0

        return [steer, throttle, brake]


class ObservationParser:
    """Parse observations from MetaDrive environment into structured data"""

    def __init__(self, env, camera_extrinsics):
        self.env = env
        self.camera_extrinsics = camera_extrinsics

    def make_odometry(self, agent):
        """Get vehicle odometry (position, heading, velocity, speed, acceleration, yaw_rate, roll, pitch)"""
        return {
            "position": np.array(agent.position),
            "heading": agent.heading_theta,
            "velocity": np.array(agent.velocity),
            "speed": agent.speed,
            "acceleration": np.array(agent.acc) if hasattr(agent, "acc") else np.array([0, 0, 0]),
            "yaw_rate": agent.yaw_rate if hasattr(agent, "yaw_rate") else 0.0,
            "roll": agent.roll if hasattr(agent, "roll") else 0.0,
            "pitch": agent.pitch if hasattr(agent, "pitch") else 0.0,
        }

    def make_lane_observations(self, agent, max_distance=30.0):
        """
        Get lane polygons (left and right boundaries of current lane)
        Returns polygon points in local vehicle coordinates
        """
        current_lane = agent.lane
        lane_polygon = np.array(current_lane.polygon)

        # Split into right and left sides
        right_side = lane_polygon[: len(lane_polygon) // 2]
        left_side = lane_polygon[len(lane_polygon) // 2 :][::-1]

        # Filter points within max_distance
        def filter_by_distance(points, agent_pos, max_dist):
            distances = np.linalg.norm(points - agent_pos, axis=1)
            valid_indices = np.where(distances <= max_dist)[0]
            return points[valid_indices]

        right_filtered = filter_by_distance(right_side, agent.position, max_distance)
        left_filtered = filter_by_distance(left_side, agent.position, max_distance)

        # Convert from WORLD frame to ROAD frame
        # Road frame origin is at vehicle's position (bottom of front wheel per ISO8855 standard)
        # convert_to_local_coordinates does: road_coords = world_coords - vehicle_position
        right_local = np.array(
            [agent.convert_to_local_coordinates(p, agent.position) for p in right_filtered]
        )
        left_local = np.array(
            [agent.convert_to_local_coordinates(p, agent.position) for p in left_filtered]
        )

        # Filter out points behind the vehicle (x < 0 in road frame)
        # Camera only sees forward, so points with negative x should be removed
        # right_local = right_local[right_local[:, 0] >= 0]
        # left_local = left_local[left_local[:, 0] >= 0]

        # Fit polynomials only if we have enough points
        if len(right_local) >= 2 and len(left_local) >= 2:
            p = min(3, right_local.shape[0] - 1)
            right_a = np.poly1d(np.polyfit(right_local[:, 0], right_local[:, 1], p))
            left_a = np.poly1d(np.polyfit(left_local[:, 0], left_local[:, 1], p))
        else:
            # Not enough points for polynomial fitting
            right_a = np.poly1d([0, 0, 0, 0])  # Zero polynomial
            left_a = np.poly1d([0, 0, 0, 0])  # Zero polynomial

        return {
            "right_a": right_a,
            "left_a": left_a,
            "right_boundary": right_local,  # Already in ROAD frame (vehicle local coordinates)
            "left_boundary": left_local,  # Already in ROAD frame (vehicle local coordinates)
        }

    def make_camera_image(self):
        """Extract camera image with custom position/orientation"""
        try:
            rgb_sensor = self.env.engine.get_sensor("rgb")
            if rgb_sensor is not None:
                img = rgb_sensor.perceive(
                    to_float=True,
                    new_parent_node=self.env.agent.origin,
                    position=self.camera_extrinsics["position"],
                    hpr=self.camera_extrinsics["hpr"],
                )
                if img is not None:
                    return img
        except Exception as e:
            return None

    def make_perception_data(self):
        """Get all observation data: odometry, lane polygons, and camera image"""
        agent = self.env.agent

        return {
            "odometry": self.make_odometry(agent),
            "lanes": self.make_lane_observations(agent),
            "camera_image": self.make_camera_image(),
        }


def make_transformation_matrix(yaw, pitch, roll, x, y, z):
    """
    Create a 4x4 transformation matrix from world frame to camera frame.

    Args:
        yaw: rotation around Z axis (degrees)
        pitch: rotation around Y axis (degrees)
        roll: rotation around X axis (degrees)
        x, y, z: translation in world frame (meters)

    Returns:
        4x4 transformation matrix from world to camera
    """
    # Convert to radians
    yaw_rad = np.deg2rad(yaw)
    pitch_rad = np.deg2rad(pitch)
    roll_rad = np.deg2rad(roll)

    # Compute sines and cosines
    cy, sy = np.cos(yaw_rad), np.sin(yaw_rad)
    cp, sp = np.cos(pitch_rad), np.sin(pitch_rad)
    cr, sr = np.cos(roll_rad), np.sin(roll_rad)

    # Rotation matrix (Z-Y-X order, which is standard for camera extrinsics)
    # This matches the convention in camera_geometry.py
    R = np.array(
        [
            [cr * cy + sp * sr * sy, cr * sp * sy - cy * sr, -cp * sy],
            [cp * sr, cp * cr, sp],
            [cr * sy - cy * sp * sr, -cr * cy * sp - sr * sy, cp * cy],
        ]
    )

    # Translation vector
    t = np.array([x, y, z])

    # Build 4x4 homogeneous transformation matrix
    T = np.eye(4)
    T[:3, :3] = R
    T[:3, 3] = t

    return T


class MetaDriveSimulator:
    """Simplified MetaDrive simulation with lane detection"""

    def __init__(self, config):
        setup_logger(True)
        self.env = MetaDriveEnv(config)
        self.controller = SimpleController()
        camera_extrinsics = {
            "position": [0.0, 0.8, 1.5],  # x, y, z
            "hpr": [0.0, -5.0, 0.0],  # yaw, pitch, roll
        }
        origin_to_camera = make_transformation_matrix(
            yaw=camera_extrinsics["hpr"][0],
            pitch=camera_extrinsics["hpr"][1],
            roll=camera_extrinsics["hpr"][2],
            x=camera_extrinsics["position"][0],
            y=camera_extrinsics["position"][1],
            z=camera_extrinsics["position"][2],
        )

        # Матрица поворота из системы координат дороги в систему координат камеры
        # Дорога: X-вперёд, Y-влево, Z-вверх
        # Камера: Z-вперёд, X-вправо, Y-вниз

        # Матрица поворота (3x3)
        R_road_to_cam = np.array([
            [0, -1,  0],   # X_cam = -Y_road (right = -left)
            [0,  0, -1],   # Y_cam = -Z_road (down = -up)
            [1,  0,  0]    # Z_cam =  X_road (forward = forward)
        ])
        
        # Комбинированная матрица поворота: world -> camera
        R_combined = origin_to_camera[:3, :3] @ R_road_to_cam
        
        # Формируем полную 4x4 матрицу Rt с трансляцией
        Rt = np.eye(4)
        Rt[:3, :3] = R_combined
        Rt[:3, 3] = origin_to_camera[:3, 3]
        
        self.camera_geometry = CameraGeometry(
            Rt=Rt,
            width=1200,
            height=900,
            fov_deg=65,
        )
        self.observation_parser = ObservationParser(self.env, camera_extrinsics)
        self.frame = 0

    def run(self):
        """Main simulation loop with full observation data"""
        try:
            obs, info = self.env.reset()

            while True:
                obs_data = self.observation_parser.make_perception_data()

                odometry = obs_data["odometry"]
                lane_polygons = obs_data["lanes"]
                camera_image = obs_data["camera_image"]

                action = self.controller.get_control(odometry["speed"], lane_polygons)

                # Visualize lanes
                if camera_image is not None and self.frame % 30 == 0:
                    # Convert to uint8 if needed
                    display_img = camera_image.copy()
                    if display_img.dtype != np.uint8:
                        display_img = (display_img * 255).astype(np.uint8)

                    # Project lane boundaries to image
                    print(f"Right boundary points: {lane_polygons['right_boundary']}")
                    print(f"Left boundary points: {lane_polygons['left_boundary']}")
                    right_points_cam = self.camera_geometry.world_to_camera(
                        lane_polygons["right_boundary"]
                    )
                    left_points_cam = self.camera_geometry.world_to_camera(
                        lane_polygons["left_boundary"]
                    )
                    print(f"Right points cam: {right_points_cam}")
                    print(f"Left points cam: {left_points_cam}")
                    # Filter valid points
                    valid_right = right_points_cam[
                        (right_points_cam[:, 0] >= 0)
                        & (right_points_cam[:, 0] < 1200)
                        & (right_points_cam[:, 1] >= 0)
                        & (right_points_cam[:, 1] < 900)
                    ]
                    valid_left = left_points_cam[
                        (left_points_cam[:, 0] >= 0)
                        & (left_points_cam[:, 0] < 1200)
                        & (left_points_cam[:, 1] >= 0)
                        & (left_points_cam[:, 1] < 900)
                    ]

                    if len(valid_right) > 1:
                        cv2.polylines(
                            display_img, [valid_right.astype(np.int32)], False, (0, 0, 255), 2
                        )
                    else:
                        print("No valid right points")
                    if len(valid_left) > 1:
                        cv2.polylines(
                            display_img, [valid_left.astype(np.int32)], False, (0, 255, 0), 2
                        )
                    else:
                        print("No valid left points")
                    print("saved frame")
                    cv2.imwrite(f"frame_{self.frame:05d}.jpg", display_img)

                if self.frame % 30 == 0:
                    # Print comprehensive info
                    print(f"Frame {self.frame}:")
                    print(
                        f"  Position: ({odometry['position'][0]:.2f}, {odometry['position'][1]:.2f})"
                    )
                    print(
                        f"  Speed: {odometry['speed']:.2f} m/s, Heading: {np.rad2deg(odometry['heading']):.1f}°"
                    )
                    print(
                        f"  Lane polygons: Left={len(lane_polygons['left_boundary'])}, Right={len(lane_polygons['right_boundary'])}"
                    )

                obs, reward, tm, tc, info = self.env.step(action)
                self.frame += 1

        except KeyboardInterrupt:
            print("\nSimulation interrupted by user")

        finally:
            self.cleanup()

    def cleanup(self):
        """Cleanup resources"""
        print("Closing environment...")
        self.env.close()
        print("Done.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run MetaDrive simulation")
    parser.add_argument(
        "--no-render", action="store_true", help="Disable rendering for faster simulation"
    )

    args = parser.parse_args()

    config = {
        "use_render": not args.no_render,
        "manual_control": False,
        "num_scenarios": 100,
        "start_seed": 42,
        "traffic_density": 0.1,
        "decision_repeat": 1,
        "physics_world_step_size": 0.01,
        "horizon": 1000000,
        "image_observation": True,  # Enable image observations
        "show_terrain": True,
        "sensors": dict(rgb=[RGBCamera, 1200, 900]),  # Create RGB camera sensor as in docs
        "vehicle_config": {
            "image_source": "rgb",  # Use the rgb sensor we created
        },
    }

    try:
        sim = MetaDriveSimulator(config)
        sim.run()
    except Exception as e:
        print(f"Error running simulation: {e}")
        import traceback

        traceback.print_exc()
