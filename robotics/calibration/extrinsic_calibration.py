#!/usr/bin/python3
import matplotlib.pyplot as plt
import numpy as np
import argparse
import cv2
import json


def calibrate_extrinsics(img, intrinsics, board_3d, pattern_params, undistord):
    chessboard_rows, chessboard_cols, _ = pattern_params
    mtx = np.asarray(intrinsics["K"])
    dist = np.asarray(intrinsics["D"])

    new_dist = dist
    if undistord:
        new_dist = np.zeros((5, 1))

    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    ret, corners = cv2.findChessboardCorners(gray, (chessboard_rows, chessboard_cols), None)

    if ret:
        board_2d = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
        # Find the rotation and translation vectors.
        ret, rotation_vector, translation_vector = cv2.solvePnP(board_3d, board_2d, mtx, new_dist)

        rotation_mat = np.zeros(shape=(3, 3))
        R = cv2.Rodrigues(rotation_vector, rotation_mat)[0]
        T = np.column_stack((R, translation_vector))
        return T

    return None


def generate_bev(img, intrinsics, extrinsics, dsize):
    mtx = np.asarray(intrinsics["K"])
    P = mtx @ extrinsics

    # Homography Matrix
    H = np.array(
        [
            P[0, 0],
            P[0, 1],
            P[0, 3],
            P[1, 0],
            P[1, 1],
            P[1, 3],
            P[2, 0],
            P[2, 1],
            P[2, 3],
        ]
    ).reshape(3, 3)

    return cv2.warpPerspective(img, np.linalg.inv(H), dsize)


def generate_chessboard(pattern_params):
    chessboard_rows, chessboard_cols, square_size = pattern_params
    board_3d = np.zeros((1, chessboard_rows * chessboard_cols, 3), np.float32)
    board_3d[0, :, :2] = (
        np.mgrid[0:chessboard_rows, 0:chessboard_cols].T.reshape(-1, 2) * square_size
    )
    return board_3d


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="")
    parser.add_argument(
        "intrinsic",
        type=str,
        default="chessboard/intrinsics.json",
        help="path to intrinsics",
    )
    parser.add_argument(
        "image", type=str, default="chessboard", help="path to images to calibrations"
    )
    parser.add_argument("--width", type=int, default=7, help="width of chessboard in squares")

    parser.add_argument("--height", type=int, default=6, help="width of chessboard in squares")
    parser.add_argument("--square-size", type=int, default=30, help="Size of square in mm")
    parser.add_argument("--rows", type=int, default=7, help="")
    parser.add_argument("--cols", type=int, default=6, help="")

    parser.add_argument("--undistord", action="store_true")
    args = parser.parse_args()

    with open(args.intrinsic, "r") as f:
        intrinsics = json.load(f)

    chessboard_params = (args.rows, args.cols, args.square_size)

    chessboard = generate_chessboard(chessboard_params)

    img = cv2.imread(args.image)

    K = np.asarray(intrinsics["K"])
    D = np.asarray(intrinsics["D"])
    newcammtx = np.asarray(intrinsics["KD"])

    dst = cv2.undistort(img, K, D, None, newcammtx)

    extrinsics = calibrate_extrinsics(
        dst, intrinsics, chessboard, chessboard_params, args.undistord
    )

    bev = generate_bev(
        dst,
        intrinsics,
        extrinsics,
        dsize=(args.rows * args.square_size, args.cols * args.square_size),
    )

    plt.subplot(121)
    plt.imshow(dst)
    plt.title("Input")
    plt.subplot(122)
    plt.imshow(bev)
    plt.title("Output")
    plt.show()
