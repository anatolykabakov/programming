#pragma once

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

namespace fs = std::filesystem;

struct CameraIntrinsic {
  double fx{0.0};
  double fy{0.0};
  double cx{0.0};
  double cy{0.0};
  int width{0};
  int height{0};
};

openMVG::geometry::Pose3 toPose3(const Eigen::Isometry3d& pose);

CameraIntrinsic getKittiCameraIntrinsic(const std::string& path_to_calibfile);

// https://stackoverflow.com/questions/1263072/changing-a-matrix-from-right-handed-to-left-handed-coordinate-system
Eigen::Isometry3d convertRight2LeftCS(const Eigen::Isometry3d& pose);

std::vector<Eigen::Isometry3d> parseGTFile(const std::string& path);

double getOdomScale(const Eigen::Vector3d& prev, const Eigen::Vector3d& curr);

cv::Point convertToImagePoint(const Eigen::Isometry3d& poseRightHandCS, int height, int offset, double scale);

Eigen::Isometry3d opencv2isometry3d(const cv::Mat& T, const cv::Mat& R);

void getRidFailedFeatures(std::vector<cv::Point2f>& points1, std::vector<cv::Point2f>& points2,
                          std::vector<uchar>& status);

class Odometry {
public:
  Odometry(const Eigen::Isometry3d& init);

  void update(const Eigen::Isometry3d& relative);

  Eigen::Isometry3d pose();

private:
  Eigen::Isometry3d globalPose_{Eigen::Isometry3d::Identity()};
};

class IRelativePoseProvider {
public:
  virtual Eigen::Isometry3d handle(const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& current) = 0;
  virtual ~IRelativePoseProvider() = default;
};

class RelativePoseProviderOpenCV : public IRelativePoseProvider {
public:
  RelativePoseProviderOpenCV(const CameraIntrinsic& intinsics);

  Eigen::Isometry3d handle(const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& current) override;

private:
  CameraIntrinsic intrinsics_;
  cv::Point2d pp_;
};

class RelativePoseProviderOpenMVG : public IRelativePoseProvider {
public:
  RelativePoseProviderOpenMVG(const CameraIntrinsic& intinsics);

  Eigen::Isometry3d handle(const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& current) override;

private:
  cv::Point2d pp_;
  std::unique_ptr<openMVG::cameras::Pinhole_Intrinsic> intrinsics_;
};

void progress_bar(int counter, int number, std::string prefix = "");

class Debug {
public:
  Debug();

  void visualize(
    int i,
    double scale,
    const Eigen::Isometry3d &relativePose,
    const Eigen::Isometry3d &globalPose,
    const Eigen::Isometry3d &sfmPose,
    const Eigen::Isometry3d &gt, const Eigen::Isometry3d &gtPrev);

  void saveImg(fs::path trajectoryImgPath);

private:
  cv::Mat trajectoryImg_;
};

class Pipeline {
public:
  Pipeline(const CameraIntrinsic& intrinsic);

  void update(const Eigen::Isometry3d& prevPose,
              const Eigen::Isometry3d& currentPose,
              const std::vector<cv::Point2f> prevFeatures,
              const std::vector<cv::Point2f> currentFeatures,
              int poseId);

  void save(fs::path sceneBeforePath);

  Eigen::Isometry3d pose(int id);

private:
  openMVG::sfm::SfM_Data scene_;

  std::unordered_map<int, openMVG::Vec3> triangulate(const openMVG::geometry::Pose3 &prevPoseVec3,
                  const openMVG::geometry::Pose3 &currentPoseVec3,
                  const std::vector<cv::Point2f> &prevFeatures,
                  const std::vector<cv::Point2f> &currentFeatures,
                  std::shared_ptr<openMVG::cameras::IntrinsicBase> intrinsics);

};

class MonoVO {
public:
  MonoVO(CameraIntrinsic intrinsic, std::unique_ptr<IRelativePoseProvider> relativeProvider, int featuresThreshold);

  Eigen::Isometry3d handle(const cv::Mat& frame);

  std::vector<cv::Point2f> prevFeatures() { return prevFeaturesSaved_; }

  std::vector<cv::Point2f> currFeatures() { return currFeatures_; }

  std::vector<unsigned char> trackingStatus() { return status; }

private:
  std::vector<cv::Point2f> prevFeatures_;
  std::vector<cv::Point2f> prevFeaturesSaved_;
  std::vector<cv::Point2f> currFeatures_;
  std::vector<unsigned char> status;
  std::vector<float> error;
  cv::Mat prevImg;
  std::vector<cv::KeyPoint> keypoints;
  double focal_;
  cv::Point2d pp_;
  CameraIntrinsic intrinsic_;

  std::unique_ptr<IRelativePoseProvider> relativeProvider_;

  cv::Ptr<cv::FeatureDetector> detector_;
  double scaleThreshold = 0.1;
  int featuresNumber_{1000};
};

class SFM {
public:
  struct Config {
    int FEATURES_NUMBER = 1000;
    int startFrame = 0;
    int endFrame = 200;
    double scaleThreshold{0.1};
    bool verbose{false};

    std::vector<std::filesystem::path> paths;
    std::vector<Eigen::Isometry3d> gt;
  };

  SFM(const SFM::Config &config,
      std::unique_ptr<MonoVO> monoVO,
      std::unique_ptr<Odometry> odom,
      std::shared_ptr<Debug> debug,
      std::shared_ptr<Pipeline> pipeline): 
        config_(config),
        monoVO_(std::move(monoVO)),
        odom_(std::move(odom)),
        debug_(debug),
        pipeline_(pipeline) {

  }

  void run();

private:
  SFM::Config config_;
  std::unique_ptr<MonoVO> monoVO_;
  std::unique_ptr<Odometry> odom_;
  std::shared_ptr<Pipeline> pipeline_;
  std::shared_ptr<Debug> debug_;

  Eigen::Isometry3d prevOdom_{Eigen::Isometry3d::Identity()};
};