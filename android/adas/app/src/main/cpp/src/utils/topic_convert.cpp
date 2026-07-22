#include "utils/topic_convert.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace adas {
namespace {

double softLaneProb(float p, float min_p)
{
  if (!(p >= min_p))
    return 0.0;
  const double span = std::max(1e-3, 1.0 - static_cast<double>(min_p));
  return std::min(1.0, (static_cast<double>(p) - min_p) / span);
}

/** Linear interp of y(x) on a sorted-ish sample grid; clamps at ends. */
double interpY(double x, const std::vector<double>& xs, const std::vector<double>& ys)
{
  if (xs.empty() || xs.size() != ys.size())
    return 0.0;
  if (x <= xs.front())
    return ys.front();
  if (x >= xs.back())
    return ys.back();
  for (size_t i = 1; i < xs.size(); ++i) {
    if (x <= xs[i]) {
      const double x0 = xs[i - 1];
      const double x1 = xs[i];
      const double t = (x1 > x0) ? (x - x0) / (x1 - x0) : 0.0;
      return ys[i - 1] + t * (ys[i] - ys[i - 1]);
    }
  }
  return ys.back();
}

}  // namespace

LanePathMsg laneLinesToPath(const ai::flow::adas::LaneLines& ll, float min_lane_prob)
{
  LanePathMsg out;
  out.timestamp_us = ll.timestamp() * 1000;
  out.frame_id = ll.frame_id();

  // --- base: best PLAN hypothesis ---
  const int np = std::min(ll.plan_x_size(), ll.plan_y_size());
  if (np >= 2) {
    out.polyline.reserve(static_cast<size_t>(np));
    for (int i = 0; i < np; ++i) {
      const double x = ll.plan_x(i);
      if (x >= 1.0)
        out.polyline.push_back({x, ll.plan_y(i)});
    }
  }

  // Near lanes: 1=leftNear, 2=rightNear (Android ProtoUtils order), Y left.
  const int nx = ll.x_size();
  const bool geom_ok = nx >= 2;
  const bool have_l = geom_ok && ll.lanes_size() > 1 && ll.lanes(1).y_size() == nx;
  const bool have_r = geom_ok && ll.lanes_size() > 2 && ll.lanes(2).y_size() == nx;

  double l_prob = have_l ? softLaneProb(ll.lanes(1).prob(), min_lane_prob) : 0.0;
  double r_prob = have_r ? softLaneProb(ll.lanes(2).prob(), min_lane_prob) : 0.0;
  // Stock OP: d_prob = l + r - l*r
  const double d_prob = l_prob + r_prob - l_prob * r_prob;

  if (d_prob <= 1e-6) {
    // No usable lanes: plan only (or empty if plan missing).
    if (out.polyline.size() >= 2)
      return out;
  }

  // Build lane sample grid for interp onto plan X.
  std::vector<double> xs;
  std::vector<double> lane_ys;
  if (d_prob > 1e-6 && (have_l || have_r)) {
    xs.reserve(static_cast<size_t>(nx));
    lane_ys.reserve(static_cast<size_t>(nx));
    for (int i = 0; i < nx; ++i) {
      const double x = ll.x(i);
      double yl = have_l ? ll.lanes(1).y(i) : 0.0;
      double yr = have_r ? ll.lanes(2).y(i) : 0.0;
      double lane_y;
      if (have_l && have_r) {
        // Y-left: left > right; center from each side with clipped width (stock-style).
        const double width = std::abs(yl - yr);
        const double w = std::min(4.0, std::max(2.6, width));
        const double from_l = yl - 0.5 * w;
        const double from_r = yr + 0.5 * w;
        lane_y = (l_prob * from_l + r_prob * from_r) / (l_prob + r_prob + 1e-6);
      } else if (have_l) {
        lane_y = yl - 1.6;  // ~half typical lane when only left visible
      } else {
        lane_y = yr + 1.6;
      }
      xs.push_back(x);
      lane_ys.push_back(lane_y);
    }
  }

  if (!xs.empty() && out.polyline.size() >= 2) {
    for (auto& pt : out.polyline) {
      const double y_lane = interpY(pt.x, xs, lane_ys);
      pt.y = d_prob * y_lane + (1.0 - d_prob) * pt.y;
    }
    return out;
  }

  // No plan: fall back to lane mid / single-side path on ll.x grid.
  if (!xs.empty()) {
    out.polyline.clear();
    out.polyline.reserve(xs.size());
    for (size_t i = 0; i < xs.size(); ++i) {
      if (xs[i] >= 1.0)
        out.polyline.push_back({xs[i], lane_ys[i]});
    }
  }
  return out;
}

ChassisSample carStateToChassis(const ai::flow::adas::CarState& cs, double steer_ratio)
{
  ChassisSample s;
  s.timestamp_us = cs.timestamp() * 1000;
  s.speed_mps = cs.v_ego();
  s.yaw_rate = cs.yaw_rate();
  s.steering_angle_deg = cs.steering_angle_deg();
  s.steering_pressed = cs.steering_pressed();
  const double ratio = std::max(steer_ratio, 1e-3);
  s.steer_rad = (s.steering_angle_deg * M_PI / 180.0) / ratio;
  return s;
}

RawImuSample imuToRaw(const ai::flow::adas::IMUData& imu)
{
  RawImuSample s;
  s.timestamp_us = imu.timestamp() * 1000;
  s.ax = imu.accel_x();
  s.ay = imu.accel_y();
  s.az = imu.accel_z();
  s.gx = imu.gyro_x();
  s.gy = imu.gyro_y();
  s.gz = imu.gyro_z();
  s.valid = true;
  return s;
}

}  // namespace adas
