#include "utils/vehicle_ekf.h"

#include <algorithm>

namespace adas {
namespace {

void matMul5(const std::array<double, 25>& A, const std::array<double, 25>& B, std::array<double, 25>& C)
{
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < 5; ++j) {
      double s = 0.0;
      for (int k = 0; k < 5; ++k)
        s += A[i * 5 + k] * B[k * 5 + j];
      C[i * 5 + j] = s;
    }
  }
}

void matTranspose5(const std::array<double, 25>& A, std::array<double, 25>& AT)
{
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      AT[j * 5 + i] = A[i * 5 + j];
}

void matAdd5(std::array<double, 25>& A, const std::array<double, 25>& B)
{
  for (int i = 0; i < 25; ++i)
    A[i] += B[i];
}

bool invert2x2(const double S[4], double Sinv[4])
{
  const double det = S[0] * S[3] - S[1] * S[2];
  if (std::abs(det) < 1e-12)
    return false;
  const double inv = 1.0 / det;
  Sinv[0] = S[3] * inv;
  Sinv[1] = -S[1] * inv;
  Sinv[2] = -S[2] * inv;
  Sinv[3] = S[0] * inv;
  return true;
}

}  // namespace

VehicleEKF::VehicleEKF(double wheelbase_, double gps_noise_pos, double imu_noise_yaw_rate) : wheelbase(wheelbase_)
{
  Q_.fill(0.0);
  at(Q_, 0, 0) = 0.1 * 0.1;
  at(Q_, 1, 1) = 0.1 * 0.1;
  at(Q_, 2, 2) = 0.01 * 0.01;
  at(Q_, 3, 3) = 0.5 * 0.5;
  at(Q_, 4, 4) = 0.05 * 0.05;
  R_gps_ = {gps_noise_pos * gps_noise_pos, 0.0, 0.0, gps_noise_pos * gps_noise_pos};
  R_imu_ = imu_noise_yaw_rate * imu_noise_yaw_rate;
  reset();
}

void VehicleEKF::reset(double x, double y, double yaw, double v, double yaw_rate, double pos_unc, double yaw_unc,
                       double v_unc, double yaw_rate_unc)
{
  state_ = {x, y, yaw, v, yaw_rate};
  P_.fill(0.0);
  at(P_, 0, 0) = pos_unc * pos_unc;
  at(P_, 1, 1) = pos_unc * pos_unc;
  at(P_, 2, 2) = yaw_unc * yaw_unc;
  at(P_, 3, 3) = v_unc * v_unc;
  at(P_, 4, 4) = yaw_rate_unc * yaw_rate_unc;
  prediction_count = gps_update_count = gps_rejected_count = imu_update_count = 0;
}

void VehicleEKF::predict(double v_measured, double steering_angle, double dt)
{
  const double x = state_[0], y = state_[1], yaw = state_[2], v = state_[3];
  double yaw_rate_pred = 0.0;
  if (std::abs(steering_angle) > 0.001 && std::abs(v_measured) > 0.01) {
    yaw_rate_pred = v_measured * std::tan(steering_angle) / wheelbase;
  }
  state_[0] = x + v * std::cos(yaw) * dt;
  state_[1] = y + v * std::sin(yaw) * dt;
  state_[2] = normalizeAngle(yaw + yaw_rate_pred * dt);
  state_[3] = v_measured;
  state_[4] = yaw_rate_pred;

  std::array<double, 25> F{};
  for (int i = 0; i < 5; ++i)
    at(F, i, i) = 1.0;
  at(F, 0, 2) = -v * std::sin(yaw) * dt;
  at(F, 0, 3) = std::cos(yaw) * dt;
  at(F, 1, 2) = v * std::cos(yaw) * dt;
  at(F, 1, 3) = std::sin(yaw) * dt;
  at(F, 2, 4) = dt;

  std::array<double, 25> FT{}, FP{}, FPFT{};
  matTranspose5(F, FT);
  matMul5(F, P_, FP);
  matMul5(FP, FT, FPFT);
  P_ = FPFT;
  matAdd5(P_, Q_);
  ++prediction_count;
}

bool VehicleEKF::updateGps(double gps_x, double gps_y, double max_innovation)
{
  const double innov[2] = {gps_x - state_[0], gps_y - state_[1]};
  const double mag = std::sqrt(innov[0] * innov[0] + innov[1] * innov[1]);
  if (mag > max_innovation) {
    ++gps_rejected_count;
    return false;
  }

  // S = H P H^T + R, H selects x,y
  double S[4] = {at(P_, 0, 0) + R_gps_[0], at(P_, 0, 1) + R_gps_[1], at(P_, 1, 0) + R_gps_[2],
                 at(P_, 1, 1) + R_gps_[3]};
  double Sinv[4];
  if (!invert2x2(S, Sinv))
    return false;

  // K (5x2) = P H^T Sinv
  double K[10];
  for (int i = 0; i < 5; ++i) {
    const double p0 = at(P_, i, 0), p1 = at(P_, i, 1);
    K[i * 2 + 0] = p0 * Sinv[0] + p1 * Sinv[2];
    K[i * 2 + 1] = p0 * Sinv[1] + p1 * Sinv[3];
  }

  for (int i = 0; i < 5; ++i)
    state_[i] += K[i * 2] * innov[0] + K[i * 2 + 1] * innov[1];
  state_[2] = normalizeAngle(state_[2]);

  // Joseph form: P = (I-KH) P (I-KH)^T + K R K^T
  std::array<double, 25> IKH{};
  for (int i = 0; i < 5; ++i)
    at(IKH, i, i) = 1.0;
  for (int i = 0; i < 5; ++i) {
    at(IKH, i, 0) -= K[i * 2];
    at(IKH, i, 1) -= K[i * 2 + 1];
  }
  std::array<double, 25> IKHT{}, TMP{}, NEWP{};
  matTranspose5(IKH, IKHT);
  matMul5(IKH, P_, TMP);
  matMul5(TMP, IKHT, NEWP);
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < 5; ++j) {
      NEWP[i * 5 + j] += K[i * 2] * (R_gps_[0] * K[j * 2] + R_gps_[1] * K[j * 2 + 1]) +
                         K[i * 2 + 1] * (R_gps_[2] * K[j * 2] + R_gps_[3] * K[j * 2 + 1]);
    }
  }
  P_ = NEWP;
  ++gps_update_count;
  return true;
}

void VehicleEKF::updateImu(double yaw_rate_imu)
{
  const double innov = yaw_rate_imu - state_[4];
  const double S = at(P_, 4, 4) + R_imu_;
  if (std::abs(S) < 1e-12)
    return;
  const double Sinv = 1.0 / S;
  double K[5];
  for (int i = 0; i < 5; ++i)
    K[i] = at(P_, i, 4) * Sinv;
  for (int i = 0; i < 5; ++i)
    state_[i] += K[i] * innov;
  state_[2] = normalizeAngle(state_[2]);

  std::array<double, 25> IKH{};
  for (int i = 0; i < 5; ++i)
    at(IKH, i, i) = 1.0;
  for (int i = 0; i < 5; ++i)
    at(IKH, i, 4) -= K[i];
  std::array<double, 25> IKHT{}, TMP{}, NEWP{};
  matTranspose5(IKH, IKHT);
  matMul5(IKH, P_, TMP);
  matMul5(TMP, IKHT, NEWP);
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      NEWP[i * 5 + j] += K[i] * R_imu_ * K[j];
  P_ = NEWP;
  ++imu_update_count;
}

}  // namespace adas
