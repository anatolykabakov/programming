#pragma once
#include "utils.h"

class Mapping {
public:
  Mapping(const CameraIntrinsic& intrinsic);

  void update(const Eigen::Isometry3d& prevPose, const Eigen::Isometry3d& currentPose,
              const std::vector<cv::Point2f> prevFeatures, const std::vector<cv::Point2f> currentFeatures,
              const std::vector<int> indexes, int poseId);

  void save(fs::path sceneBeforePath);

  void adjust();

  openMVG::sfm::SfM_Data scene() { return scene_; }

  Eigen::Isometry3d pose(int id);

private:
  openMVG::sfm::SfM_Data scene_;

  std::unordered_map<int, openMVG::Vec3> triangulate(const Eigen::Isometry3d& relative,
                                                     const std::vector<cv::Point2f>& prevFeatures,
                                                     const std::vector<cv::Point2f>& currentFeatures,
                                                     const std::vector<int> indexes,
                                                     std::shared_ptr<openMVG::cameras::IntrinsicBase> intrinsics);

  void filter();
};
