#include "services/topic_convert_service.h"

#include "utils/logger.h"
#include "utils/topic_convert.h"

namespace adas {

TopicConvertService::TopicConvertService(double steer_ratio) : steer_ratio_(steer_ratio) {}

void TopicConvertService::configure()
{
  subscribe<ai::flow::adas::ZMQMessage>(topics::kVisionLanes,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onVisionLanes(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kVehicleState,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onVehicleState(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kImu, [this](const ai::flow::adas::ZMQMessage& m) { onImu(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kGpsLocation,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onGps(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kCalibLaneUv,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onLaneUv(m); });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kCameraOdometry,
                                        [this](const ai::flow::adas::ZMQMessage& m) { onCameraOdometry(m); });
  LOGI("TopicConvertService: lanes/state/imu/gps/lane_uv/cam_odom → typed");
}

void TopicConvertService::reset() { gps_proj_.reset(); }

void TopicConvertService::onVisionLanes(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_lane_lines()) {
    LOGW("vision/lanes without lane_lines payload");
    return;
  }
  auto path = laneLinesToPath(msg.lane_lines());
  LOGI("vision/lanes → path n=%zu frame=%d (plan+lane blend)", path.polyline.size(), path.frame_id);
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
  auto sample = gps_proj_.project(t_us, g.latitude(), g.longitude(), ok_fix);
  if (!sample.valid)
    return;
  if (gps_proj_.haveOrigin() && sample.x == 0.0 && sample.y == 0.0) {
    // first fix — still valid origin
  }
  publish(topics::kGpsLocation, sample);
}

void TopicConvertService::onLaneUv(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_lane_uv())
    return;
  const auto& p = msg.lane_uv();
  LaneUvMsg out;
  out.timestamp_us = p.timestamp() * 1000;
  out.left_uv.reserve(p.left_uv_size());
  out.right_uv.reserve(p.right_uv_size());
  for (const auto& pt : p.left_uv()) {
    out.left_uv.push_back(Vec2{pt.u(), pt.v()});
  }
  for (const auto& pt : p.right_uv()) {
    out.right_uv.push_back(Vec2{pt.u(), pt.v()});
  }
  if (out.left_uv.size() < 2 || out.right_uv.size() < 2)
    return;
  publish(topics::kCalibLaneUv, out);
}

void TopicConvertService::onCameraOdometry(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_camera_odometry())
    return;
  const auto& p = msg.camera_odometry();
  CameraOdometrySample out;
  out.timestamp_us = p.timestamp() * 1000;
  out.valid = p.trans_size() >= 3 && p.rot_size() >= 3;
  if (!out.valid)
    return;
  for (int i = 0; i < 3; ++i) {
    out.trans[i] = p.trans(i);
    out.rot[i] = p.rot(i);
    out.trans_std[i] = (p.trans_std_size() > i) ? p.trans_std(i) : 1.0;
    out.rot_std[i] = (p.rot_std_size() > i) ? p.rot_std(i) : 1.0;
  }
  publish(topics::kCameraOdometry, out);
}

}  // namespace adas
