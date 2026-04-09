#include "utils.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include "pcl/common/transforms.h"

#include "openMVG/numeric/eigen_alias_definition.hpp"
#include "openMVG/sfm/pipelines/sfm_robust_model_estimation.hpp"
#include "openMVG/multiview/triangulation.hpp"
#include <openMVG/sfm/sfm_data_filters.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>

#include "odom.h"

namespace fs = std::filesystem;

openMVG::geometry::Pose3 toPose3(const Eigen::Isometry3d& pose)
{
  openMVG::geometry::Pose3 poseVec3(pose.linear(), pose.translation());
  // openMVG::geometry::Pose3 poseVec3(pose.linear(), - pose.linear().inverse() * pose.translation());
  return poseVec3;
}

void progress_bar(int counter, int number, std::string prefix)
{
  const int total_signs_size = 60;
  const int current_percentage = counter * 100 / number;
  const int current_sign_num = current_percentage * total_signs_size / 100;

  std::cout << prefix << " [";
  for (int i = 0; i < total_signs_size; ++i) {
    if (i < current_sign_num) {
      std::cout << "=";
    } else if (i == current_sign_num) {
      std::cout << ">";
    } else {
      std::cout << "_";
    }
  }

  std::cout << "] " << current_percentage << "%\r";
  std::cout.flush();
}

Calibration getKittiCalibration(const std::string& path_to_calibfile)
{
  std::ifstream file(path_to_calibfile);
  std::string line;
  std::vector<std::string> v;
  std::string each;

  Calibration calib;

  if (file.is_open()) {
    while (std::getline(file, line)) {
      std::stringstream in(line);
      std::vector<std::string> v;
      std::string label;
      while (std::getline(in, each, ' ')) {
        if (label.empty()) {
          label = each;
          continue;
        }
        v.push_back(each);
      }

      if (label.empty()) {
        throw std::logic_error("Label not found!");
      }

      if (label == "P0:") {
        calib.intrinsic.fx = std::stod(v[0]);
        calib.intrinsic.fy = std::stod(v[5]);
        calib.intrinsic.cx = std::stod(v[2]);
        calib.intrinsic.cy = std::stod(v[6]);
      }

      if (label == "Tr:") {
        const int rows = 3;
        const int cols = 4;
        for (int i = 0; i < rows; ++i) {
          for (int j = 0; j < cols; ++j) {
            calib.extrinsic(i, j) = std::stod(v[(i * cols) + j]);
          }
        }
      }
    }

    file.close();
  } else {
    std::cerr << "Unable to open kitti calib.txt file!" << std::endl;
  }
  return calib;
}

// https://stackoverflow.com/questions/1263072/changing-a-matrix-from-right-handed-to-left-handed-coordinate-system
Eigen::Isometry3d convertRight2LeftCS(const Eigen::Isometry3d& pose)
{
  Eigen::Isometry3d right2left = Eigen::Isometry3d::Identity();
  Eigen::Matrix3d right2leftR;
  right2leftR << 1, 0, 0, 0, 0, 1, 0, 1, 0;
  right2left.linear() = right2leftR;
  return right2left * pose * right2left.inverse();
}

std::vector<Eigen::Isometry3d> parseGTFile(const std::string& path)
{
  std::string line;
  std::ifstream myfile(path);
  std::vector<Eigen::Isometry3d> gt;

  if (myfile.is_open()) {
    while ((getline(myfile, line))) {
      cv::Point3d p;
      std::istringstream in(line);

      Eigen::Matrix4d pose;

      std::array<double, 12> buffer;  // r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz right hand coordinate system
      for (int i = 0; i < 12; ++i) {
        in >> buffer[i];
      }

      int rows = 3;
      int cols = 4;
      for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
          pose(i, j) = buffer[i * 4 + j];
        }
      }

      Eigen::Isometry3d mat = Eigen::Isometry3d::Identity();
      mat.translation() = pose.block<3, 1>(0, 3);
      mat.linear() = pose.block<3, 3>(0, 0);

      gt.push_back(mat);
    }
    myfile.close();
  } else {
    throw std::runtime_error("Unable to open file");
  }

  return gt;
}

double getOdomScale(const Eigen::Vector3d& prev, const Eigen::Vector3d& curr)
{
  return std::sqrt((curr.x() - prev.x()) * (curr.x() - prev.x()) + (curr.y() - prev.y()) * (curr.y() - prev.y()) +
                   (curr.z() - prev.z()) * (curr.z() - prev.z()));
}

cv::Point convertToImagePoint(const Eigen::Isometry3d& poseRightHandCS, int height, int offset, double scale)
{
  const auto poseLeftHandCS = convertRight2LeftCS(poseRightHandCS);

  int x = offset + int(poseLeftHandCS.translation().x() * scale);
  int y = height - (offset + int(poseLeftHandCS.translation().y() * scale));
  return cv::Point2d(x, y);
}

Eigen::Isometry3d opencv2isometry3d(const cv::Mat& T, const cv::Mat& R)
{
  Eigen::Isometry3d mat = Eigen::Isometry3d::Identity();
  const auto t = Eigen::Vector3d(T.at<double>(0), T.at<double>(1), T.at<double>(2));
  mat.translation() = t;
  Eigen::Matrix3d rot;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      rot(i, j) = R.at<double>(i, j);
    }
  }

  mat.linear() = rot;
  return mat;
}

cv::Mat getP(const Eigen::Isometry3f& pose, CameraIntrinsic intrinsic)
{
  cv::Mat Rt(3, 4, CV_64F);
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      Rt.at<float>(i, j) = pose(i, j);
    }
  }
  Rt.at<float>(0, 3) = pose.translation().x();
  Rt.at<float>(1, 3) = pose.translation().y();
  Rt.at<float>(2, 3) = pose.translation().z();

  cv::Mat K =
      (cv::Mat_<float>(3, 3) << intrinsic.fx, 0.f, intrinsic.cx, 0.f, intrinsic.fy, intrinsic.cy, 0.f, 0.f, 1.f);
  cv::Mat Kd;
  K.convertTo(Kd, CV_64F);
  return Kd * Rt;
}

std::vector<Eigen::Vector3d> triangulateOpenCV(RelativePoseResult relative, CameraIntrinsic intrinsic,
                                               const std::vector<cv::Point2f>& prevFeatures,
                                               const std::vector<cv::Point2f>& currentFeatures)
{
  cv::Mat K =
      (cv::Mat_<float>(3, 3) << intrinsic.fx, 0.f, intrinsic.cx, 0.f, intrinsic.fy, intrinsic.cy, 0.f, 0.f, 1.f);
  cv::Mat Kd;
  K.convertTo(Kd, CV_64F);

  cv::Mat R = relative.R;
  cv::Mat t = relative.T;

  cv::Mat Rt0 = cv::Mat::eye(3, 4, CV_64FC1);
  cv::Mat Rt1 = cv::Mat::eye(3, 4, CV_64FC1);

  R.copyTo(Rt1.rowRange(0, 3).colRange(0, 3));
  t.copyTo(Rt1.rowRange(0, 3).col(3));

  std::vector<cv::Point2d> points1, points2;
  for (int i = 0; i < relative.poseMask.rows; ++i) {
    // if (relative.poseMask.at<unsigned char>(i)) {
    points1.push_back(cv::Point2d((double)prevFeatures[i].x, (double)prevFeatures[i].y));
    points2.push_back(cv::Point2d((double)currentFeatures[i].x, (double)currentFeatures[i].y));
    // }
  }

  cv::Mat point3d_homo;
  cv::triangulatePoints(Kd * Rt0, Kd * Rt1, points1, points2, point3d_homo);

  assert(point3d_homo.cols == points1.size());
  // std::cout << point3d_homo.cols << std::endl;

  std::vector<Eigen::Vector3d> points3d;
  for (int i = 0; i < point3d_homo.cols; i++) {
    pcl::PointXYZ point;
    cv::Mat p3d;
    cv::Mat _p3h = point3d_homo.col(i);
    cv::convertPointsFromHomogeneous(_p3h.t(), p3d);
    auto pt = Eigen::Vector3d(p3d.at<double>(0), p3d.at<double>(1), p3d.at<double>(2));
    points3d.push_back(pt);
  }

  return points3d;
}
