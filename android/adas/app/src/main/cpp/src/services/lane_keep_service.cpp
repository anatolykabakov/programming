#include "services/lane_keep_service.h"

#include <cmath>

#include "messages.pb.h"
#include "utils/logger.h"
#include "utils/protobuf_utils.h"

namespace adas {
namespace {

int64_t nowUs()
{
  return utils::getCurrentTimestamp() * 1000;  // BOOTTIME ms → us
}

}  // namespace

LaneKeepService::LaneKeepService(Config p)
  : config_(p)
  , pp_(p.pp_k_dd, p.wheelbase_m, p.pp_shift, p.pp_ld_min, p.pp_ld_max)
  , lat_(p.pid_kp, p.pid_ki, p.pid_kf, 50.0)
  , max_steer_rad_(p.max_steer_deg * M_PI / 180.0)
  , max_torque_cnm_(p.max_torque_cnm)
  , steer_ratio_(std::max(p.steer_ratio, 1e-3))
  , steer_sign_(p.steer_sign < 0.0 ? -1.0 : 1.0)
  , steer_output_enabled_(p.steer_output_enabled)
{
}

void LaneKeepService::configure()
{
  subscribe<ChassisSample>(topics::kVehicleChassis, [this](const ChassisSample& m) { onChassis(m); });
  subscribe<LanePathMsg>(topics::kVisionPath, [this](const LanePathMsg& m) { onLanes(m); });
  LOGI("LaneKeepService: PP + LatControlPID  ratio=%.1f  → %s / %s", steer_ratio_, topics::kLaneKeep,
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

void LaneKeepService::setPurePursuit(double k_dd, double ld_min, double ld_max, double shift)
{
  pp_.K_dd = k_dd;
  pp_.ld_min = ld_min;
  pp_.ld_max = ld_max;
  pp_.waypoint_shift = shift;
  config_.pp_k_dd = k_dd;
  config_.pp_ld_min = ld_min;
  config_.pp_ld_max = ld_max;
  config_.pp_shift = shift;
}

void LaneKeepService::setMaxSteerDeg(double max_steer_deg)
{
  config_.max_steer_deg = max_steer_deg;
  max_steer_rad_ = max_steer_deg * M_PI / 180.0;
}

void LaneKeepService::onChassis(const ChassisSample& msg)
{
  chassis_ = msg;
  have_chassis_ = true;
  if (have_desired_)
    updateTorqueFromAngle();
}

void LaneKeepService::onLanes(const LanePathMsg& msg)
{
  const double speed = have_chassis_ ? chassis_.speed_mps : 0.0;
  auto out = step(speed, msg.polyline);
  out.capture_ts_us = msg.capture_ts_us > 0 ? msg.capture_ts_us : (msg.timestamp_us > 0 ? msg.timestamp_us : 0);
  // vision = ONNX done; do not fall back to capture timestamp
  out.vision_ts_us = msg.infer_ts_us > 0 ? msg.infer_ts_us : 0;
  out.chassis_ts_us = have_chassis_ ? chassis_.timestamp_us : 0;

  if (out.has_target && out.status == "ok") {
    // PP δ is device-frame (Y right+); SWA/torque for VW are left-positive.
    desired_swa_deg_ = steer_sign_ * (out.steer_rad * 180.0 / M_PI) * steer_ratio_;
    have_desired_ = true;
  } else {
    have_desired_ = false;
    desired_swa_deg_ = 0.0;
    lat_.reset();
  }

  out.desired_swa_deg = desired_swa_deg_;
  if (have_chassis_) {
    out.actual_swa_deg = chassis_.steering_angle_deg;
    out.angle_error_deg = desired_swa_deg_ - chassis_.steering_angle_deg;
  }
  last_ = out;
  // Publish geometric PP first (steer_norm = δ/δ_max). LatControlPid overwrites
  // steer_norm for torque on controls/steer — must not clobber lane_keep for host/sim.
  publishLaneKeep(last_);
  updateTorqueFromAngle();
}

void LaneKeepService::updateTorqueFromAngle()
{
  LaneKeepOutput out = last_;
  out.chassis_ts_us = have_chassis_ ? chassis_.timestamp_us : out.chassis_ts_us;
  // Keep vision_ts from last lane update — do not replace with chassis time.

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
  const int64_t publish_ms = nowUs() / 1000;
  const int64_t capture_ms = out.capture_ts_us / 1000;
  const int64_t vision_ms = out.vision_ts_us / 1000;
  const int64_t chassis_ms = out.chassis_ts_us / 1000;

  ai::flow::adas::ZMQMessage lk_zmq;
  lk_zmq.set_timestamp(publish_ms);
  lk_zmq.set_topic(topics::kLaneKeep);
  auto* lk = lk_zmq.mutable_lane_keep();
  lk->set_timestamp(publish_ms);
  lk->set_steer_rad(out.steer_rad);
  lk->set_steer_norm(out.steer_norm);
  lk->set_throttle(0.0);
  lk->set_brake(0.0);
  lk->set_lookahead_m(out.lookahead_m);
  lk->set_target_x(out.target_x);
  lk->set_target_y(out.target_y);
  lk->set_has_target(out.has_target);
  lk->set_curvature(out.curvature);
  lk->set_status(out.status);
  lk->set_capture_ts_ms(capture_ms);
  lk->set_vision_ts_ms(vision_ms);
  lk->set_chassis_ts_ms(chassis_ms);
  lk->set_publish_ts_ms(publish_ms);
  publish(topics::kLaneKeep, lk_zmq);
}

void LaneKeepService::publishSteer(const LaneKeepOutput& out)
{
  const int64_t publish_ms = nowUs() / 1000;
  const int64_t capture_ms = out.capture_ts_us / 1000;
  const int64_t vision_ms = out.vision_ts_us / 1000;
  const int64_t chassis_ms = out.chassis_ts_us / 1000;

  ai::flow::adas::ZMQMessage zmq;
  zmq.set_timestamp(publish_ms);
  zmq.set_topic(topics::kSteerCommand);
  auto* cmd = zmq.mutable_steer_command();
  const int torque = static_cast<int>(std::lround(out.steer_norm * max_torque_cnm_));
  const bool en = steer_output_enabled_ && out.has_target && out.status == "ok" && have_desired_;
  cmd->set_torque_cnm(en ? torque : 0);
  cmd->set_enabled(en);
  cmd->set_capture_ts_ms(capture_ms);
  cmd->set_vision_ts_ms(vision_ms);
  cmd->set_chassis_ts_ms(chassis_ms);
  cmd->set_publish_ts_ms(publish_ms);
  publish(topics::kSteerCommand, zmq);
}

LaneKeepOutput LaneKeepService::step(double speed_mps, const std::vector<Vec2>& polyline_ego)
{
  LaneKeepOutput out;
  if (polyline_ego.size() < 2) {
    out.status = "no_polyline";
    return out;
  }

  const auto pp = pp_.compute(polyline_ego, speed_mps);
  out.steer_rad = pp.steer_rad;
  if (max_steer_rad_ > 1e-6)
    out.steer_rad = std::clamp(out.steer_rad, -max_steer_rad_, max_steer_rad_);
  out.steer_norm = max_steer_rad_ > 1e-6 ? out.steer_rad / max_steer_rad_ : 0.0;
  out.lookahead_m = pp.lookahead_m;
  out.curvature = pp.curvature();
  if (pp.target_ego) {
    out.has_target = true;
    out.target_x = pp.target_ego->x();
    out.target_y = pp.target_ego->y();
  }
  out.status = "ok";
  return out;
}

}  // namespace adas
