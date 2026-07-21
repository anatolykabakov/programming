#pragma once

#include <array>
#include <cmath>

#include "utils/math_utils.h"

namespace adas {

class VehicleEKF {
public:
  explicit VehicleEKF(double wheelbase = 2.636, double gps_noise_pos = 5.0, double imu_noise_yaw_rate = 0.02);

  void reset(double x = 0, double y = 0, double yaw = 0, double v = 0, double yaw_rate = 0, double pos_unc = 10.0,
             double yaw_unc = 0.5, double v_unc = 2.0, double yaw_rate_unc = 0.1);

  void predict(double v_measured, double steering_angle, double dt);
  bool updateGps(double gps_x, double gps_y, double max_innovation = 50.0);
  void updateImu(double yaw_rate_imu);

  double x() const { return state_[0]; }
  double y() const { return state_[1]; }
  double yaw() const { return state_[2]; }
  double v() const { return state_[3]; }
  double yawRate() const { return state_[4]; }

  int prediction_count = 0;
  int gps_update_count = 0;
  int gps_rejected_count = 0;
  int imu_update_count = 0;

  double wheelbase = 2.636;

private:
  std::array<double, 5> state_{};
  std::array<double, 25> P_{};  // row-major 5x5
  std::array<double, 25> Q_{};
  std::array<double, 4> R_gps_{};  // 2x2
  double R_imu_ = 0.0004;

  static double& at(std::array<double, 25>& M, int r, int c) { return M[r * 5 + c]; }
  static double at(const std::array<double, 25>& M, int r, int c) { return M[r * 5 + c]; }
};

}  // namespace adas
