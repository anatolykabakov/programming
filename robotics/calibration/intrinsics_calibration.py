#!/usr/bin/python3

import numpy as np
import cv2 as cv
import glob
import argparse
import os
import json


def calibrate_chessboard(images_path, rows=7, cols=6, square_size=30, verbose=False):
    criteria = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 30, 0.001)
    # prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
    objp = np.zeros((rows * cols, 3), np.float32)
    objp[:, :2] = np.mgrid[0:rows, 0:cols].T.reshape(-1, 2) * square_size

    # Arrays to store object points and image points from all the images.
    objpoints = []  # 3d point in real world space
    imgpoints = []  # 2d points in image plane.
    images = glob.glob(os.path.join(images_path, "*.jpg"))
    for fname in images:
        img = cv.imread(fname)
        gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
        # Find the chess board corners
        ret, corners = cv.findChessboardCorners(gray, (rows, cols), None)
        # If found, add object points, image points (after refining them)
        if ret == True:
            objpoints.append(objp)
            corners2 = cv.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
            imgpoints.append(corners)
            # Draw and display the corners
            if verbose:
                cv.drawChessboardCorners(img, (rows, cols), corners2, ret)
                cv.imshow("img", img)
                cv.waitKey(500)

    cv.destroyAllWindows()

    ret, mtx, dist, rvecs, tvecs = cv.calibrateCamera(
        objpoints, imgpoints, gray.shape[::-1], None, None
    )

    img = cv.imread(os.path.join(images_path, "left12.jpg"))
    h, w = img.shape[:2]
    newcameramtx, _ = cv.getOptimalNewCameraMatrix(mtx, dist, (w, h), 1, (w, h))

    mean_error = 0
    for i in range(len(objpoints)):
        imgpoints2, _ = cv.projectPoints(objpoints[i], rvecs[i], tvecs[i], mtx, dist)
        error = cv.norm(imgpoints[i], imgpoints2, cv.NORM_L2) / len(imgpoints2)
        mean_error += error
    mean_error = mean_error / len(objpoints)

    calib = {}
    calib["K"] = mtx.tolist()
    calib["D"] = dist.tolist()
    calib["KD"] = newcameramtx.tolist()
    return calib, mean_error


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="")
    parser.add_argument(
        "images", type=str, default="chessboard", help="path to images to calibrations"
    )
    parser.add_argument(
        "--out",
        type=str,
        default="chessboard/intrinsics.json",
        help="path to intrinsics calibration",
    )
    parser.add_argument("--square-size", type=float, default=30, help="Size of square in mm")
    parser.add_argument("--rows", type=int, default=7, help="")
    parser.add_argument("--cols", type=int, default=6, help="")

    args = parser.parse_args()

    calib, err = calibrate_chessboard(args.images, args.rows, args.cols, args.square_size)

    print("reproject error: {}".format(err))

    with open(args.out, "w") as outfile:
        json.dump(calib, outfile, indent=4, sort_keys=True)
