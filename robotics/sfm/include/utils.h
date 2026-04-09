#pragma once
#include "dds.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/calib3d.hpp>

#include "openMVG/cameras/Camera_Pinhole.hpp"
#include "openMVG/cameras/Camera_Pinhole_Radial.hpp"
#include "openMVG/features/feature.hpp"
#include "openMVG/features/sift/SIFT_Anatomy_Image_Describer.hpp"
#include "openMVG/features/svg_features.hpp"
#include "openMVG/geometry/pose3.hpp"
#include "openMVG/image/image_io.hpp"
#include "openMVG/image/image_concat.hpp"
#include "openMVG/matching/indMatchDecoratorXY.hpp"
#include "openMVG/matching/regions_matcher.hpp"
#include "openMVG/matching/svg_matches.hpp"
#include "openMVG/multiview/triangulation.hpp"
#include "openMVG/numeric/eigen_alias_definition.hpp"
#include "openMVG/sfm/pipelines/sfm_robust_model_estimation.hpp"
#include "openMVG/sfm/sfm_data.hpp"
#include "openMVG/sfm/sfm_data_BA.hpp"
#include "openMVG/sfm/sfm_data_BA_ceres.hpp"
#include "openMVG/sfm/sfm_data_io.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <filesystem>
#include <iostream>
#include <fstream>

#include "openMVG/cameras/Camera_Pinhole.hpp"

const auto white = cv::Scalar(255, 255, 255);
const auto red = cv::Scalar(0, 0, 255);
const auto blue = cv::Scalar(255, 0, 0);
const auto green = cv::Scalar(0, 255, 0);

const int offset = 300;
const double imgScale = 2.;

namespace fs = std::filesystem;

struct CameraIntrinsic {
  double fx{0.0};
  double fy{0.0};
  double cx{0.0};
  double cy{0.0};
  int width{0};
  int height{0};
};

struct Calibration {
  CameraIntrinsic intrinsic;
  Eigen::Isometry3d extrinsic;
};

openMVG::geometry::Pose3 toPose3(const Eigen::Isometry3d& pose);

Calibration getKittiCalibration(const std::string& path_to_calibfile);

// https://stackoverflow.com/questions/1263072/changing-a-matrix-from-right-handed-to-left-handed-coordinate-system
Eigen::Isometry3d convertRight2LeftCS(const Eigen::Isometry3d& pose);

std::vector<Eigen::Isometry3d> parseGTFile(const std::string& path);

double getOdomScale(const Eigen::Vector3d& prev, const Eigen::Vector3d& curr);

cv::Point convertToImagePoint(const Eigen::Isometry3d& poseRightHandCS, int height, int offset, double scale);

Eigen::Isometry3d opencv2isometry3d(const cv::Mat& T, const cv::Mat& R);

void progress_bar(int counter, int number, std::string prefix = "");
