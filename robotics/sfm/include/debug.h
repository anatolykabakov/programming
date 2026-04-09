#pragma once

#include "utils.h"

class Debug {
public:
  Debug(Eigen::Isometry3d extrinsic);

  void visualize(int i, const cv::Mat& frame, const std::vector<cv::Point2f> prevFeatures,
                 const std::vector<cv::Point2f> currentFeatures, double scale, const Eigen::Isometry3d& relativePose,
                 const Eigen::Isometry3d& globalPose, const Eigen::Isometry3d& sfmPose, const Eigen::Isometry3d& gt,
                 const Eigen::Isometry3d& gtPrev);

  void saveImg(fs::path trajectoryImgPath);

  void publish(const openMVG::sfm::SfM_Data& scene);

  void publish(const Eigen::Isometry3d& pose, const std::string& type);

  void publish(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud);

private:
  cv::Mat trajectoryImg_;

  Eigen::Isometry3d extrinsic_;

  std::unique_ptr<CycloneDDS> dds_;

  std::unique_ptr<cv::VideoWriter> video_;
};
