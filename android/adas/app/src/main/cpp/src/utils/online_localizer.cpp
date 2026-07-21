#include "utils/online_localizer.h"

#include <cmath>

namespace adas {

OnlineLocalizer::OnlineLocalizer(double wheelbase, double gps_noise_pos, double gps_update_interval,
                                 bool imu_every_step)
  : ekf_(wheelbase, gps_noise_pos, 0.05)
  , wheelbase_(wheelbase)
  , gps_update_interval_(gps_update_interval)
  , imu_every_step_(imu_every_step)
{
}

void OnlineLocalizer::record(double rx, double ry, double ox, double oy, double ex, double ey)
{
  ref_x_.push_back(rx);
  ref_y_.push_back(ry);
  odom_traj_x_.push_back(ox);
  odom_traj_y_.push_back(oy);
  ekf_traj_x_.push_back(ex);
  ekf_traj_y_.push_back(ey);
}

void OnlineLocalizer::reset(double x, double y, double yaw, double v, double yaw_rate)
{
  ekf_.reset(x, y, yaw, v, yaw_rate, 1.0, 0.2, 1.0, 0.1);
  odom_x_ = x;
  odom_y_ = y;
  odom_yaw_ = yaw;
  t_ = 0;
  last_gps_t_ = -1e9;
  initialized_ = true;
  ref_x_.clear();
  ref_y_.clear();
  odom_traj_x_.clear();
  odom_traj_y_.clear();
  ekf_traj_x_.clear();
  ekf_traj_y_.clear();
  record(x, y, x, y, x, y);
}

std::tuple<double, double, double> OnlineLocalizer::step(double dt, double speed_mps, double steer_rad,
                                                         std::optional<double> yaw_rate, std::optional<Vec2> gps_xy,
                                                         std::optional<Vec2> ref_xy)
{
  if (!initialized_) {
    const Vec2 seed = ref_xy.value_or(gps_xy.value_or(Vec2{0, 0}));
    reset(seed.x, seed.y, 0.0, speed_mps, yaw_rate.value_or(0.0));
  }

  dt = std::max(dt, 1e-4);
  const double v = speed_mps;
  const double delta = steer_rad;
  double yr = 0.0;
  if (std::abs(delta) > 1e-3 && std::abs(v) > 0.01) {
    yr = v * std::tan(delta) / wheelbase_;
  }
  odom_x_ += v * std::cos(odom_yaw_) * dt;
  odom_y_ += v * std::sin(odom_yaw_) * dt;
  odom_yaw_ = normalizeAngle(odom_yaw_ + yr * dt);

  ekf_.predict(v, delta, dt);
  if (yaw_rate && imu_every_step_)
    ekf_.updateImu(*yaw_rate);

  t_ += dt;
  if (gps_xy && (t_ - last_gps_t_) >= gps_update_interval_) {
    if (ekf_.updateGps(gps_xy->x, gps_xy->y))
      last_gps_t_ = t_;
  }

  const double ex = ekf_.x(), ey = ekf_.y();
  double rx = ex, ry = ey;
  if (ref_xy) {
    rx = ref_xy->x;
    ry = ref_xy->y;
  } else if (gps_xy) {
    rx = gps_xy->x;
    ry = gps_xy->y;
  }
  record(rx, ry, odom_x_, odom_y_, ex, ey);
  return {ex, ey, ekf_.yaw()};
}

}  // namespace adas
