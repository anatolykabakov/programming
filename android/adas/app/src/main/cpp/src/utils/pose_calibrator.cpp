#include "utils/pose_calibrator.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace adas {
namespace {

constexpr double kPitchLo = -0.15;  // ~-8.6° — phone-on-windshield mounts look further down
constexpr double kPitchHi = 0.17;
constexpr double kYawLo = -0.06912048084718224;
constexpr double kYawHi = 0.06912048084718235;

bool finite3(const std::array<double, 3>& a)
{
  return std::isfinite(a[0]) && std::isfinite(a[1]) && std::isfinite(a[2]);
}

std::array<double, 3> sanityClip(std::array<double, 3> rpy)
{
  if (!finite3(rpy))
    return {{0, 0, 0}};
  rpy[1] = std::clamp(rpy[1], kPitchLo - 0.005, kPitchHi + 0.005);
  rpy[2] = std::clamp(rpy[2], kYawLo - 0.005, kYawHi + 0.005);
  return rpy;
}

bool isValid(const std::array<double, 3>& rpy)
{
  return kPitchLo < rpy[1] && rpy[1] < kPitchHi && kYawLo < rpy[2] && rpy[2] < kYawHi;
}

using Mat3 = std::array<std::array<double, 3>, 3>;

Mat3 rotFromEuler(const std::array<double, 3>& eu)
{
  const double r = eu[0], p = eu[1], y = eu[2];
  const double cr = std::cos(r), sr = std::sin(r);
  const double cp = std::cos(p), sp = std::sin(p);
  const double cy = std::cos(y), sy = std::sin(y);
  return {{{{cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr}},
           {{sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr}},
           {{-sp, cp * sr, cp * cr}}}};
}

std::array<double, 3> eulerFromRot(const Mat3& R)
{
  const double pitch = std::asin(std::clamp(-R[2][0], -1.0, 1.0));
  double roll = 0.0, yaw = 0.0;
  if (std::abs(std::cos(pitch)) > 1e-6) {
    roll = std::atan2(R[2][1], R[2][2]);
    yaw = std::atan2(R[1][0], R[0][0]);
  } else {
    yaw = std::atan2(-R[0][1], R[1][1]);
  }
  return {{roll, pitch, yaw}};
}

Mat3 matmul(const Mat3& A, const Mat3& B)
{
  Mat3 C{};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      C[i][j] = A[i][0] * B[0][j] + A[i][1] * B[1][j] + A[i][2] * B[2][j];
  return C;
}

}  // namespace

PoseCalibrator::PoseCalibrator(double pitch0_deg, double yaw0_deg, double height_m) : height_m_(height_m)
{
  reset(pitch0_deg, yaw0_deg, 0);
}

void PoseCalibrator::reset(double pitch0_deg, double yaw0_deg, int valid_blocks)
{
  rpy_ = {{0.0, pitch0_deg * M_PI / 180.0, yaw0_deg * M_PI / 180.0}};
  valid_blocks_ = std::max(0, valid_blocks);
  for (auto& row : rpys_)
    row = rpy_;
  idx_ = 0;
  block_idx_ = 0;
  old_rpy_ = {{0, 0, 0}};
  old_rpy_weight_ = 0.0;
  calib_spread_ = {{0, 0, 0}};
  status_ = Uncalibrated;
  updateStatus();
}

void PoseCalibrator::setVEgo(double v_ego_mps) { v_ego_ = v_ego_mps; }

std::array<double, 3> PoseCalibrator::smoothRpy() const
{
  if (old_rpy_weight_ > 0.0) {
    return {{old_rpy_weight_ * old_rpy_[0] + (1.0 - old_rpy_weight_) * rpy_[0],
             old_rpy_weight_ * old_rpy_[1] + (1.0 - old_rpy_weight_) * rpy_[1],
             old_rpy_weight_ * old_rpy_[2] + (1.0 - old_rpy_weight_) * rpy_[2]}};
  }
  return rpy_;
}

double PoseCalibrator::pitchDeg() const { return smoothRpy()[1] * 180.0 / M_PI; }
double PoseCalibrator::yawDeg() const { return smoothRpy()[2] * 180.0 / M_PI; }

int PoseCalibrator::calPercent() const
{
  return std::min(100, (valid_blocks_ * kBlockSize + idx_) * 100 / (kInputsNeeded * kBlockSize));
}

bool PoseCalibrator::handleCamOdom(const CameraOdometrySample& odom)
{
  if (!odom.valid)
    return false;
  old_rpy_weight_ = std::max(0.0, old_rpy_weight_ - 1.0 / kSmoothCycles);

  const double ego = (v_ego_ > 0.5) ? v_ego_ : odom.trans[0];
  const bool straight_and_fast = ego > kMinSpeed && odom.trans[0] > kMinSpeed && std::abs(odom.rot[2]) < kMaxYawRate;
  const bool rpy_certain = std::atan2(odom.trans_std[1], odom.trans[0]) < kMaxVelAngleStd;
  const bool certain_if_calib = rpy_certain || valid_blocks_ < kInputsNeeded;
  if (!(straight_and_fast && certain_if_calib))
    return false;

  const std::array<double, 3> observed = {
      {0.0, -std::atan2(odom.trans[2], odom.trans[0]), std::atan2(odom.trans[1], odom.trans[0])}};
  auto new_rpy = sanityClip(eulerFromRot(matmul(rotFromEuler(smoothRpy()), rotFromEuler(observed))));

  const double w_idx = static_cast<double>(idx_);
  const double w_new = static_cast<double>(kBlockSize - idx_);
  for (int i = 0; i < 3; ++i) {
    rpys_[block_idx_][i] = (w_idx * rpys_[block_idx_][i] + w_new * new_rpy[i]) / kBlockSize;
  }

  idx_ = (idx_ + 1) % kBlockSize;
  if (idx_ == 0) {
    block_idx_ += 1;
    valid_blocks_ = std::max(block_idx_, valid_blocks_);
    block_idx_ = block_idx_ % kInputsWanted;
  }
  updateStatus();
  return true;
}

std::vector<int> PoseCalibrator::validIdxs() const
{
  std::vector<int> out;
  for (int i = 0; i < block_idx_; ++i)
    out.push_back(i);
  for (int i = std::min(valid_blocks_, block_idx_ + 1); i < valid_blocks_; ++i)
    out.push_back(i);
  return out;
}

void PoseCalibrator::updateStatus()
{
  const auto idxs = validIdxs();
  if (!idxs.empty()) {
    std::array<double, 3> sum{{0, 0, 0}};
    std::array<double, 3> mx{{-1e9, -1e9, -1e9}};
    std::array<double, 3> mn{{1e9, 1e9, 1e9}};
    for (int vi : idxs) {
      for (int i = 0; i < 3; ++i) {
        const double v = rpys_[vi][i];
        sum[i] += v;
        mx[i] = std::max(mx[i], v);
        mn[i] = std::min(mn[i], v);
      }
    }
    const double inv = 1.0 / static_cast<double>(idxs.size());
    for (int i = 0; i < 3; ++i) {
      rpy_[i] = sum[i] * inv;
      calib_spread_[i] = std::abs(mx[i] - mn[i]);
    }
  } else {
    calib_spread_ = {{0, 0, 0}};
  }

  if (valid_blocks_ < kInputsNeeded) {
    if (status_ != Recalibrating)
      status_ = Uncalibrated;
  } else if (isValid(rpy_)) {
    status_ = Calibrated;
  } else {
    status_ = Invalid;
  }

  const double spread = std::max(calib_spread_[0], std::max(calib_spread_[1], calib_spread_[2]));
  if (spread > kMaxSpread && status_ == Calibrated) {
    const int prev = (block_idx_ - 1 + kInputsWanted) % kInputsWanted;
    const auto from = rpy_;
    rpy_ = rpys_[prev];
    valid_blocks_ = 1;
    for (auto& row : rpys_)
      row = rpy_;
    idx_ = 0;
    block_idx_ = 0;
    old_rpy_ = from;
    old_rpy_weight_ = 1.0;
    status_ = Recalibrating;
  }
}

}  // namespace adas
