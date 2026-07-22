#pragma once

#include <string>
#include <vector>

#include "framework/service_manager.hpp"
#include "utils/adas_topics.h"
#include "utils/lat_control_pid.h"
#include "utils/math_utils.h"
#include "utils/pure_pursuit.h"

namespace adas {

struct LaneKeepOutput {
  int64_t timestamp_us = 0;
  double steer_rad = 0.0;        // desired road-wheel [rad] from PP
  double steer_norm = 0.0;       // torque command ∈ [-1,1] after angle PID
  double desired_swa_deg = 0.0;  // desired steering-wheel angle [deg]
  double actual_swa_deg = 0.0;   // measured SWA [deg]
  double angle_error_deg = 0.0;
  double throttle = 0.0;
  double brake = 0.0;
  double lookahead_m = 0.0;
  double target_x = 0.0;
  double target_y = 0.0;
  bool has_target = false;
  double curvature = 0.0;
  std::string status = "ok";
};

/**
 * Lane-keep: Pure Pursuit → desired SWA, then flowpilot-style LatControlPID
 * (desired vs actual steering-wheel angle → torque).
 *
 * Topics: sub vision/path + vehicle/chassis → pub control/lane_keep (+ controls/steer).
 */
class LaneKeepService : public microros::Service {
public:
  LaneKeepService(double wheelbase = 2.636, double desired_speed = 12.0, double max_steer_deg = 8.0,
                  double pp_k_dd = 0.4, double pp_ld_min = 3.0, double pp_ld_max = 20.0, double pp_shift = 1.4,
                  double max_torque_cnm = 300.0, double steer_ratio = 15.7, double pid_kp = 0.6, double pid_ki = 0.2,
                  double pid_kf = 0.00006);

  void configure() override;
  void reset() override;

  LaneKeepOutput step(double speed_mps, const std::vector<Vec2>& polyline_ego);

  PurePursuit& purePursuit() { return pp_; }
  const LaneKeepOutput& last() const { return last_; }

  /** When false, controls/steer is published with enabled=false (no torque). */
  void setSteerOutputEnabled(bool enabled) { steer_output_enabled_ = enabled; }
  bool steerOutputEnabled() const { return steer_output_enabled_; }

  void setSteerRatio(double ratio) { steer_ratio_ = std::max(ratio, 1e-3); }
  void setPidGains(double kp, double ki, double kf) { lat_.setGains(kp, ki, kf); }

private:
  void onChassis(const ChassisSample& msg);
  void onLanes(const LanePathMsg& msg);
  void publishLaneKeep(const LaneKeepOutput& out);
  void publishSteer(const LaneKeepOutput& out);
  /** Run angle PID with last PP desire + current chassis; publish steer. */
  void updateTorqueFromAngle();

  PurePursuit pp_;
  LatControlPid lat_;
  double desired_speed_ = 12.0;
  double max_steer_rad_ = 20.0 * M_PI / 180.0;
  double speed_kp_ = 0.08;
  double max_torque_cnm_ = 300.0;
  double steer_ratio_ = 15.7;
  bool steer_output_enabled_ = false;

  ChassisSample chassis_;
  bool have_chassis_ = false;
  double desired_swa_deg_ = 0.0;
  bool have_desired_ = false;
  LaneKeepOutput last_;
};

}  // namespace adas
