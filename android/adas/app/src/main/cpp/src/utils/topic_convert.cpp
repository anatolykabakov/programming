#include "utils/topic_convert.h"

#include <algorithm>
#include <cmath>

namespace adas {

LanePathMsg laneLinesToPath(const ai::flow::adas::LaneLines& ll, float min_lane_prob)
{
  LanePathMsg out;
  out.timestamp_us = ll.timestamp() * 1000;
  out.frame_id = ll.frame_id();

  const int nx = ll.x_size();
  // Prefer near lanes: indices 1=leftNear, 2=rightNear (Android ProtoUtils order).
  const bool have_l =
      ll.lanes_size() > 1 && ll.lanes(1).prob() >= min_lane_prob && ll.lanes(1).y_size() == nx && nx >= 2;
  const bool have_r =
      ll.lanes_size() > 2 && ll.lanes(2).prob() >= min_lane_prob && ll.lanes(2).y_size() == nx && nx >= 2;

  if (have_l && have_r) {
    out.polyline.reserve(static_cast<size_t>(nx));
    for (int i = 0; i < nx; ++i) {
      const double x = ll.x(i);
      const double y = 0.5 * (ll.lanes(1).y(i) + ll.lanes(2).y(i));
      if (x >= 1.0)
        out.polyline.push_back({x, y});
    }
    return out;
  }

  // Fallback: best PLAN hypothesis (xy)
  const int np = std::min(ll.plan_x_size(), ll.plan_y_size());
  if (np >= 2) {
    out.polyline.reserve(static_cast<size_t>(np));
    for (int i = 0; i < np; ++i) {
      const double x = ll.plan_x(i);
      if (x >= 1.0)
        out.polyline.push_back({x, ll.plan_y(i)});
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
  const double ratio = std::max(steer_ratio, 1e-3);
  s.steer_rad = (cs.steering_angle_deg() * M_PI / 180.0) / ratio;
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
