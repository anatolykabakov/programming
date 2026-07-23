#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "middleware/middleware.hpp"
#include "utils/adas_topics.h"
#include "utils/lat_control_pid.h"
#include "utils/math_utils.h"
#include "utils/pure_pursuit.h"

namespace adas {

struct LaneKeepOutput {
  /** @deprecated prefer publish_ts_us; kept as publish time for bindings. */
  int64_t timestamp_us = 0;
  int64_t capture_ts_us = 0;  // camera frame arrival
  int64_t vision_ts_us = 0;   // ONNX / path ready (infer)
  int64_t chassis_ts_us = 0;
  int64_t publish_ts_us = 0;
  double steer_rad = 0.0;
  double steer_norm = 0.0;
  double desired_swa_deg = 0.0;
  double actual_swa_deg = 0.0;
  double angle_error_deg = 0.0;
  double lookahead_m = 0.0;
  double target_x = 0.0;
  double target_y = 0.0;
  bool has_target = false;
  double curvature = 0.0;
  std::string status = "ok";
};

class LaneKeepService : public adas::Service {
public:
  struct Config {
    double wheelbase_m = 2.636;
    double max_steer_deg = 8.0;
    double pp_k_dd = 0.4;
    double pp_ld_min = 3.0;
    double pp_ld_max = 20.0;
    double pp_shift = 1.4;
    double max_torque_cnm = 300.0;
    double steer_ratio = 15.7;
    double pid_kp = 0.6;
    double pid_ki = 0.2;
    double pid_kf = 0.00006;
    bool steer_output_enabled = false;
    double steer_sign = -1.0;
  };

  LaneKeepService() : LaneKeepService(Config{}) {}
  explicit LaneKeepService(Config config);

  void configure() override;
  void reset() override;
  std::string_view getName() const override { return "lane_keep"; }

  LaneKeepOutput step(double speed_mps, const std::vector<Vec2>& polyline_ego);

  PurePursuit& purePursuit() { return pp_; }
  const LaneKeepOutput& last() const { return last_; }
  const Config& config() const { return config_; }

  void setPurePursuit(double k_dd, double ld_min, double ld_max, double shift);
  void setMaxSteerDeg(double max_steer_deg);

  void setSteerOutputEnabled(bool enabled) { steer_output_enabled_ = enabled; }
  bool steerOutputEnabled() const { return steer_output_enabled_; }

  void setSteerRatio(double ratio) { steer_ratio_ = std::max(ratio, 1e-3); }
  void setPidGains(double kp, double ki, double kf) { lat_.setGains(kp, ki, kf); }
  void setSteerSign(double sign) { steer_sign_ = (sign < 0.0) ? -1.0 : 1.0; }

private:
  void onChassis(const ChassisSample& msg);
  void onLanes(const LanePathMsg& msg);
  void publishLaneKeep(const LaneKeepOutput& out);
  void publishSteer(const LaneKeepOutput& out);
  void updateTorqueFromAngle();

  Config config_;
  PurePursuit pp_;
  LatControlPid lat_;
  double max_steer_rad_ = 8.0 * M_PI / 180.0;
  double max_torque_cnm_ = 300.0;
  double steer_ratio_ = 15.7;
  double steer_sign_ = -1.0;
  bool steer_output_enabled_ = false;

  ChassisSample chassis_;
  bool have_chassis_ = false;
  double desired_swa_deg_ = 0.0;
  bool have_desired_ = false;
  LaneKeepOutput last_;
};

}  // namespace adas
