#include "sfm.h"

void SFM::run()
{
  pcl::PointCloud<pcl::PointXYZ>::Ptr globalCloudMap(new pcl::PointCloud<pcl::PointXYZ>);
  std::unordered_map<int, pcl::PointXYZ> hm;
  for (int i = config_.startFrame + 1; i < config_.endFrame; ++i) {
    const auto frame = cv::imread(config_.paths[i].string());

    tracker_->track(frame);

    auto relative = relativeProvider_->handle(tracker_->prevFeatures(), tracker_->currFeatures());

    tracker_->filter(relative.essMask);

    double scale = 1.;// getOdomScale(config_.gt[i].translation(), config_.gt[i - 1].translation());

    if (scale > config_.scaleThreshold && relative.pose.translation().z() > relative.pose.translation().x() &&
        relative.pose.translation().z() > relative.pose.translation().y()) {
      relative.pose.translation() *= scale;
      odom_->update(relative.pose);

      const auto currentPose = odom_->pose();

      if (config_.verbose && i % 10) {
        pipeline_->update(relative.pose, currentPose, tracker_->prevFeatures(), tracker_->currFeatures(),
                          tracker_->indexes(), i);
        pipeline_->adjust();

        // const auto points3d = triangulateOpenCV(relative, config_.calibration.intrinsic, tracker_->prevFeatures(),
        //                                         tracker_->currFeatures());
        // auto ind = tracker_->indexes();
        // for (int i = 0; i < points3d.size(); ++i) {
        //   const auto pt_xyz = points3d[i];
        //   if (pt_xyz.z() <= 0.0 || pt_xyz.z() >= 50.0) {
        //     continue;
        //   }
        //   auto pt_cam = pcl::PointXYZ(pt_xyz.x(), pt_xyz.y(), pt_xyz.z());
        //   const auto pt_pcl_map = pcl::transformPoint<pcl::PointXYZ, double>(pt_cam, currentPose);
        //   if (hm.find(ind[i]) == hm.end()) {
        //     hm.insert(std::make_pair(ind[i], pt_pcl_map));
        //     globalCloudMap->points.push_back(pt_pcl_map);
        //   } else {
        //     hm[ind[i]] = pt_pcl_map;
        //   }
        // }

        debug_->visualize(i, frame, tracker_->prevFeatures(), tracker_->currFeatures(), scale, relative.pose,
                          currentPose, pipeline_->pose(i), config_.gt[i], config_.gt[i - 1]);
        // debug_->publish(globalCloudMap);
        debug_->publish(pipeline_->scene());
        debug_->publish(currentPose, "pose");
        debug_->publish(config_.gt[i], "gt");

        progress_bar(i, config_.endFrame);
      }

      prevOdom_ = currentPose;
    }
  }
}
