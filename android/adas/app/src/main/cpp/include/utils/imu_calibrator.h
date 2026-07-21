#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include "utils/adas_topics.h"

namespace adas {

/**
 * Online phone→vehicle IMU calibration.
 *
 * - Cold-start: mount prior R (pitch/yaw) → publish yaw immediately.
 * - Quiet standstill: lock R once from gravity; then only EMA bias.
 * - Moving / noisy: apply last transform only (no buffer updates).
 */
class ImuCalibrator {
public:
  explicit ImuCalibrator(double speed_threshold_mps = 0.5 / 3.6, int min_samples = 50, int max_buffer = 400,
                         bool invert_yaw_rate = true);

  void reset();
  void setSpeed(double speed_mps);

  /** Mount prior so yaw is published before standstill calib (degrees). */
  void setMountPrior(double roll_deg, double pitch_deg, double yaw_deg);

  /** Always returns vehicle yaw_rate once prior or calib is set. */
  std::optional<double> push(const RawImuSample& raw);

  bool hasPrior() const { return has_prior_; }
  bool orientationLocked() const { return orientation_locked_; }
  bool ready() const { return has_prior_ || orientation_locked_; }
  const std::array<double, 3>& bias() const { return bias_; }
  const std::array<double, 9>& rotation() const { return R_; }
  int orient_samples() const { return static_cast<int>(accel_buf_.size()); }
  int bias_samples() const { return static_cast<int>(gyro_buf_.size()); }

  static std::array<double, 9> rotationFromGravity(double ax, double ay, double az);
  static std::array<double, 9> rotationFromMountRpy(double roll_deg, double pitch_deg, double yaw_deg);

private:
  bool isQuiet(const RawImuSample& raw) const;
  void tryLockOrientation();
  void updateBiasEma(double gx, double gy, double gz);
  double apply(double gx, double gy, double gz) const;

  double speed_threshold_mps_;
  int min_samples_;
  int max_buffer_;
  bool invert_yaw_rate_;

  double speed_mps_ = 0.0;
  bool has_prior_ = false;
  bool orientation_locked_ = false;
  std::array<double, 3> bias_{{0, 0, 0}};
  std::array<double, 9> R_{{1, 0, 0, 0, 1, 0, 0, 0, 1}};

  std::vector<std::array<double, 3>> accel_buf_;
  std::vector<std::array<double, 3>> gyro_buf_;

  double gyro_quiet_max_ = 0.08;  // rad/s
  double accel_g_err_max_ = 1.5;  // m/s² vs 9.81
  double bias_ema_alpha_ = 0.02;
};

}  // namespace adas
