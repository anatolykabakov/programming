#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "utils/math_utils.h"

namespace adas {

/** Topic names shared by Android bag / sim pipeline / visualizer. */
namespace topics {
inline constexpr const char* kVehicleChassis = "vehicle/chassis";
/** Android / bag ZMQ protobuf topic. */
inline constexpr const char* kVisionLanes = "vision/lanes";
/** Typed ego polyline for LaneKeep (converted from vision/lanes). */
inline constexpr const char* kVisionPath = "vision/path";
inline constexpr const char* kGpsLocation = "sensors/gps/location";
/** Android ZMQ IMU topic. */
inline constexpr const char* kImu = "sensors/imu";
/** Typed raw phone-frame IMU (from TopicConvert). */
inline constexpr const char* kImuRaw = "sensors/imu_raw";
/** Calibrated vehicle yaw-rate for Localization (from ImuCalibService). */
inline constexpr const char* kImuYaw = "sensors/imu_yaw";
inline constexpr const char* kLaneKeep = "control/lane_keep";
inline constexpr const char* kLocalizationPose = "localization/pose";
inline constexpr const char* kSteerCommand = "controls/steer";
inline constexpr const char* kCameraCalib = "calibration/camera";
inline constexpr const char* kCalibLaneUv = "calibration/lane_uv";
/** Model pose / cameraOdometry from Android or bag. */
inline constexpr const char* kCameraOdometry = "model/camera_odometry";
inline constexpr const char* kVehicleState = "vehicle/state";
}  // namespace topics

/** Lightweight chassis sample for algorithm services (sim + topic bus). */
struct ChassisSample {
  int64_t timestamp_us = 0;
  double speed_mps = 0.0;
  double steer_rad = 0.0;  // road-wheel angle
  double yaw_rate = 0.0;   // rad/s
};

/** Ego-frame path for lane keep (X forward, Y left). */
struct LanePathMsg {
  int64_t timestamp_us = 0;
  int frame_id = 0;
  std::vector<Vec2> polyline;  // (x, y)
};

struct GpsSample {
  int64_t timestamp_us = 0;
  double x = 0.0;  // local ENU / bag frame
  double y = 0.0;
  bool valid = false;
};

/** Phone-frame IMU (accel m/s², gyro rad/s). TopicConvert → ImuCalibService. */
struct RawImuSample {
  int64_t timestamp_us = 0;
  double ax = 0, ay = 0, az = 0;
  double gx = 0, gy = 0, gz = 0;
  bool valid = false;
};

struct ImuSample {
  int64_t timestamp_us = 0;
  double yaw_rate = 0.0;  // rad/s (vehicle frame, after ImuCalibService)
  bool valid = false;
};

struct LocalizationPose {
  int64_t timestamp_us = 0;
  double x = 0.0;
  double y = 0.0;
  double yaw = 0.0;
  double v = 0.0;
  double yaw_rate = 0.0;
  double odom_x = 0.0;
  double odom_y = 0.0;
  double ekf_x = 0.0;
  double ekf_y = 0.0;
};

/** Image-space left/right lane samples for VP calib (u,v). */
struct LaneUvMsg {
  int64_t timestamp_us = 0;
  std::vector<Vec2> left_uv;
  std::vector<Vec2> right_uv;
};

/** Model pose / cameraOdometry (openpilot units after parse). */
struct CameraOdometrySample {
  int64_t timestamp_us = 0;
  double trans[3] = {0, 0, 0};
  double rot[3] = {0, 0, 0};
  double trans_std[3] = {1, 1, 1};
  double rot_std[3] = {1, 1, 1};
  bool valid = false;
};

struct CameraCalibrationState {
  int64_t timestamp_us = 0;
  double roll_deg = 0.0;
  double pitch_deg = -6.0;
  double yaw_deg = 0.0;
  double camera_height_m = 1.40;
  double fx = 930.0;
  double fy = 930.0;
  double cx = 640.0;
  double cy = 360.0;
  bool calibration_success = false;
  int n_updates = 0;
  double vp_u = 0.0;
  double vp_v = 0.0;
  bool has_vp = false;
  int cal_percent = 0;
  int cal_status = 0;  // PoseCalibrator::Status
};

}  // namespace adas
