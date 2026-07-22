#pragma once

#include <cmath>
#include <optional>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "utils/adas_topics.h"

namespace adas {

/** Equirectangular lat/lon → local meters (x=East, y=North), origin locked on first fix. */
class GpsLocalProjector {
public:
  void reset()
  {
    have_origin_ = false;
    lat0_deg_ = lon0_deg_ = 0;
  }

  GpsSample project(int64_t timestamp_us, double lat_deg, double lon_deg, bool valid_fix = true)
  {
    GpsSample s;
    s.timestamp_us = timestamp_us;
    if (!valid_fix || !std::isfinite(lat_deg) || !std::isfinite(lon_deg)) {
      s.valid = false;
      return s;
    }
    if (!have_origin_) {
      lat0_deg_ = lat_deg;
      lon0_deg_ = lon_deg;
      have_origin_ = true;
    }
    constexpr double kR = 6371000.0;
    const double dlat = (lat_deg - lat0_deg_) * (M_PI / 180.0);
    const double dlon = (lon_deg - lon0_deg_) * (M_PI / 180.0);
    const double lat0 = lat0_deg_ * (M_PI / 180.0);
    s.y = dlat * kR;                   // North
    s.x = dlon * std::cos(lat0) * kR;  // East
    s.valid = true;
    return s;
  }

  bool haveOrigin() const { return have_origin_; }
  double lat0() const { return lat0_deg_; }
  double lon0() const { return lon0_deg_; }

private:
  bool have_origin_ = false;
  double lat0_deg_ = 0;
  double lon0_deg_ = 0;
};

/** Feature flags + vehicle/camera priors (assets/config.json). */
struct AdasRuntimeConfig {
  bool panda = true;
  bool zmq_bridge = true;
  bool lane_keep = false;
  bool localization = false;
  bool camera_calib = false;
  bool imu_calib = true;  // used when localization is on

  double wheelbase_m = 2.636;
  double steer_ratio = 15.7;
  /** Road-wheel angle that maps to full ±max_torque_cnm (smaller → stronger assist). */
  double max_steer_deg = 8.0;
  /** Cap for controls/steer (VW HCA / panda clamp at 300). */
  double max_torque_cnm = 300.0;
  /** Flowpilot VW LatControlPID defaults (angle error [deg] → torque [-1,1]). */
  double lat_pid_kp = 0.6;
  double lat_pid_ki = 0.2;
  double lat_pid_kf = 0.00006;
  double pitch0_deg = 0.76;
  double yaw0_deg = 1.56;
  double roll0_deg = 0.0;
  double camera_height_m = 0.7;
  double fx = 930.0;
  double fy = 930.0;
  double cx = 640.0;
  double cy = 360.0;

  /** Native ZMQ bind endpoints (must match Java AdasConfig zmq.*). */
  std::string zmq_endpoint_in = "tcp://127.0.0.1:5555";
  std::string zmq_endpoint_out = "tcp://127.0.0.1:5556";
};

/**
 * Load runtime config from assets/config.json schema.
 * Missing keys keep defaults. On I/O/parse failure returns defaults and sets *ok=false.
 */
AdasRuntimeConfig loadAdasRuntimeConfig(const std::string& path, bool* ok = nullptr);

}  // namespace adas
