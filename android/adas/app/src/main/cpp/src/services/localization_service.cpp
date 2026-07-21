#include "services/localization_service.h"

#include <cmath>

#include "messages.pb.h"
#include "utils/logger.h"

namespace adas {

LocalizationService::LocalizationService(double wheelbase, double gps_noise_pos, double gps_update_interval)
  : loc_(wheelbase, gps_noise_pos, gps_update_interval, true)
{
}

void LocalizationService::configure()
{
  subscribe<ChassisSample>(topics::kVehicleChassis, [this](const ChassisSample& m) { onChassis(m); });
  subscribe<GpsSample>(topics::kGpsLocation, [this](const GpsSample& m) { onGps(m); });
  subscribe<ImuSample>(topics::kImuYaw, [this](const ImuSample& m) { onImu(m); });
  LOGI("LocalizationService: subscribed chassis/gps/%s → %s", topics::kImuYaw, topics::kLocalizationPose);
}

void LocalizationService::reset()
{
  loc_.reset();
  have_chassis_ = false;
  last_t_us_ = 0;
  last_pose_ = LocalizationPose{};
}

void LocalizationService::resetPose(double x, double y, double yaw, double v, double yaw_rate)
{
  loc_.reset(x, y, yaw, v, yaw_rate);
  last_pose_.x = x;
  last_pose_.y = y;
  last_pose_.yaw = yaw;
  last_pose_.v = v;
  last_pose_.yaw_rate = yaw_rate;
}

void LocalizationService::onGps(const GpsSample& msg) { gps_ = msg; }

void LocalizationService::onImu(const ImuSample& msg) { imu_ = msg; }

void LocalizationService::onChassis(const ChassisSample& msg)
{
  double dt = 0.05;
  if (last_t_us_ > 0 && msg.timestamp_us > last_t_us_) {
    dt = (msg.timestamp_us - last_t_us_) * 1e-6;
  }
  last_t_us_ = msg.timestamp_us;
  chassis_ = msg;
  have_chassis_ = true;

  std::optional<double> yr;
  if (imu_.valid) {
    yr = imu_.yaw_rate;
  } else if (std::abs(msg.yaw_rate) > 1e-9) {
    yr = msg.yaw_rate;
  }

  std::optional<double> gx, gy;
  if (gps_.valid) {
    gx = gps_.x;
    gy = gps_.y;
  }

  step(dt, msg.speed_mps, msg.steer_rad, yr, gx, gy, std::nullopt, std::nullopt);
  publishPose(msg.timestamp_us);
}

void LocalizationService::publishPose(int64_t timestamp_us)
{
  LocalizationPose pose;
  pose.timestamp_us = timestamp_us;
  pose.x = loc_.x();
  pose.y = loc_.y();
  pose.yaw = loc_.yaw();
  pose.v = loc_.ekf().v();
  pose.yaw_rate = loc_.ekf().yawRate();
  pose.odom_x = loc_.odomXNow();
  pose.odom_y = loc_.odomYNow();
  pose.ekf_x = loc_.x();
  pose.ekf_y = loc_.y();
  last_pose_ = pose;

  ai::flow::adas::ZMQMessage zmq;
  zmq.set_timestamp(timestamp_us / 1000);
  zmq.set_topic(topics::kLocalizationPose);
  auto* p = zmq.mutable_localization_pose();
  p->set_timestamp(timestamp_us / 1000);
  p->set_x(pose.x);
  p->set_y(pose.y);
  p->set_yaw(pose.yaw);
  p->set_v(pose.v);
  p->set_yaw_rate(pose.yaw_rate);
  p->set_odom_x(pose.odom_x);
  p->set_odom_y(pose.odom_y);
  p->set_ekf_x(pose.ekf_x);
  p->set_ekf_y(pose.ekf_y);
  publish(topics::kLocalizationPose, zmq);
}

std::tuple<double, double, double> LocalizationService::step(double dt, double speed_mps, double steer_rad,
                                                             std::optional<double> yaw_rate,
                                                             std::optional<double> gps_x, std::optional<double> gps_y,
                                                             std::optional<double> ref_x, std::optional<double> ref_y)
{
  std::optional<Vec2> gps, ref;
  if (gps_x && gps_y)
    gps = Vec2{*gps_x, *gps_y};
  if (ref_x && ref_y)
    ref = Vec2{*ref_x, *ref_y};
  return loc_.step(dt, speed_mps, steer_rad, yaw_rate, gps, ref);
}

}  // namespace adas
