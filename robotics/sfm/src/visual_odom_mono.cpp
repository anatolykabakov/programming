#include "visual_odom_mono.h"

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

const auto white = cv::Scalar(255, 255, 255);
const auto red = cv::Scalar(0, 0, 255);
const auto blue = cv::Scalar(255, 0, 0);
const auto green = cv::Scalar(0, 255, 0);

const int offset = 500;
const double imgScale = 6.;

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
  const auto E = cv::findEssentialMat(prev, current, intrinsics_.fx, pp_, cv::RANSAC, 0.999, 1.0);
  cv::Mat relativeRot, relativeT;
  cv::recoverPose(E, prev, current, relativeRot, relativeT, intrinsics_.fx, pp_);
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

openMVG::geometry::Pose3 toPose3(const Eigen::Isometry3d& pose)
{
  // Eigen::Vector3d center = -pose.linear().inverse() * pose.translation();
  // openMVG::geometry::Pose3 poseVec3(pose.linear(), center);
  openMVG::geometry::Pose3 poseVec3(pose.linear(), pose.translation());
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

void TrackerOpenCV::getRidFailedFeatures(std::vector<cv::Point2f>& points1, std::vector<cv::Point2f>& points2,
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
      indexes_.erase(indexes_.begin() + i - indexCorrection);

      indexCorrection++;
    }
  }
}

void TrackerOpenCV::spatialFilter(const cv::Mat frame, const std::vector<cv::Point2f>& prev,
                                  const std::vector<cv::Point2f>& curr, std::vector<uchar>& status)
{
  std::vector<double> norms;

  for (int i = 0; i < status.size(); ++i) {
    if (status[i]) {
      const auto norm = std::sqrt(std::pow(curr[i].x - prev[i].x, 2) + std::pow(curr[i].y - prev[i].y, 2));
      norms.push_back(norm);
      const auto threshold = 40.0;
      if (norm > threshold) {
        status[i] = 0;
      }
    }
  }

  std::cout << std::endl;
}

Pipeline::Pipeline(const CameraIntrinsic& intrinsic)
{
  scene_.intrinsics[0] = std::make_shared<openMVG::cameras::Pinhole_Intrinsic>(
      intrinsic.width, intrinsic.height, intrinsic.fx, intrinsic.cx, intrinsic.cy);
}

void Pipeline::filter()
{
  const size_t pointcount_initial = scene_.structure.size();
  openMVG::sfm::RemoveOutliers_PixelResidualError(scene_, 4.0);
  const size_t pointcount_pixelresidual_filter = scene_.structure.size();
  openMVG::sfm::RemoveOutliers_AngleError(scene_, 2.0);
  const size_t pointcount_angular_filter = scene_.structure.size();
  std::cout << "Outlier removal (remaining #points):\n"
            << "\t initial structure size #3DPoints: " << pointcount_initial << "\n"
            << "\t\t pixel residual filter  #3DPoints: " << pointcount_pixelresidual_filter << "\n"
            << "\t\t angular filter         #3DPoints: " << pointcount_angular_filter << std::endl;
  // ;
  // const openMVG::IndexT minPointPerPose = 12; // 6 min
  // const openMVG::IndexT minTrackLength = 3;   // 2 min
  // if (openMVG::sfm::eraseUnstablePosesAndObservations(scene_, minPointPerPose,
  //                                                     minTrackLength)) {
  //   // TODO: must ensure that track graph is producing a single connected
  //   // component

  //   const size_t pointcount_cleaning = scene_.structure.size();
  //   std::cout << "Point_cloud cleaning:\n"
  //             << "\t #3DPoints: " << pointcount_cleaning << "\n";
  // } else {
  //   std::cout << "JOPA PIZDEC" << std::endl;
  // }
}

void Pipeline::update(const Eigen::Isometry3d& prevPose, const Eigen::Isometry3d& currentPose,
                      const std::vector<cv::Point2f> prevFeatures, const std::vector<cv::Point2f> currentFeatures,
                      const std::vector<int> indexes, int poseId)
{
  scene_.views[poseId].reset(
      new openMVG::sfm::View("", poseId, 0, poseId, scene_.intrinsics[0]->w(), scene_.intrinsics[0]->h()));
  // scene_.poses[poseId] = openMVG::geometry::Pose3(currentPose.linear(), currentPose.translation());;

  scene_.poses[poseId] = toPose3(currentPose);

  const auto points3d = triangulate(toPose3(prevPose), toPose3(currentPose), prevFeatures, currentFeatures, indexes,
                                    scene_.intrinsics[0]);
  std::cout << "features " << prevFeatures.size() << " points3d size " << points3d.size() << std::endl;
  for (const auto& [featureId, point3d] : points3d) {
    const auto currFeature = openMVG::Vec2(currentFeatures[featureId].x, currentFeatures[featureId].y).cast<double>();
    if (scene_.structure.find(featureId) == scene_.structure.end()) {
      openMVG::sfm::Landmark landmark;
      landmark.obs[poseId] = openMVG::sfm::Observation(currFeature, featureId);
      landmark.X = point3d;
      scene_.structure.insert(std::make_pair(featureId, landmark));
    } else {
      scene_.structure[featureId].obs[poseId] = openMVG::sfm::Observation(currFeature, featureId);
    }
  }

  // filter();
}

Eigen::Isometry3d Pipeline::pose(int id)
{
  Eigen::Isometry3d p = Eigen::Isometry3d::Identity();
  p.translation() = scene_.poses[id].center();
  p.linear() = scene_.poses[id].rotation();
  return p;
}

void Pipeline::adjust()
{
  openMVG::sfm::Bundle_Adjustment_Ceres ba;
  ba.ceres_options().bVerbose_ = false;
  ba.ceres_options().bCeres_summary_ = false;
  ba.ceres_options().nb_threads_ = 4;
  ba.ceres_options().max_num_iterations_ = 1000;
  ba.Adjust(scene_, openMVG::sfm::Optimize_Options(openMVG::cameras::Intrinsic_Parameter_Type::ADJUST_ALL,
                                                   openMVG::sfm::Extrinsic_Parameter_Type::ADJUST_ALL,
                                                   openMVG::sfm::Structure_Parameter_Type::ADJUST_ALL));
}

void Pipeline::save(fs::path sceneBeforePath)
{
  openMVG::sfm::Save(scene_, sceneBeforePath.string(),
                     openMVG::sfm::ESfM_Data::ALL);  // openMVG::sfm::ESfM_Data::EXTRINSICS
}

std::unordered_map<int, openMVG::Vec3>
Pipeline::triangulate(const openMVG::geometry::Pose3& prevPoseVec3, const openMVG::geometry::Pose3& currentPoseVec3,
                      const std::vector<cv::Point2f>& prevFeatures, const std::vector<cv::Point2f>& currentFeatures,
                      const std::vector<int> indexes, std::shared_ptr<openMVG::cameras::IntrinsicBase> intrinsics)
{
  std::unordered_map<int, openMVG::Vec3> landmarks;
  for (int id = 0; id < prevFeatures.size(); ++id) {
    const auto prevFeature = openMVG::Vec2(prevFeatures[id].x, prevFeatures[id].y).cast<double>();
    const auto currFeature = openMVG::Vec2(currentFeatures[id].x, currentFeatures[id].y).cast<double>();

    const auto bearingPrev = (*intrinsics)(prevFeature);
    const auto bearingCurr = (*intrinsics)(currFeature);
    // Point triangulation
    openMVG::Vec3 point3d;
    bool success = openMVG::Triangulate2View(prevPoseVec3.rotation(), prevPoseVec3.translation(), bearingPrev,
                                             currentPoseVec3.rotation(), currentPoseVec3.translation(), bearingCurr,
                                             point3d, openMVG::ETriangulationMethod::DEFAULT);
    if (success) {
      landmarks.insert({indexes[id], point3d});
    }
  }
  return landmarks;
}

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

TrackerOpenCV::TrackerOpenCV(CameraIntrinsic intrinsic, int featuresNumber)
  : intrinsic_(std::move(intrinsic)), pp_(cv::Point2d(intrinsic.cx, intrinsic.cy)), featuresNumber_(featuresNumber)
{
  detector_ = cv::GFTTDetector::create(featuresNumber, 0.01, 0.0);
}

void TrackerOpenCV::detect()
{
  cv::Mat mask(prevImg.size(), CV_8UC1, cv::Scalar(255));
  for (const auto& p : prevFeatures_) {
    cv::circle(mask, p, 20, 0. - 1);
  }
  int count = featuresNumber_ - prevFeatures_.size();
  if (count == 0) {
    return;
  }
  detector_ = cv::GFTTDetector::create(count, 0.01, 0.0);

  std::vector<cv::KeyPoint> keypoints;
  detector_->detect(prevImg, keypoints, mask);

  std::vector<cv::Point2f> newPoints;
  cv::KeyPoint::convert(keypoints, newPoints, std::vector<int>());

  for (const auto& p : newPoints) {
    prevFeatures_.push_back(p);
    ++index_;
    indexes_.push_back(index_);
  }
}

void TrackerOpenCV::track(const cv::Mat& frame)
{
  try {
    if (prevImg.empty()) {
      prevImg = frame.clone();
    }
    prevFeatures_ = currFeatures_;

    if (currFeatures_.size() < featuresNumber_ * 0.8) {
      detect();
    }

    cv::calcOpticalFlowPyrLK(prevImg, frame, prevFeatures_, currFeatures_, status, error, cv::Size(21, 21), 3);

    spatialFilter(frame, prevFeatures_, currFeatures_, status);
    getRidFailedFeatures(prevFeatures_, currFeatures_, status);

    prevImg = frame.clone();
  } catch (const std::runtime_error& ex) {
    std::cout << ex.what() << std::endl;
  }
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

pcl::PointCloud<pcl::PointXYZ>::Ptr triangulateOpenCV(Eigen::Isometry3d rel, CameraIntrinsic intrinsic,
                                                      const std::vector<cv::Point2f>& prevFeatures,
                                                      const std::vector<cv::Point2f>& currentFeatures)
{
  cv::Mat K =
      (cv::Mat_<float>(3, 3) << intrinsic.fx, 0.f, intrinsic.cx, 0.f, intrinsic.fy, intrinsic.cy, 0.f, 0.f, 1.f);
  cv::Mat Kd;
  K.convertTo(Kd, CV_64F);

  auto pp = cv::Point2d(intrinsic.cx, intrinsic.cy);

  cv::Mat E = cv::findEssentialMat(prevFeatures, currentFeatures, intrinsic.fx, pp, cv::RANSAC, 0.999, 1.0);

  cv::Mat R, t;
  cv::recoverPose(E, prevFeatures, currentFeatures, R, t, intrinsic.fx, pp);

  R.convertTo(R, CV_32F);
  t.convertTo(t, CV_32F);

  cv::Mat Rt0 = cv::Mat::eye(3, 4, CV_64FC1);
  cv::Mat Rt1 = cv::Mat::eye(3, 4, CV_64FC1);

  R.copyTo(Rt1.rowRange(0, 3).colRange(0, 3));
  t.copyTo(Rt1.rowRange(0, 3).col(3));

  // cv::Mat Rt1Iso = cv::Mat::eye(3, 4, CV_64FC1);
  // cv::Mat R1Iso = cv::Mat::eye(3, 3, CV_64FC1);
  // cv::Mat T1Iso = cv::Mat::zeros(3, 1, CV_64FC1);
  // for (int i = 0; i < 3; ++i) {
  //   for (int j = 0; j < 3; ++j) {
  //     R1Iso.at<float>(i, j) = static_cast<float>(rel(i, j));
  //   }
  // }
  // cv::eigen2cv(rel.linear().matrix(), R1Iso);
  // T1Iso.at<float>(0, 0) = static_cast<float>(rel.translation().x());
  // T1Iso.at<float>(1, 0) = static_cast<float>(rel.translation().y());
  // T1Iso.at<float>(2, 0) = static_cast<float>(rel.translation().z());

  // R1Iso.convertTo(R1Iso, CV_32F);
  // T1Iso.convertTo(T1Iso, CV_32F);

  // R1Iso.copyTo(Rt1Iso.rowRange(0, 3).colRange(0, 3));
  // T1Iso.copyTo(Rt1Iso.rowRange(0, 3).col(3));

  // std::cout << "T compare" << std::endl;

  // std::cout << rel.translation().x() << " vs " << t.at<float>(0) << std::endl;
  // std::cout << rel.translation().y() << " vs " << t.at<float>(1) << std::endl;
  // std::cout << rel.translation().z() << " vs " << t.at<float>(2) << std::endl;

  // std::cout << "R compare" << std::endl;

  // for (int i = 0; i < R.rows; ++i) {
  //   for (int j = 0; j < R.cols; ++j) {
  //     std::cout << rel(i, j) << " vs " << R.at<float>(i, j) << std::endl;
  //   }
  // }

  // std::cout << "rt compare" << std::endl;

  // for (int i = 0; i < Rt1Iso.rows; ++i) {
  //   for (int j = 0; j < Rt1Iso.cols; ++j) {
  //     std::cout << Rt1Iso.at<float>(i, j) << " vs " << Rt1.at<float>(i, j) << std::endl;
  //   }
  // }

  std::vector<cv::Point2d> points1, points2;
  for (int i = 0; i < prevFeatures.size(); ++i) {
    points1.push_back(cv::Point2d((double)prevFeatures[i].x, (double)prevFeatures[i].y));
    points2.push_back(cv::Point2d((double)currentFeatures[i].x, (double)currentFeatures[i].y));
  }

  cv::Mat point3d_homo;
  cv::triangulatePoints(Kd * Rt0, Kd * Rt1, points1, points2, point3d_homo);

  assert(point3d_homo.cols == points1.size());
  std::cout << point3d_homo.cols << std::endl;

  std::vector<Eigen::Vector3d> points3d;
  for (int i = 0; i < point3d_homo.cols; i++) {
    pcl::PointXYZ point;
    cv::Mat p3d;
    cv::Mat _p3h = point3d_homo.col(i);
    cv::convertPointsFromHomogeneous(_p3h.t(), p3d);
    points3d.push_back(Eigen::Vector3d(p3d.at<double>(0), p3d.at<double>(1), p3d.at<double>(2)));
  }

  pcl::PointCloud<pcl::PointXYZ>::Ptr localCloud(new pcl::PointCloud<pcl::PointXYZ>);
  for (int i = 0; i < points3d.size(); i++) {
    localCloud->points.push_back(pcl::PointXYZ(points3d[i].x(), points3d[i].y(), points3d[i].z()));
  }
  return localCloud;
}

void SFM::run()
{
  for (int i = config_.startFrame + 1; i < config_.endFrame; ++i) {
    const auto frame = cv::imread(config_.paths[i].string());

    tracker_->track(frame);

    auto relativePoseCam = relativeProvider_->handle(tracker_->prevFeatures(), tracker_->currFeatures());
    // inverse of camera moving transform
    Eigen::Isometry3d relativePoseCamInverse = Eigen::Isometry3d::Identity();
    relativePoseCamInverse.linear() = relativePoseCam.linear().transpose();
    relativePoseCamInverse.translation() = -relativePoseCam.linear().transpose() * relativePoseCam.translation();

    double scale = getOdomScale(config_.gt[i].translation(), config_.gt[i - 1].translation());

    if (scale > config_.scaleThreshold &&
        relativePoseCamInverse.translation().z() > relativePoseCamInverse.translation().x() &&
        relativePoseCamInverse.translation().z() > relativePoseCamInverse.translation().y()) {
      auto localCloud = triangulateOpenCV(relativePoseCam, config_.calibration.intrinsic, tracker_->prevFeatures(),
                                          tracker_->currFeatures());

      relativePoseCamInverse.translation() *= scale;
      odom_->update(relativePoseCamInverse);

      const auto currentPose = odom_->pose();

      // pipeline_->update(config_.gt[i - 1], config_.gt[i], tracker_->prevFeatures(), tracker_->currFeatures(),
      // tracker_->indexes(),
      //                   i);
      // pipeline_->adjust();

      if (config_.verbose) {
        debug_->visualize(i, frame, tracker_->prevFeatures(), tracker_->currFeatures(), scale, relativePoseCamInverse,
                          currentPose, pipeline_->pose(i), config_.gt[i], config_.gt[i - 1]);
        // debug_->publish(pipeline_->scene());
        debug_->publish(currentPose, "pose");
        debug_->publish(config_.gt[i], "gt");
        pcl::PointCloud<pcl::PointXYZ>::Ptr localCloudMap(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::transformPointCloud(*localCloud, *localCloudMap, currentPose.matrix());

        debug_->publish(localCloudMap);
        progress_bar(i, config_.endFrame);
      }

      prevOdom_ = currentPose;
    }
  }
}
