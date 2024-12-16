#pragma once

#include "utils.h"

class Odometry {
public:
  Odometry(const Eigen::Isometry3d& init);

  void update(const Eigen::Isometry3d& relative);

  Eigen::Isometry3d pose();

private:
  Eigen::Isometry3d globalPose_{Eigen::Isometry3d::Identity()};
};

struct RelativePoseResult {
  cv::Mat T;
  cv::Mat R;
  cv::Mat essMask;
  cv::Mat poseMask;
  Eigen::Isometry3d pose{Eigen::Isometry3d::Identity()};
};

class IRelativePoseProvider {
public:
  virtual RelativePoseResult handle(const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& current) = 0;
  virtual ~IRelativePoseProvider() = default;
};

class RelativePoseProviderOpenCV : public IRelativePoseProvider {
public:
  RelativePoseProviderOpenCV(const CameraIntrinsic& intinsics);

  RelativePoseResult handle(const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& current) override;

private:
  CameraIntrinsic intrinsics_;
  cv::Point2d pp_;
};

class RelativePoseProviderOpenMVG : public IRelativePoseProvider {
public:
  RelativePoseProviderOpenMVG(const CameraIntrinsic& intinsics);

  RelativePoseResult handle(const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& current) override;

private:
  cv::Point2d pp_;
  std::unique_ptr<openMVG::cameras::Pinhole_Intrinsic> intrinsics_;
};
