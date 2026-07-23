#include "services/topic_convert_service.h"

#include "utils/logger.h"
#include "utils/topic_convert.h"

namespace adas {

TopicConvertService::TopicConvertService(Config config) : config_(config), steer_ratio_(config.steer_ratio) {}

void TopicConvertService::configure()
{
  subscribe<ai::flow::adas::ZMQMessage>(topics::kVisionLanes,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onVisionLanes(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kVehicleState,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onVehicleState(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kImu, [this](const ai::flow::adas::ZMQMessage& m) { onImu(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kGpsLocation,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onGps(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kCameraOdometry,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onCameraOdometry(m); });
  LOGI("TopicConvertService: lanes/state/imu/gps/cam_odo → path/chassis/imu_raw/gps(ENU)/cam_odo");
}

void TopicConvertService::reset() { gps_proj_.reset(); }

void TopicConvertService::onVisionLanes(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_lane_lines()) {
    LOGW("vision/lanes without lane_lines payload");
    return;
  }
  auto path = laneLinesToPath(msg.lane_lines());
  LOGI("vision/lanes → path n=%zu frame=%d", path.polyline.size(), path.frame_id);
  publish(topics::kVisionPath, path);
}

void TopicConvertService::onVehicleState(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_car_state())
    return;
  publish(topics::kVehicleChassis, carStateToChassis(msg.car_state(), steer_ratio_));
}

void TopicConvertService::onImu(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_imu_data())
    return;
  publish(topics::kImuRaw, imuToRaw(msg.imu_data()));
}

void TopicConvertService::onGps(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_gps_location())
    return;
  const auto& g = msg.gps_location();
  const int64_t t_us = g.timestamp() * 1000;
  const bool ok_fix =
      g.fix_type() != ai::flow::adas::GPSLocation::NO_FIX && g.fix_type() != ai::flow::adas::GPSLocation::TIME_ONLY;
  auto sample = gps_proj_.project(t_us, g.latitude(), g.longitude(), ok_fix, g.speed(), g.bearing());
  if (!sample.valid)
    return;
  publish(topics::kGpsLocation, sample);
}

void TopicConvertService::onLaneUv(const ai::flow::adas::ZMQMessage& /*msg*/) {}

void TopicConvertService::onCameraOdometry(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_camera_odometry())
    return;
  auto sample = cameraOdometryToSample(msg.camera_odometry());
  if (!sample.valid)
    return;
  publish(topics::kCameraOdometry, sample);
}

}  // namespace adas
