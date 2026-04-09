#include "mapping.h"
#include "openMVG/sfm/sfm_data_triangulation.hpp"
#include "openMVG/multiview/triangulation_nview.hpp"
#include "openMVG/multiview/triangulation.hpp"

// #include "utils.h"

Mapping::Mapping(const CameraIntrinsic& intrinsic)
{
  scene_.intrinsics[0] = std::make_shared<openMVG::cameras::Pinhole_Intrinsic>(
      intrinsic.width, intrinsic.height, intrinsic.fx, intrinsic.cx, intrinsic.cy);
}

static double RMSE(const openMVG::sfm::SfM_Data& sfm_data)
{
  // Compute residuals for each observation
  std::vector<double> vec;
  for (openMVG::sfm::Landmarks::const_iterator iterTracks = sfm_data.GetLandmarks().begin();
       iterTracks != sfm_data.GetLandmarks().end(); ++iterTracks) {
    const openMVG::sfm::Observations& obs = iterTracks->second.obs;
    for (openMVG::sfm::Observations::const_iterator itObs = obs.begin(); itObs != obs.end(); ++itObs) {
      const openMVG::sfm::View* view = sfm_data.GetViews().find(itObs->first)->second.get();
      const openMVG::geometry::Pose3 pose = sfm_data.GetPoseOrDie(view);
      const std::shared_ptr<openMVG::cameras::IntrinsicBase> intrinsic =
          sfm_data.GetIntrinsics().at(view->id_intrinsic);
      const openMVG::Vec2 residual = intrinsic->residual(pose(iterTracks->second.X), itObs->second.x);
      // std::cout << residual << " ";
      vec.push_back(std::abs(residual(0)));
      vec.push_back(std::abs(residual(1)));
    }
  }
  const Eigen::Map<Eigen::RowVectorXd> residuals(&vec[0], vec.size());
  const double RMSE = std::accumulate(vec.begin(), vec.end(), 0);  // std::sqrt(residuals.squaredNorm() / vec.size());
  std::cout << "RMSE " << RMSE << " " << vec.size() << " " << sfm_data.GetLandmarks().size() << std::endl;
  // if (RMSE != RMSE) {
  //   std::abort();
  // }
  return RMSE;
}

/// Triangulate a given set of observations
bool track_triangulation
(
  const openMVG::sfm::SfM_Data & sfm_data,
  const openMVG::sfm::Observations & obs,
  openMVG::Vec3 & X,
  const openMVG::ETriangulationMethod & etri_method = openMVG::ETriangulationMethod::DEFAULT
)
{
  if (obs.size() >= 2)
  {
    std::vector<openMVG::Vec3> bearing;
    std::vector<openMVG::Mat34> poses;
    std::vector<openMVG::sfm::Pose3> poses_;
    bearing.reserve(obs.size());
    poses.reserve(obs.size());
    for (const auto& observation : obs)
    {
      const openMVG::sfm::View * view = sfm_data.views.at(observation.first).get();
      if (!sfm_data.IsPoseAndIntrinsicDefined(view))
        return false;
      const openMVG::cameras::IntrinsicBase * cam = sfm_data.GetIntrinsics().at(view->id_intrinsic).get();
      const openMVG::sfm::Pose3 pose = sfm_data.GetPoseOrDie(view);
      bearing.emplace_back((*cam)(cam->get_ud_pixel(observation.second.x)));
      poses.emplace_back(pose.asMatrix());
      poses_.emplace_back(pose);
    }
    if (bearing.size() > 2)
    {
      const Eigen::Map<const openMVG::Mat3X> bearing_matrix(bearing[0].data(), 3, bearing.size());
      openMVG::Vec4 Xhomogeneous;
      if (openMVG::TriangulateNViewAlgebraic
      (
        bearing_matrix,
        poses,
        &Xhomogeneous))
      {
        X = Xhomogeneous.hnormalized();
        return true;
      }
    }
    else
    {
      return openMVG::Triangulate2View
      (
        poses_.front().rotation(),
        poses_.front().translation(),
        bearing.front(),
        poses_.back().rotation(),
        poses_.back().translation(),
        bearing.back(),
        X,
        etri_method
      );
    }
  }
  return false;
}


void Mapping::filter()
{
  // const size_t pointcount_initial = scene_.structure.size();
  // openMVG::sfm::RemoveOutliers_PixelResidualError(scene_, 4.0);
  // const size_t pointcount_pixelresidual_filter = scene_.structure.size();
  // openMVG::sfm::RemoveOutliers_AngleError(scene_, 2.0);
  // const size_t pointcount_angular_filter = scene_.structure.size();
  // std::cout << "Outlier removal (remaining #points):\n"
  //           << "\t initial structure size #3DPoints: " << pointcount_initial << "\n"
  //           << "\t\t pixel residual filter  #3DPoints: " << pointcount_pixelresidual_filter << "\n"
  //           << "\t\t angular filter         #3DPoints: " << pointcount_angular_filter << std::endl;
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

void Mapping::update(const Eigen::Isometry3d& relative, const Eigen::Isometry3d& currentPose,
                     const std::vector<cv::Point2f> prevFeatures, const std::vector<cv::Point2f> currentFeatures,
                     const std::vector<int> indexes, int poseId)
{
  scene_.views[poseId] = std::make_shared<openMVG::sfm::View>("", poseId, 0, poseId, scene_.intrinsics[0]->w(), scene_.intrinsics[0]->h());

  scene_.poses[poseId] =
      openMVG::geometry::Pose3(currentPose.linear(), -currentPose.linear().inverse() * currentPose.translation());

  const auto points3d = triangulate(relative.inverse(), prevFeatures, currentFeatures, indexes, scene_.intrinsics[0]);
  std::cout << "features " << prevFeatures.size() << " points3d size " << points3d.size() << std::endl;
  for (const auto& [featureId, point3d] : points3d) {
    const auto currFeature = openMVG::Vec2(currentFeatures[featureId].x, currentFeatures[featureId].y).cast<double>();
    if (scene_.structure.find(featureId) == scene_.structure.end()) {
      openMVG::sfm::Landmark landmark;
      landmark.obs[poseId] = openMVG::sfm::Observation(currFeature, featureId);
      // landmark.X = openMVG::Vec3::Zero();
      landmark.X = scene_.poses[poseId](point3d);
      // auto res = scene_.intrinsics[0]->residual(scene_.poses[poseId](landmark.X), landmark.obs[poseId].x, true);
      // std::cout << res.matrix() << std::endl;
      scene_.structure.insert(std::make_pair(featureId, landmark));
    } else {
      scene_.structure[featureId].obs[poseId] = openMVG::sfm::Observation(currFeature, featureId);
    }
    // track_triangulation(scene_, scene_.structure[featureId].obs, scene_.structure[featureId].X);
  }
  // if (poseId > 10) {
  //   openMVG::sfm::SfM_Data_Structure_Computation_Blind tr;
  //   tr.triangulate(scene_);
  //   std::cout << std::endl;
  // }
  // filter();
}

Eigen::Isometry3d Mapping::pose(int id) { return Eigen::Isometry3d{scene_.poses[id].asMatrix()}; }

void Mapping::adjust()
{
  openMVG::sfm::Bundle_Adjustment_Ceres ba;
  ba.ceres_options().bVerbose_ = false;
  ba.ceres_options().bCeres_summary_ = false;
  ba.ceres_options().nb_threads_ = 4;
  ba.ceres_options().max_num_iterations_ = 1000;
  bool success =
      ba.Adjust(scene_, openMVG::sfm::Optimize_Options(openMVG::cameras::Intrinsic_Parameter_Type::ADJUST_ALL,
                                                       openMVG::sfm::Extrinsic_Parameter_Type::ADJUST_ALL,
                                                       openMVG::sfm::Structure_Parameter_Type::ADJUST_ALL));
  std::cout << "adjust " << success << std::endl;
  RMSE(scene_);
}

void Mapping::save(fs::path sceneBeforePath)
{
  openMVG::sfm::Save(scene_, sceneBeforePath.string(),
                     openMVG::sfm::ESfM_Data::ALL);  // openMVG::sfm::ESfM_Data::EXTRINSICS
}

std::unordered_map<int, openMVG::Vec3> Mapping::triangulate(const Eigen::Isometry3d& relative,
                                                            const std::vector<cv::Point2f>& prevFeatures,
                                                            const std::vector<cv::Point2f>& currentFeatures,
                                                            const std::vector<int> indexes,
                                                            std::shared_ptr<openMVG::cameras::IntrinsicBase> intrinsics)
{
  // auto pInv = prev.inverse();
  // auto cInv = curr.inverse();
  std::unordered_map<int, openMVG::Vec3> landmarks;
  for (int id = 0; id < prevFeatures.size(); ++id) {
    const auto prevFeature = openMVG::Vec2(prevFeatures[id].x, prevFeatures[id].y).cast<double>();
    const auto currFeature = openMVG::Vec2(currentFeatures[id].x, currentFeatures[id].y).cast<double>();

    // std::cout << curr.translation().transpose().matrix() << std::endl;
    // std::cout << cInv.translation().transpose().matrix() << std::endl;
    const auto bearingPrev = (*intrinsics)(prevFeature);
    const auto bearingCurr = (*intrinsics)(currFeature);
    // Point triangulation
    openMVG::Vec3 point3d;
    bool success = openMVG::Triangulate2View(openMVG::Mat3::Identity(), openMVG::Vec3::Zero(), bearingPrev,
                                             relative.linear(), relative.translation(), bearingCurr, point3d,
                                             openMVG::ETriangulationMethod::DIRECT_LINEAR_TRANSFORM);
    if (success) {
      landmarks.insert({indexes[id], point3d});
    }
  }
  return landmarks;
}
