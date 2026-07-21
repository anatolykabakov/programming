#pragma once

#include "framework/service_manager.hpp"
#include "utils/adas_topics.h"
#include "utils/pose_calibrator.h"
#include "utils/vanishing_point_calib.h"

namespace adas {

/**
 * Camera extrinsic calib.
 * Primary (Android/sim/bag): model pose → PoseCalibrator (flowpilot calibrationd).
 * Optional host VP: calibration/lane_uv → VanishingPointCalibrator.
 * Publishes calibration/camera.
 */
class CameraCalibService : public microros::Service {
public:
  CameraCalibService(double pitch0_deg = -6.0, double yaw0_deg = 0.0, double height_m = 1.40, double fx = 930.0,
                     double fy = 930.0, double cx = 640.0, double cy = 360.0, int history_len = 50);

  void configure() override;
  void reset() override;

  void setIntrinsics(double fx, double fy, double cx, double cy);
  void setHeight(double height_m);
  void setEstimate(double pitch_deg, double yaw_deg);
  void setVEgo(double v_ego_mps);

  /** Standalone pose step (Python / tests). */
  bool updateFromPose(const CameraOdometrySample& odom, double v_ego_mps = -1.0);

  /** Host VP path. */
  bool updateFromUv(const std::vector<Vec2>& left_uv, const std::vector<Vec2>& right_uv, int64_t timestamp_us = 0);

  const CameraCalibrationState& last() const { return last_; }
  PoseCalibrator& poseCalibrator() { return pose_calib_; }
  VanishingPointCalibrator& calibrator() { return vp_calib_; }
  int historyPending() const { return vp_calib_.historySize(); }
  int calPercent() const { return pose_calib_.calPercent(); }

private:
  void onLaneUv(const LaneUvMsg& msg);
  void onCameraOdometry(const CameraOdometrySample& msg);
  void onChassis(const ChassisSample& msg);
  void syncLastFromPose(int64_t timestamp_us);
  void publishState(int64_t timestamp_us);

  PoseCalibrator pose_calib_;
  VanishingPointCalibrator vp_calib_;
  double height_m_ = 1.40;
  double fx_ = 930.0, fy_ = 930.0, cx_ = 640.0, cy_ = 360.0;
  CameraCalibrationState last_;
};

}  // namespace adas
