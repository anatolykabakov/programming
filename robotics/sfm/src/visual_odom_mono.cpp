#include "visual_odom_mono.h"

#include <filesystem>
#include <iostream>
#include <fstream>

#include "openMVG/numeric/eigen_alias_definition.hpp"
#include "openMVG/sfm/pipelines/sfm_robust_model_estimation.hpp"

namespace fs = std::filesystem;

Odometry::Odometry(const Eigen::Isometry3d& init) : globalPose_{init} {}

void Odometry::update(const Eigen::Isometry3d& relative) { globalPose_ = globalPose_ * relative; }

Eigen::Isometry3d Odometry::pose() { return globalPose_; }

RelativePoseProviderOpenCV::RelativePoseProviderOpenCV(const CameraIntrinsic& intinsics)
  : intrinsics_(intinsics), pp_(cv::Point2d(intinsics.cx, intinsics.cy))
{
}

Eigen::Isometry3d RelativePoseProviderOpenCV::handle(const std::vector<cv::Point2f>& prev,
                                                     const std::vector<cv::Point2f>& current)
{
  const auto E = cv::findEssentialMat(current, prev, intrinsics_.fx, pp_, cv::RANSAC, 0.999, 1.0);
  cv::Mat relativeRot, relativeT;
  cv::recoverPose(E, current, prev, relativeRot, relativeT, intrinsics_.fx, pp_);
  return opencv2isometry3d(relativeT, relativeRot);
}

RelativePoseProviderOpenMVG::RelativePoseProviderOpenMVG(const CameraIntrinsic& intrinsic)
  : intrinsics_(std::make_unique<openMVG::cameras::Pinhole_Intrinsic>(intrinsic.width, intrinsic.height, intrinsic.fx,
                                                                      intrinsic.cx, intrinsic.cy))
{
}

Eigen::Isometry3d RelativePoseProviderOpenMVG::handle(const std::vector<cv::Point2f>& prev,
                                                      const std::vector<cv::Point2f>& current)
{
  openMVG::Mat2X xL(2, prev.size()), xR(2, prev.size());
  for (int featureId = 0; featureId < prev.size(); ++featureId) {
    xR.col(featureId) = openMVG::Vec2(prev[featureId].x, prev[featureId].y).cast<double>();
    xL.col(featureId) = openMVG::Vec2(current[featureId].x, current[featureId].y).cast<double>();
  }
  openMVG::sfm::RelativePose_Info relativePose_info;
  relativePose_info.initial_residual_tolerance = openMVG::Square(2.5);
  if (!openMVG::sfm::robustRelativePose(intrinsics_.get(), intrinsics_.get(), xL, xR, relativePose_info,
                                        {intrinsics_->w(), intrinsics_->h()}, {intrinsics_->w(), intrinsics_->h()},
                                        2048)) {
    std::cout << "Robust relative pose estimation failure." << std::endl;
  }

  Eigen::Isometry3d relative = Eigen::Isometry3d::Identity();
  relative.translation() = relativePose_info.relativePose.translation();
  relative.linear() = relativePose_info.relativePose.rotation();
  return relative;
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

CameraIntrinsic getKittiCameraIntrinsic(const std::string& path_to_calibfile)
{
  std::ifstream file(path_to_calibfile);
  std::string line;
  std::vector<std::string> v;
  std::string each;
  CameraIntrinsic intrinsic;

  if (file.is_open()) {
    while (std::getline(file, line)) {
      std::stringstream stream(line);
      std::vector<std::string> v;
      while (std::getline(stream, each, ' ')) {
        v.push_back(each);
      }

      if (v[0] == "P0:") {
        intrinsic.fx = std::stod(v[1]);
        intrinsic.cx = std::stod(v[3]);
        intrinsic.fy = std::stod(v[6]);
        intrinsic.cy = std::stod(v[7]);
      }
    }

    file.close();
  } else {
    std::cerr << "Unable to open kitti calib.txt file!" << std::endl;
  }
  return intrinsic;
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

void getRidFailedFeatures(std::vector<cv::Point2f>& points1, std::vector<cv::Point2f>& points2,
                          std::vector<uchar>& status)
{
  int indexCorrection = 0;
  for (int i = 0; i < status.size(); i++) {
    cv::Point2f pt = points2.at(i - indexCorrection);
    if ((status.at(i) == 0) || (pt.x < 0) || (pt.y < 0)) {
      if ((pt.x < 0) || (pt.y < 0)) {
        status.at(i) = 0;
      }

      points1.erase(points1.begin() + i - indexCorrection);
      points2.erase(points2.begin() + i - indexCorrection);
      indexCorrection++;
    }
  }
}

MonoVO::MonoVO(CameraIntrinsic intrinsic, std::unique_ptr<IRelativePoseProvider> relativeProvider, int featuresNumber)
  : intrinsic_(std::move(intrinsic))
  , relativeProvider_(std::move(relativeProvider))
  , pp_(cv::Point2d(intrinsic.cx, intrinsic.cy))
  , featuresNumber_(featuresNumber)
{
  detector_ = cv::GFTTDetector::create(featuresNumber, 0.01, 0.0);
}

Eigen::Isometry3d MonoVO::handle(const cv::Mat& frame)
{
  if (prevImg.empty()) {
    prevImg = frame.clone();

    detector_->detect(prevImg, keypoints);
    cv::KeyPoint::convert(keypoints, prevFeatures_, std::vector<int>());
    return Eigen::Isometry3d::Identity();
  }
  cv::Mat currentImg = frame.clone();

  cv::calcOpticalFlowPyrLK(prevImg, currentImg, prevFeatures_, currFeatures_, status, error, cv::Size(21, 21), 3);
  getRidFailedFeatures(prevFeatures_, currFeatures_, status);

  if (currFeatures_.size() < featuresNumber_ * 0.8) {
    detector_->detect(prevImg, keypoints);
    cv::KeyPoint::convert(keypoints, prevFeatures_, std::vector<int>());

    cv::calcOpticalFlowPyrLK(prevImg, currentImg, prevFeatures_, currFeatures_, status, error, cv::Size(21, 21), 3);
    getRidFailedFeatures(prevFeatures_, currFeatures_, status);
  }

  const auto relative = relativeProvider_->handle(prevFeatures_, currFeatures_);

  prevImg = currentImg.clone();
  prevFeaturesSaved_ = prevFeatures_;
  prevFeatures_ = currFeatures_;
  return relative;
}
