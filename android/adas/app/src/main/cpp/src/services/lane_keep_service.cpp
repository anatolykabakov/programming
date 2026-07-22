#include "services/lane_keep_service.h"

#include <algorithm>
#include <cmath>

#include "messages.pb.h"
#include "utils/logger.h"

namespace adas {

LaneKeepService::LaneKeepService(double wheelbase, double desired_speed, double max_steer_deg, double pp_k_dd,
                                 double pp_ld_min, double pp_ld_max, double pp_shift, double max_torque_cnm,
                                 double steer_ratio, double pid_kp, double pid_ki, double pid_kf)
  : pp_(pp_k_dd, wheelbase, pp_shift, pp_ld_min, pp_ld_max)
  , lat_(pid_kp, pid_ki, pid_kf, /*rate_hz=*/50.0)
  , desired_speed_(desired_speed)
  , max_steer_rad_(max_steer_deg * M_PI / 180.0)
  , max_torque_cnm_(max_torque_cnm)
  , steer_ratio_(std::max(steer_ratio, 1e-3))
{
}

void LaneKeepService::configure()
{
  subscribe<ChassisSample>(topics::kVehicleChassis, [this](const ChassisSample& m) { onChassis(m); });
  subscribe<LanePathMsg>(topics::kVisionPath, [this](const LanePathMsg& m) { onLanes(m); });
  LOGI("LaneKeepService: PP + LatControlPID (angle→torque)  ratio=%.1f  → %s / %s", steer_ratio_, topics::kLaneKeep,
       topics::kSteerCommand);
}

void LaneKeepService::reset()
{
  last_ = LaneKeepOutput{};
  have_chassis_ = false;
  have_desired_ = false;
  desired_swa_deg_ = 0.0;
  lat_.reset();
}

void LaneKeepService::onChassis(const ChassisSample& msg)
{
  chassis_ = msg;
  have_chassis_ = true;
  // High-rate torque update so PID tracks LWI angle between vision frames.
  if (have_desired_)
    updateTorqueFromAngle();
}

void LaneKeepService::onLanes(const LanePathMsg& msg)
{
  const double speed = have_chassis_ ? chassis_.speed_mps : 0.0;
  auto out = step(speed, msg.polyline);
  out.timestamp_us = msg.timestamp_us > 0 ? msg.timestamp_us : (have_chassis_ ? chassis_.timestamp_us : 0);

  if (out.has_target && out.status == "ok") {
    // PP outputs road-wheel; LatControlPID uses steering-wheel degrees (flowpilot).
    desired_swa_deg_ = (out.steer_rad * 180.0 / M_PI) * steer_ratio_;
    have_desired_ = true;
  } else {
    have_desired_ = false;
    desired_swa_deg_ = 0.0;
    lat_.reset();
  }

  // Fill angle fields for bag/HUD; torque from PID below.
  out.desired_swa_deg = desired_swa_deg_;
  if (have_chassis_) {
    out.actual_swa_deg = chassis_.steering_angle_deg;
    out.angle_error_deg = desired_swa_deg_ - chassis_.steering_angle_deg;
  }
  last_ = out;
  publishLaneKeep(out);
  updateTorqueFromAngle();
}

void LaneKeepService::updateTorqueFromAngle()
{
  LaneKeepOutput out = last_;
  out.timestamp_us = have_chassis_ ? chassis_.timestamp_us : out.timestamp_us;

  const bool active = steer_output_enabled_ && have_desired_ && have_chassis_ && out.has_target && out.status == "ok";
  const auto lat =
      lat_.update(active, desired_swa_deg_, chassis_.steering_angle_deg, chassis_.speed_mps, chassis_.steering_pressed);

  out.desired_swa_deg = lat.angle_des_deg;
  out.actual_swa_deg = lat.angle_act_deg;
  out.angle_error_deg = lat.angle_error_deg;
  out.steer_norm = lat.steer_norm;
  last_ = out;
  publishSteer(out);
}

void LaneKeepService::publishLaneKeep(const LaneKeepOutput& out)
{
  ai::flow::adas::ZMQMessage lk_zmq;
  lk_zmq.set_timestamp(out.timestamp_us / 1000);
  lk_zmq.set_topic(topics::kLaneKeep);
  auto* lk = lk_zmq.mutable_lane_keep();
  lk->set_timestamp(out.timestamp_us / 1000);
  lk->set_steer_rad(out.steer_rad);
  lk->set_steer_norm(out.steer_norm);
  lk->set_throttle(out.throttle);
  lk->set_brake(out.brake);
  lk->set_lookahead_m(out.lookahead_m);
  lk->set_target_x(out.target_x);
  lk->set_target_y(out.target_y);
  lk->set_has_target(out.has_target);
  lk->set_curvature(out.curvature);
  lk->set_status(out.status);
  publish(topics::kLaneKeep, lk_zmq);
}

void LaneKeepService::publishSteer(const LaneKeepOutput& out)
{
  ai::flow::adas::ZMQMessage zmq;
  zmq.set_timestamp(out.timestamp_us / 1000);
  zmq.set_topic(topics::kSteerCommand);
  auto* cmd = zmq.mutable_steer_command();
  const int torque = static_cast<int>(std::lround(out.steer_norm * max_torque_cnm_));
  const bool en = steer_output_enabled_ && out.has_target && out.status == "ok" && have_desired_;
  cmd->set_torque_cnm(en ? torque : 0);
  cmd->set_enabled(en);
  publish(topics::kSteerCommand, zmq);
}

LaneKeepOutput LaneKeepService::step(double speed_mps, const std::vector<Vec2>& polyline_ego)
{
  LaneKeepOutput out;
  const double err = desired_speed_ - std::max(0.0, speed_mps);
  if (err > 0.5)
    out.throttle = std::min(0.85, speed_kp_ * err);
  else if (err < -1.0)
    out.brake = std::min(0.5, 0.05 * (-err));

  if (polyline_ego.size() < 2) {
    out.status = "no_polyline";
    return out;
  }

  const auto pp = pp_.compute(polyline_ego, speed_mps);
  out.steer_rad = pp.steer_rad;
  // Soft clamp road-wheel desire (safety); PID still tracks measured SWA.
  if (max_steer_rad_ > 1e-6)
    out.steer_rad = std::clamp(out.steer_rad, -max_steer_rad_, max_steer_rad_);
  out.lookahead_m = pp.lookahead_m;
  out.curvature = pp.curvature();
  if (pp.target_ego) {
    out.has_target = true;
    out.target_x = pp.target_ego->x;
    out.target_y = pp.target_ego->y;
  }
  out.status = "ok";
  return out;
}

}  // namespace adas
