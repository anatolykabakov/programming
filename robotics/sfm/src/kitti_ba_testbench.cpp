#include "visual_odom_mono.h"

#include <tclap/CmdLine.h>

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

#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/core/core_c.h"

bool readIntrinsic(const std::string& fileName, openMVG::Mat3& K)
{
  // Load the K matrix
  std::ifstream in;
  in.open(fileName.c_str(), std::ifstream::in);
  if (in) {
    for (int j = 0; j < 3; ++j)
      for (int i = 0; i < 3; ++i)
        in >> K(j, i);
  } else {
    std::cerr << std::endl << "Invalid input K.txt file" << std::endl;
    return false;
  }
  return true;
}

openMVG::geometry::Pose3 toPose3(const Eigen::Isometry3d& pose)
{
  Eigen::Vector3d center = -pose.linear().transpose() * pose.translation();
  openMVG::geometry::Pose3 poseVec3(pose.linear(), center);
  return poseVec3;
}

openMVG::sfm::SfM_Data buildOpenMVGScene(const Eigen::Isometry3d& prevPose, const Eigen::Isometry3d& currentPose,
                                         const std::vector<cv::Point2f> prevFeatures,
                                         const std::vector<cv::Point2f> currentFeatures,
                                         std::shared_ptr<openMVG::cameras::Pinhole_Intrinsic> intrinsics, int prevId,
                                         int currentId)
{
  openMVG::geometry::Pose3 prevPoseVec3 = toPose3(prevPose);
  openMVG::geometry::Pose3 currentPoseVec3 = toPose3(currentPose);

  openMVG::sfm::SfM_Data scene;
  scene.views[prevId].reset(new openMVG::sfm::View("", prevId, 0, prevId, intrinsics->w(), intrinsics->h()));
  scene.views[currentId].reset(new openMVG::sfm::View("", currentId, 0, currentId, intrinsics->w(), intrinsics->h()));
  scene.intrinsics[0] = intrinsics;
  scene.poses[scene.views[prevId]->id_pose] = prevPoseVec3;
  scene.poses[scene.views[currentId]->id_pose] = currentPoseVec3;

  for (int featureId = 0; featureId < prevFeatures.size(); ++featureId) {
    const auto prevFeature = openMVG::Vec2(prevFeatures[featureId].x, prevFeatures[featureId].y).cast<double>();
    const auto currFeature = openMVG::Vec2(currentFeatures[featureId].x, currentFeatures[featureId].y).cast<double>();

    // Point triangulation
    openMVG::Vec3 euclideanTriangulatedPoint;
    if (openMVG::Triangulate2View(prevPoseVec3.rotation(), prevPoseVec3.translation(),
                                  (*scene.intrinsics[0])(prevFeature), currentPoseVec3.rotation(),
                                  currentPoseVec3.translation(), (*scene.intrinsics[0])(currFeature),
                                  euclideanTriangulatedPoint, openMVG::ETriangulationMethod::DEFAULT)) {
      // Add a new landmark (3D point with it's 2d observations)
      openMVG::sfm::Landmark landmark;
      landmark.obs[scene.views[prevId]->id_view] = openMVG::sfm::Observation(prevFeature, featureId);
      landmark.obs[scene.views[currentId]->id_view] = openMVG::sfm::Observation(currFeature, featureId);
      landmark.X = euclideanTriangulatedPoint;
      scene.structure.insert(std::make_pair(featureId, landmark));
    }
  }
  return scene;
}

int main(int argc, char** argv)
{
  TCLAP::CmdLine cmd("Command description message", ' ', "0.9");
  TCLAP::UnlabeledValueArg<std::string> datasetPathArg("dataset", "", true, "", "dataset");
  TCLAP::UnlabeledValueArg<std::string> resultDirArg("output", "Path to out dir", true, "00", "output");
  cmd.add(datasetPathArg);
  cmd.add(resultDirArg);
  cmd.parse(argc, argv);

  bool verbose = false;

  fs::path datasetPath = fs::path(datasetPathArg.getValue());
  fs::path calibPath = datasetPath / fs::path("K.txt");
  openMVG::Mat3 K;
  readIntrinsic(calibPath.string(), K);

  CameraIntrinsic intrinsic;
  intrinsic.fx = K(0, 0), intrinsic.fx = K(0, 0), intrinsic.cx = K(0, 2);
  intrinsic.cx = K(1, 2);

  fs::path firstImgPath = datasetPath / fs::path("100_7101.jpg");
  fs::path secondImgPath = datasetPath / fs::path("100_7102.jpg");

  cv::Mat currentImg = cv::imread(firstImgPath.string());
  cv::Mat prevImg = cv::imread(secondImgPath.string());

  cv::cvtColor(currentImg, currentImg, cv::COLOR_BGR2GRAY);
  cv::cvtColor(prevImg, prevImg, cv::COLOR_BGR2GRAY);

  int FEATURES_NUMBER = 1000;

  std::unique_ptr<IRelativePoseProvider> relativeProvider = std::make_unique<RelativePoseProviderOpenCV>(intrinsic);

  auto monoVO = std::make_unique<MonoVO>(intrinsic, std::move(relativeProvider), FEATURES_NUMBER);

  Eigen::Isometry3d prevPose = Eigen::Isometry3d::Identity();
  Eigen::Isometry3d currentPose = Eigen::Isometry3d::Identity();
  Eigen::Isometry3d relativePose = Eigen::Isometry3d::Identity();

  relativePose = monoVO->handle(prevImg);

  relativePose = monoVO->handle(currentImg);

  currentPose = currentPose * relativePose;

  std::cout << "prevPose t " << prevPose.translation().transpose().matrix() << " R "
            << prevPose.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
  std::cout << "globalPose t " << currentPose.translation().transpose().matrix() << " R "
            << currentPose.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;

  const auto prevFeatures = monoVO->prevFeatures();
  const auto currFeatures = monoVO->currFeatures();

  const auto status = monoVO->trackingStatus();

  const auto white = cv::Scalar(255, 255, 255);
  const auto black = cv::Scalar(0, 0, 0);
  const auto red = cv::Scalar(0, 0, 255);
  const auto blue = cv::Scalar(255, 0, 0);
  const auto green = cv::Scalar(0, 255, 0);
  for (const auto& point : prevFeatures) {
    cv::circle(prevImg, cv::Point(point.x, point.y), 1, black, 2);
  }

  for (const auto& point : currFeatures) {
    cv::circle(currentImg, cv::Point(point.x, point.y), 1, green, 2);
  }

  cv::imshow("prev", prevImg);
  cv::imshow("curr", currentImg);
  cv::waitKey(0);

  auto intrinsics = std::make_shared<openMVG::cameras::Pinhole_Intrinsic>(prevImg.size().width, prevImg.size().height,
                                                                          intrinsic.fx, intrinsic.cx, intrinsic.cy);

  auto scene = buildOpenMVGScene(prevPose, currentPose, prevFeatures, currFeatures, intrinsics, 0, 1);
  std::cout << "scene before optimization " << std::endl;
  for (const auto& [index, pose] : scene.poses) {
    std::cout << "t " << pose.translation().transpose().matrix() << std::endl;
  }
  fs::path outputDir = fs::path(resultDirArg.getValue());
  fs::path sceneBeforePath = outputDir / fs::path("scene_before.ply");
  fs::path sceneAfterPath = outputDir / fs::path("scene_after.ply");

  openMVG::sfm::Save(scene, sceneBeforePath.string(), openMVG::sfm::ESfM_Data(openMVG::sfm::ALL));

  openMVG::sfm::Bundle_Adjustment_Ceres bundle_adjustment_obj;
  bundle_adjustment_obj.ceres_options().bVerbose_ = false;
  bundle_adjustment_obj.Adjust(scene,
                               openMVG::sfm::Optimize_Options(openMVG::cameras::Intrinsic_Parameter_Type::ADJUST_ALL,
                                                              openMVG::sfm::Extrinsic_Parameter_Type::ADJUST_ALL,
                                                              openMVG::sfm::Structure_Parameter_Type::ADJUST_ALL));

  openMVG::sfm::Save(scene, sceneAfterPath.string(), openMVG::sfm::ESfM_Data(openMVG::sfm::ALL));
  std::cout << "scene after optimization " << std::endl;
  for (const auto& [index, pose] : scene.poses) {
    std::cout << "t " << pose.translation().transpose().matrix() << std::endl;
  }

  std::cout << "structure size " << scene.structure.size() << std::endl;
}
