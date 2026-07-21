#pragma once

#include <string>
#include <vector>

#include "framework/service_manager.hpp"
#include "utils/adas_topics.h"
#include "utils/math_utils.h"
#include "utils/pure_pursuit.h"

namespace adas {

struct LaneKeepOutput {
  int64_t timestamp_us = 0;
  double steer_rad = 0.0;
  double steer_norm = 0.0;
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
 * Lane-keep (Pure Pursuit).
 * Topics: sub vision/path + vehicle/chassis → pub control/lane_keep (+ controls/steer).
 * ``vision/path`` is produced by TopicConvertService from Android ``vision/lanes``.
 */
class LaneKeepService : public microros::Service {
public:
  LaneKeepService(double wheelbase = 2.636, double desired_speed = 12.0, double max_steer_deg = 40.0,
                  double pp_k_dd = 0.4, double pp_ld_min = 3.0, double pp_ld_max = 20.0, double pp_shift = 1.4,
                  double max_torque_cnm = 300.0);

  void configure() override;
  void reset() override;

  LaneKeepOutput step(double speed_mps, const std::vector<Vec2>& polyline_ego);

  PurePursuit& purePursuit() { return pp_; }
  const LaneKeepOutput& last() const { return last_; }

  /** When false, controls/steer is published with enabled=false (no torque). */
  void setSteerOutputEnabled(bool enabled) { steer_output_enabled_ = enabled; }
  bool steerOutputEnabled() const { return steer_output_enabled_; }

private:
  void onChassis(const ChassisSample& msg);
  void onLanes(const LanePathMsg& msg);
  void publishOutputs(const LaneKeepOutput& out);

  PurePursuit pp_;
  double desired_speed_ = 12.0;
  double max_steer_rad_ = 0.7;
  double speed_kp_ = 0.08;
  double max_torque_cnm_ = 300.0;
  bool steer_output_enabled_ = false;
  ChassisSample chassis_;
  bool have_chassis_ = false;
  LaneKeepOutput last_;
};

}  // namespace adas
