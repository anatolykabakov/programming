#include "utils/imu_calibrator.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace adas {
namespace {

inline double norm3(double x, double y, double z) { return std::sqrt(x * x + y * y + z * z); }

inline void cross(double ax, double ay, double az, double bx, double by, double bz, double& ox, double& oy, double& oz)
{
  ox = ay * bz - az * by;
  oy = az * bx - ax * bz;
  oz = ax * by - ay * bx;
}

}  // namespace

ImuCalibrator::ImuCalibrator(double speed_threshold_mps, int min_samples, int max_buffer, bool invert_yaw_rate)
  : speed_threshold_mps_(speed_threshold_mps)
  , min_samples_(std::max(8, min_samples))
  , max_buffer_(std::max(min_samples_, max_buffer))
  , invert_yaw_rate_(invert_yaw_rate)
{
}

void ImuCalibrator::reset()
{
  has_prior_ = false;
  orientation_locked_ = false;
  bias_ = {{0, 0, 0}};
  R_ = {{1, 0, 0, 0, 1, 0, 0, 0, 1}};
  accel_buf_.clear();
  gyro_buf_.clear();
  speed_mps_ = 0.0;
}

void ImuCalibrator::setSpeed(double speed_mps) { speed_mps_ = std::max(0.0, speed_mps); }

void ImuCalibrator::setMountPrior(double roll_deg, double pitch_deg, double yaw_deg)
{
  R_ = rotationFromMountRpy(roll_deg, pitch_deg, yaw_deg);
  has_prior_ = true;
}

std::array<double, 9> ImuCalibrator::rotationFromMountRpy(double roll_deg, double pitch_deg, double yaw_deg)
{
  const double r = roll_deg * M_PI / 180.0;
  const double p = pitch_deg * M_PI / 180.0;
  const double y = yaw_deg * M_PI / 180.0;
  const double cr = std::cos(r), sr = std::sin(r);
  const double cp = std::cos(p), sp = std::sin(p);
  const double cy = std::cos(y), sy = std::sin(y);
  // R = Rz(yaw) * Ry(pitch) * Rx(roll)  (phone → vehicle)
  std::array<double, 9> R{};
  R[0] = cy * cp;
  R[1] = cy * sp * sr - sy * cr;
  R[2] = cy * sp * cr + sy * sr;
  R[3] = sy * cp;
  R[4] = sy * sp * sr + cy * cr;
  R[5] = sy * sp * cr - cy * sr;
  R[6] = -sp;
  R[7] = cp * sr;
  R[8] = cp * cr;
  return R;
}

std::array<double, 9> ImuCalibrator::rotationFromGravity(double ax, double ay, double az)
{
  const double n = norm3(ax, ay, az);
  std::array<double, 9> I{{1, 0, 0, 0, 1, 0, 0, 0, 1}};
  if (n < 1e-6)
    return I;

  const double gx = ax / n, gy = ay / n, gz = az / n;
  const double tx = 0.0, ty = 0.0, tz = -1.0;

  double cx, cy, cz;
  cross(gx, gy, gz, tx, ty, tz, cx, cy, cz);
  const double cnorm = norm3(cx, cy, cz);
  const double d = gx * tx + gy * ty + gz * tz;

  if (cnorm < 1e-6) {
    if (d > 0.0)
      return I;
    return {{1, 0, 0, 0, -1, 0, 0, 0, -1}};
  }

  const double ux = cx / cnorm, uy = cy / cnorm, uz = cz / cnorm;
  const double angle = std::atan2(cnorm, d);
  const double s = std::sin(angle);
  const double c = std::cos(angle);
  const double one_c = 1.0 - c;

  std::array<double, 9> R{};
  R[0] = c + ux * ux * one_c;
  R[1] = ux * uy * one_c - uz * s;
  R[2] = ux * uz * one_c + uy * s;
  R[3] = uy * ux * one_c + uz * s;
  R[4] = c + uy * uy * one_c;
  R[5] = uy * uz * one_c - ux * s;
  R[6] = uz * ux * one_c - uy * s;
  R[7] = uz * uy * one_c + ux * s;
  R[8] = c + uz * uz * one_c;
  return R;
}

bool ImuCalibrator::isQuiet(const RawImuSample& raw) const
{
  const double g = norm3(raw.ax, raw.ay, raw.az);
  const double g_err = std::abs(g - 9.81);
  const double w = norm3(raw.gx, raw.gy, raw.gz);
  return g_err <= accel_g_err_max_ && w <= gyro_quiet_max_;
}

void ImuCalibrator::tryLockOrientation()
{
  if (orientation_locked_)
    return;
  if (static_cast<int>(accel_buf_.size()) < min_samples_ || static_cast<int>(gyro_buf_.size()) < min_samples_) {
    return;
  }
  double sax = 0, say = 0, saz = 0;
  for (const auto& a : accel_buf_) {
    sax += a[0];
    say += a[1];
    saz += a[2];
  }
  const double inv = 1.0 / static_cast<double>(accel_buf_.size());
  R_ = rotationFromGravity(sax * inv, say * inv, saz * inv);

  double sgx = 0, sgy = 0, sgz = 0;
  for (const auto& g : gyro_buf_) {
    sgx += g[0];
    sgy += g[1];
    sgz += g[2];
  }
  const double inv_g = 1.0 / static_cast<double>(gyro_buf_.size());
  bias_ = {{sgx * inv_g, sgy * inv_g, sgz * inv_g}};
  orientation_locked_ = true;
  accel_buf_.clear();
  gyro_buf_.clear();
}

void ImuCalibrator::updateBiasEma(double gx, double gy, double gz)
{
  const double a = bias_ema_alpha_;
  bias_[0] = (1.0 - a) * bias_[0] + a * gx;
  bias_[1] = (1.0 - a) * bias_[1] + a * gy;
  bias_[2] = (1.0 - a) * bias_[2] + a * gz;
}

double ImuCalibrator::apply(double gx, double gy, double gz) const
{
  const double cx = gx - bias_[0];
  const double cy = gy - bias_[1];
  const double cz = gz - bias_[2];
  const double vz = R_[6] * cx + R_[7] * cy + R_[8] * cz;
  double yaw = vz;
  if (invert_yaw_rate_)
    yaw = -yaw;
  return yaw;
}

std::optional<double> ImuCalibrator::push(const RawImuSample& raw)
{
  if (!raw.valid)
    return std::nullopt;

  const bool stationary = speed_mps_ < speed_threshold_mps_;
  const bool quiet = isQuiet(raw);

  if (stationary && quiet) {
    if (!orientation_locked_) {
      accel_buf_.push_back({{raw.ax, raw.ay, raw.az}});
      gyro_buf_.push_back({{raw.gx, raw.gy, raw.gz}});
      while (static_cast<int>(accel_buf_.size()) > max_buffer_)
        accel_buf_.erase(accel_buf_.begin());
      while (static_cast<int>(gyro_buf_.size()) > max_buffer_)
        gyro_buf_.erase(gyro_buf_.begin());
      tryLockOrientation();
    } else {
      updateBiasEma(raw.gx, raw.gy, raw.gz);
    }
  }

  if (!has_prior_ && !orientation_locked_)
    return std::nullopt;
  return apply(raw.gx, raw.gy, raw.gz);
}

}  // namespace adas
