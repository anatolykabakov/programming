#include "debug.h"
#include "utils.h"

Debug::Debug(Eigen::Isometry3d extrinsic) : dds_(std::make_unique<CycloneDDS>(extrinsic)), extrinsic_(extrinsic)
{
  trajectoryImg_ = cv::Mat(1200, 1200, CV_8UC3, white);
}

void Debug::visualize(int i, const cv::Mat& frame, const std::vector<cv::Point2f> prevFeatures,
                      const std::vector<cv::Point2f> currentFeatures, double scale,
                      const Eigen::Isometry3d& relativePose, const Eigen::Isometry3d& globalPose,
                      const Eigen::Isometry3d& sfmPose, const Eigen::Isometry3d& gt, const Eigen::Isometry3d& gtPrev)
{
  const auto posePoint = convertToImagePoint(globalPose, trajectoryImg_.size().height, offset, imgScale);
  const auto gtPoint = convertToImagePoint(gt, trajectoryImg_.size().height, offset, imgScale);
  const auto sfmPoint = convertToImagePoint(sfmPose, trajectoryImg_.size().height, offset, imgScale);

  cv::circle(trajectoryImg_, posePoint, 1, green, 2);
  cv::circle(trajectoryImg_, gtPoint, 1, red, 2);
  cv::circle(trajectoryImg_, sfmPoint, 1, blue, 2);

  const auto gtRelative = gtPrev.inverse() * gt;
  std::cout << std::endl;
  std::cout << "frame: " << i << std::endl;
  std::cout << "scale: " << scale << std::endl;
  std::cout << "rel norm: " << relativePose.translation().norm() << std::endl;
  // std::cout << "currFeatures size " << currFeatures.size() << std::endl;
  std::cout << "relative gt t " << gtRelative.translation().transpose().matrix() << " R "
            << gtRelative.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
  std::cout << "relative t " << relativePose.translation().transpose().matrix() << " R "
            << relativePose.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
  std::cout << "odom t " << globalPose.translation().transpose().matrix() << " R "
            << globalPose.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
  std::cout << "gt t " << gt.translation().transpose().matrix() << " R "
            << gt.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
  std::cout << std::endl;
  // cv::imshow("trajectoryImg", trajectoryImg);
  // cv::imshow("currentImg", frame);
  // cv::waitKey(0);
  if (!video_) {
    video_ = std::make_unique<cv::VideoWriter>("out.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 10, frame.size(),
                                               true);
  }
  for (int i = 0; i < prevFeatures.size(); ++i) {
    const auto prev = prevFeatures[i];
    const auto curr = currentFeatures[i];
    cv::circle(frame, prev, 2, red, -1);
    cv::circle(frame, curr, 2, green, -1);
    cv::line(frame, prev, curr, blue, 1);
  }
  video_->write(frame);
  dds_->publish(frame);
}

void Debug::saveImg(fs::path trajectoryImgPath) { cv::imwrite(trajectoryImgPath, trajectoryImg_); }

void Debug::publish(const openMVG::sfm::SfM_Data& scene) { dds_->publish(scene); }

void Debug::publish(const Eigen::Isometry3d& pose, const std::string& type) { dds_->publish(pose, type); }

void Debug::publish(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud) { dds_->publish(cloud); }
