#include "odom.h"
#include "utils.h"

Odometry::Odometry(const Eigen::Isometry3d& init) : globalPose_{init} {}

void Odometry::update(const Eigen::Isometry3d& relative) { globalPose_ = globalPose_ * relative; }

Eigen::Isometry3d Odometry::pose() { return globalPose_; }

RelativePoseProviderOpenCV::RelativePoseProviderOpenCV(const CameraIntrinsic& intinsics)
  : intrinsics_(intinsics), pp_(cv::Point2d(intinsics.cx, intinsics.cy))
{
}

RelativePoseResult RelativePoseProviderOpenCV::handle(const std::vector<cv::Point2f>& prev,
                                                      const std::vector<cv::Point2f>& current)
{
  RelativePoseResult result;

  const auto E = cv::findEssentialMat(prev, current, intrinsics_.fx, pp_, cv::RANSAC, 0.999, 1.0, result.essMask);
  cv::Mat relativeRot, relativeT;
  cv::recoverPose(E, prev, current, relativeRot, relativeT, intrinsics_.fx, pp_, result.poseMask);
  result.R = relativeRot;
  result.T = relativeT;
  result.pose = opencv2isometry3d(relativeT, relativeRot).inverse();
  return result;
}

RelativePoseProviderOpenMVG::RelativePoseProviderOpenMVG(const CameraIntrinsic& intrinsic)
  : intrinsics_(std::make_unique<openMVG::cameras::Pinhole_Intrinsic>(intrinsic.width, intrinsic.height, intrinsic.fx,
                                                                      intrinsic.cx, intrinsic.cy))
{
}

RelativePoseResult RelativePoseProviderOpenMVG::handle(const std::vector<cv::Point2f>& prev,
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
  RelativePoseResult result;
  result.pose = relative;
  return result;
}
