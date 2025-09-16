#! /usr/bin/env python3

import numpy as np


class ZupRightHandCS:
    def __init__(self):
        self.R = np.array([[0, 0, 1], [0, 1, 0], [-1, 0, 0]])

    def transform(self, x, y, z, yaw_deg, pitch_deg, roll_deg):
        T = np.eye(4)
        T[:3, :3] = self.rotation_zyx(yaw_deg, pitch_deg, roll_deg)
        T[:3, 3] = np.array([x, y, z])
        return T

    def rotation_zyx(self, yaw_deg, pitch_deg, roll_deg):
        return self.rotation_z(yaw_deg) @ self.rotation_y(pitch_deg) @ self.rotation_x(roll_deg)

    def rotation_x(self, angle_deg):
        """Поворот вокруг оси X (roll)"""
        angle_rad = np.deg2rad(angle_deg)
        return np.array(
            [
                [1, 0, 0],
                [0, np.cos(angle_rad), -np.sin(angle_rad)],
                [0, np.sin(angle_rad), np.cos(angle_rad)],
            ]
        )

    def rotation_y(self, angle_deg):
        """Поворот вокруг оси Y (pitch)"""
        angle_rad = np.deg2rad(angle_deg)
        return np.array(
            [
                [np.cos(angle_rad), 0, np.sin(angle_rad)],
                [0, 1, 0],
                [-np.sin(angle_rad), 0, np.cos(angle_rad)],
            ]
        )

    def rotation_z(self, angle_deg):
        """Поворот вокруг оси Z (yaw)"""
        angle_rad = np.deg2rad(angle_deg)
        return np.array(
            [
                [np.cos(angle_rad), -np.sin(angle_rad), 0],
                [np.sin(angle_rad), np.cos(angle_rad), 0],
                [0, 0, 1],
            ]
        )

class CameraParams:
    def __init__(self, env):
        rgb_camera = env.engine.get_sensor("rgb")
        self.fov = rgb_camera.lens.getFov()
        self.width = rgb_camera.BUFFER_W
        self.height = rgb_camera.BUFFER_H
        self.intrinsics = self.make_intrinsics(self.fov, self.width, self.height)

        hpr = rgb_camera.cam.getHpr(env.agent.origin)
        xyz = rgb_camera.cam.getPos(env.engine.render)
        self.extrinsics = self.make_extrinsics(xyz, hpr)

    def get_fov(self):
        return self.fov
    
    def get_width(self):
        return self.width
    
    def get_height(self):
        return self.height

    def get_intrinsics(self):
        return self.intrinsics
    
    def get_extrinsics(self):
        return self.extrinsics

    def make_intrinsics(self, fov, width, height):
        f_x = (width / 2) / (np.tan(fov[0] / 2 / 180 * np.pi))
        f_y = (height / 2) / (np.tan(fov[1] / 2 / 180 * np.pi))
        u0 = width / 2
        v0 = height / 2
        return np.array([[f_x, 0, u0], [0, f_y, v0], [0, 0, 1]])

    def make_extrinsics(self, xyz, hpr):
        zup_right_hand_cs = ZupRightHandCS()
        height = xyz[2]
        pitch_deg = hpr[1]
        T = zup_right_hand_cs.transform(
            x=0.0, y=0.0, z=-height,
            yaw_deg=hpr[0], pitch_deg=pitch_deg, roll_deg=hpr[2])

        RoadCSToCameraCS = np.array(
            [
                [0, -1, 0, 0],  # Xc = -Yr
                [0, 0, -1, 0],  # Yc = -Zr
                [1, 0, 0, 0],  # Zc = Xr
                [0, 0, 0, 1],
            ]
        )

        W2C = RoadCSToCameraCS @ T
        return W2C


class CameraGeometry:
    def __init__(self, K, Rt):
        self.K = K
        self.Rt = Rt

    def xyz_to_uv(self, xyz):
        xyz_homo = np.stack(
            (xyz[:, 0], xyz[:, 1], xyz[:, 2], np.ones_like(xyz[:, 0]))
        )  # Shape: (4, N)
        uvw = self.K @ self.Rt[:3, :] @ xyz_homo
        u = uvw[0, :] / uvw[2, :]
        v = uvw[1, :] / uvw[2, :]
        return np.stack((u, v)).T

    def uv_to_xyz(self, uv):
        uv_coords = np.stack([uv[:, 0], uv[:, 1], np.ones_like(uv[:, 0])])  # Shape: (N, 3)
        K_inv = np.linalg.inv(self.K)
        xyz_camera = (K_inv @ uv_coords.T).T  # Shape: (3, N)
        Rt_inv = np.linalg.inv(self.Rt)
        xyz_world = Rt_inv[:3, :] @ xyz_camera
        return xyz_world

