#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "utils/adas_topics.h"

namespace adas {

/**
 * Live extrinsic calibrator — flowpilot/openpilot calibrationd.
 * Input: model pose (cameraOdometry) + optional v_ego. No image CV.
 */
class PoseCalibrator {
public:
  enum Status { Uncalibrated = 0, Calibrated = 1, Invalid = 2, Recalibrating = 3 };

  explicit PoseCalibrator(double pitch0_deg = 0.0, double yaw0_deg = 0.0, double height_m = 1.22);

  void reset(double pitch0_deg, double yaw0_deg, int valid_blocks = 0);
  void setVEgo(double v_ego_mps);
  void setHeight(double height_m) { height_m_ = height_m; }

  bool handleCamOdom(const CameraOdometrySample& odom);

  double pitchDeg() const;
  double yawDeg() const;
  double rollDeg() const { return 0.0; }
  double heightM() const { return height_m_; }
  Status status() const { return status_; }
  int validBlocks() const { return valid_blocks_; }
  int calPercent() const;
  bool calibrated() const { return status_ == Calibrated; }

  std::array<double, 3> smoothRpy() const;

private:
  void updateStatus();
  std::vector<int> validIdxs() const;

  static constexpr double kMinSpeed = 15.0 * 0.44704;
  static constexpr double kMaxVelAngleStd = 0.25 * M_PI / 180.0;
  static constexpr double kMaxYawRate = 2.0 * M_PI / 180.0;
  static constexpr int kSmoothCycles = 10;
  static constexpr int kBlockSize = 100;
  static constexpr int kInputsNeeded = 5;
  static constexpr int kInputsWanted = 50;
  static constexpr double kMaxSpread = 2.0 * M_PI / 180.0;

  double height_m_ = 1.22;
  double v_ego_ = 0.0;
  std::array<double, 3> rpy_{{0, 0, 0}};
  std::array<std::array<double, 3>, kInputsWanted> rpys_{};
  std::array<double, 3> old_rpy_{{0, 0, 0}};
  std::array<double, 3> calib_spread_{{0, 0, 0}};
  double old_rpy_weight_ = 0.0;
  int valid_blocks_ = 0;
  int idx_ = 0;
  int block_idx_ = 0;
  Status status_ = Uncalibrated;
};

}  // namespace adas
