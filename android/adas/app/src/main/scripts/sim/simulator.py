"""
MetaDrive Simulation for ADAS Lane Detection and Control
"""

import argparse
from metadrive.component.sensors.rgb_camera import RGBCamera
import numpy as np
import cv2
import sys
import os
from panda3d.core import Point3


try:
    from metadrive.envs.metadrive_env import MetaDriveEnv
    from metadrive.utils import setup_logger
    from metadrive.constants import DEFAULT_SENSOR_OFFSET, DEFAULT_SENSOR_HPR
    from metadrive.component.sensors.rgb_camera import RGBCamera
    from metadrive.component.sensors.point_cloud_lidar import euler_to_rotation_matrix
except ImportError as e:
    print(f"Error importing MetaDrive: {e}")
    print("Make sure MetaDrive is installed: pip install metadrive")
    sys.exit(1)

# from camera_geometry import CameraGeometry
from observation_parser import ObservationParser
from controller import SimpleController
from camera_utils import CameraParams, CameraGeometry

def save_data(img, obs_data, frame):
    np.savetxt(f"frame_{frame:05d}_left_line_road.txt", obs_data["lanes"]["left_road"])
    np.savetxt(f"frame_{frame:05d}_right_line_road.txt", obs_data["lanes"]["right_road"])
    np.savetxt(f"frame_{frame:05d}_left_line_world.txt", obs_data["lanes"]["left_world"])
    np.savetxt(f"frame_{frame:05d}_right_line_world.txt", obs_data["lanes"]["right_world"])
    img = img.copy()
    if img.dtype != np.uint8:
        img = (img * 255).astype(np.uint8)
    cv2.imwrite(f"frame_{frame:05d}.jpg", img)


class MetaDriveSimulator:
    """Simplified MetaDrive simulation with lane detection"""

    def __init__(self, config):
        setup_logger(True)
        self.env = MetaDriveEnv(config)
        self.env.reset()

        self.camera_params = CameraParams(self.env)
        self.camera_geometry = CameraGeometry(self.camera_params.intrinsics, self.camera_params.extrinsics)
        np.savetxt("intrinsics.txt", self.camera_params.intrinsics)
        np.savetxt("extrinsics.txt", self.camera_params.extrinsics)

        # Transform relative to world origin (like get_relative_point)
        # cam_to_world = rgb_camera.cam.getMat(self.env.engine.render)
        # np.savetxt("cam_to_world.txt", cam_to_world)
        rgb_camera = self.env.engine.get_sensor("rgb")
        cam_pos_world = np.asarray(rgb_camera.cam.getPos(self.env.engine.render))
        cam_hpr_world = rgb_camera.cam.getHpr(self.env.engine.render)

        # # Compare with get_relative_point
        p = np.asarray(self.env.engine.render.get_relative_point(rgb_camera.cam, Point3(0, 0, 0)))
        print(f"Camera relative to world (like get_relative_point): {p}")

        # print(f"Camera relative to world (like get_relative_point):")
        # print(f"  Position from getMat: [{cam_pos_world[0]:.3f}, {cam_pos_world[1]:.3f}, {cam_pos_world[2]:.3f}]")
        # print(f"  Position from get_relative_point: [{p[0]:.3f}, {p[1]:.3f}, {p[2]:.3f}]")
        # print(f"  Transform matrix: {cam_to_world}")
        # print(f"\nCamera relative to agent (vehicle-centric):")
        # print(f"  Transform matrix: {cam_to_vehicle}")
        # print(f"  Position: [{cam_pos_rel_agent[0]:.3f}, {cam_pos_rel_agent[1]:.3f}, {cam_pos_rel_agent[2]:.3f}]")
        # print(f"  HPR: [{cam_hpr_rel_agent[0]:.3f}, {cam_hpr_rel_agent[1]:.3f}, {cam_hpr_rel_agent[2]:.3f}]")
        # np.savetxt("extrinsics.txt", cam_to_vehicle)
        # np.savetxt("cam_xyz.txt", np.array([cam_pos_rel_agent[0], cam_pos_rel_agent[1], cam_pos_rel_agent[2]]))
        # np.savetxt("cam_hpr.txt", np.array([cam_hpr_rel_agent[0], cam_hpr_rel_agent[1], cam_hpr_rel_agent[2]]))
        # np.savetxt("fov.txt", np.array([fov[0], fov[1]]))

        # self.camera_geometry = CameraGeometry(
        #     width=rgb_camera.BUFFER_W, height=rgb_camera.BUFFER_H, fov=fov
        # )
        # self.camera_geometry.set_extrinsics(cam_to_vehicle)
        # np.savetxt("intrinsics.txt", self.camera_geometry.K)

        self.observation_parser = ObservationParser(self.env)
        self.controller = SimpleController()
        self.frame = 0

    def run(self):
        """Main simulation loop with full observation data"""
        try:
            # obs, info = self.env.reset()

            while True:
                obs_data = self.observation_parser.make_perception_data()

                odometry = obs_data["odometry"]
                lane_polygons = obs_data["lanes"]
                camera_image = obs_data["camera_image"]

                action = self.controller.get_control(odometry["speed"], lane_polygons)

                # Visualize lanes
                if camera_image is not None and self.frame % 30 == 0:
                    # Convert to uint8 if needed

                    # right_points_cam = self.camera_geometry.road_to_camera(
                    #     lane_polygons["right_road"]
                    # )
                    # left_points_cam = self.camera_geometry.road_to_camera(
                    #     lane_polygons["left_road"]
                    # )

                    save_data(camera_image, obs_data, self.frame)

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
                        f"  Lane polygons: Left={len(lane_polygons['left_road'])}, Right={len(lane_polygons['right_road'])}"
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


def main():
    """Main entry point"""
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
        "sensors": dict(rgb=[RGBCamera, 640, 480]),  # Create RGB camera sensor as in docs
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


if __name__ == "__main__":
    main()
