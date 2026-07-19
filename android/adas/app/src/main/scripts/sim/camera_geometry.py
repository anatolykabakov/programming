"""
Camera geometry for projecting lane polygons onto camera image
"""

import numpy as np
import numpy as np
import matplotlib.pyplot as plt
import cv2
import os


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


def carla_test():
    data_path = os.path.join(os.path.dirname(__file__), "carla")
    image_path = os.path.join(data_path, "frame.png")
    image = cv2.imread(image_path)
    print(image.shape)
    rows, cols = image.shape[:2]

    fov = (45, 45)
    u0 = cols / 2
    v0 = rows / 2
    f_x = (cols / 2) / (np.tan(fov[0] / 2 / 180 * np.pi))
    intrinsics = np.array([[f_x, 0, u0], [0, f_x, v0], [0, 0, 1]])

    boundary_gt = np.loadtxt(os.path.join(data_path, "left_line.txt"))
    world_to_cam = np.loadtxt(os.path.join(data_path, "world_to_cam.txt"))

    cg = CameraGeometry(K=intrinsics, Rt=world_to_cam)
    left_line = boundary_gt[:, 0:3]
    uv = cg.xyz_to_uv(left_line)
    u, v = uv[:, 0], uv[:, 1]
    plt.plot(u, v)
    plt.imshow(image)
    plt.show()


def metadrive_test():
    data_path = os.path.join(os.path.dirname(__file__), "metasim")
    image_path = os.path.join(data_path, "frame_00030.jpg")
    left_line = np.loadtxt(os.path.join(data_path, "frame_00030_left_line.txt"))
    right_line = np.loadtxt(os.path.join(data_path, "frame_00030_right_line.txt"))
    intrinsics = np.loadtxt(os.path.join(data_path, "intrinsics.txt"))
    extrinsics = np.loadtxt(os.path.join(data_path, "extrinsics.txt"))

    right_line = np.stack((right_line[:, 0], right_line[:, 1], np.zeros(len(right_line[:, 0])))).T
    left_line = np.stack((left_line[:, 0], left_line[:, 1], np.zeros(len(left_line[:, 0])))).T

    image = cv2.imread(image_path)
    cg = CameraGeometry(K=intrinsics, Rt=extrinsics)
    left_uv = cg.xyz_to_uv(left_line)
    right_uv = cg.xyz_to_uv(right_line)
    plt.plot(left_uv[:, 0], left_uv[:, 1], "ro")
    plt.plot(right_uv[:, 0], right_uv[:, 1], "bo")
    plt.imshow(image)
    plt.show()


if __name__ == "__main__":
    # carla_test()
    metadrive_test()
