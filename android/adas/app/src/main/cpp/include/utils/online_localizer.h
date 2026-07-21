#pragma once

#include <optional>
#include <tuple>
#include <vector>

#include "utils/math_utils.h"
#include "utils/vehicle_ekf.h"

namespace adas {

class OnlineLocalizer {
public:
  OnlineLocalizer(double wheelbase = 2.636, double gps_noise_pos = 0.5, double gps_update_interval = 0.2,
                  bool imu_every_step = true);

  void reset(double x = 0, double y = 0, double yaw = 0, double v = 0, double yaw_rate = 0);

  /** Returns (ekf_x, ekf_y, ekf_yaw). */
  std::tuple<double, double, double> step(double dt, double speed_mps, double steer_rad,
                                          std::optional<double> yaw_rate = std::nullopt,
                                          std::optional<Vec2> gps_xy = std::nullopt,
                                          std::optional<Vec2> ref_xy = std::nullopt);

  double x() const { return ekf_.x(); }
  double y() const { return ekf_.y(); }
  double yaw() const { return ekf_.yaw(); }
  double odomXNow() const { return odom_x_; }
  double odomYNow() const { return odom_y_; }
  VehicleEKF& ekf() { return ekf_; }
  const VehicleEKF& ekf() const { return ekf_; }

  const std::vector<double>& refX() const { return ref_x_; }
  const std::vector<double>& refY() const { return ref_y_; }
  const std::vector<double>& odomX() const { return odom_traj_x_; }
  const std::vector<double>& odomY() const { return odom_traj_y_; }
  const std::vector<double>& ekfX() const { return ekf_traj_x_; }
  const std::vector<double>& ekfY() const { return ekf_traj_y_; }

private:
  void record(double rx, double ry, double ox, double oy, double ex, double ey);

  VehicleEKF ekf_;
  double wheelbase_ = 2.636;
  double gps_update_interval_ = 0.2;
  bool imu_every_step_ = true;
  bool initialized_ = false;
  double odom_x_ = 0, odom_y_ = 0, odom_yaw_ = 0;
  double t_ = 0, last_gps_t_ = -1e9;

  std::vector<double> ref_x_, ref_y_;
  std::vector<double> odom_traj_x_, odom_traj_y_;
  std::vector<double> ekf_traj_x_, ekf_traj_y_;
};

}  // namespace adas
