#pragma once

#include "framework/service_manager.hpp"
#include "messages.pb.h"
#include "utils/adas_config.h"

namespace adas {

/**
 * Android/ZMQ protobuf → typed algorithm inputs:
 *   vision/lanes  → vision/path
 *   vehicle/state → vehicle/chassis
 *   sensors/imu   → sensors/imu_raw
 *   sensors/gps/location → sensors/gps/location (GpsSample ENU)
 *   calibration/lane_uv (ZMQ) → calibration/lane_uv (LaneUvMsg)
 *   model/camera_odometry → typed CameraOdometrySample
 */
class TopicConvertService : public microros::Service {
public:
  explicit TopicConvertService(double steer_ratio = 15.7);

  void configure() override;
  void reset() override;

private:
  void onVisionLanes(const ai::flow::adas::ZMQMessage& msg);
  void onVehicleState(const ai::flow::adas::ZMQMessage& msg);
  void onImu(const ai::flow::adas::ZMQMessage& msg);
  void onGps(const ai::flow::adas::ZMQMessage& msg);
  void onLaneUv(const ai::flow::adas::ZMQMessage& msg);
  void onCameraOdometry(const ai::flow::adas::ZMQMessage& msg);

  double steer_ratio_ = 15.7;
  GpsLocalProjector gps_proj_;
};

}  // namespace adas
