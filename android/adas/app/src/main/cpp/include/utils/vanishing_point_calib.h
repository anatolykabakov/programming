#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "utils/math_utils.h"

namespace adas {

/** Line v = m*u + c in the image. */
struct ImageLine {
  double m = 0.0;
  double c = 0.0;
};

std::optional<std::pair<double, double>> getIntersection(const ImageLine& a, const ImageLine& b);

/** pitch, yaw (radians), roll assumed 0. Pitch sign matches AAD (negative = looking down). */
std::pair<double, double> getPitchYawFromVp(double u_i, double v_i, double fx, double fy, double cx, double cy);

std::optional<ImageLine> fitLineVOfU(const std::vector<Vec2>& uv, double mean_residuals_thresh = 25.0);

/**
 * Online AAD vanishing-point calibrator (pitch/yaw only).
 * Accumulate history_len samples, then commit mean estimate.
 */
class VanishingPointCalibrator {
public:
  explicit VanishingPointCalibrator(int history_len = 50, double pitch0_deg = -6.0, double yaw0_deg = 0.0);

  void reset();
  void setEstimate(double pitch_deg, double yaw_deg);

  /** Returns true when a new mean estimate was committed. */
  bool updateFromLines(const ImageLine& left, const ImageLine& right, double fx, double fy, double cx, double cy);

  bool updateFromUv(const std::vector<Vec2>& left_uv, const std::vector<Vec2>& right_uv, double fx, double fy,
                    double cx, double cy);

  double pitchDeg() const { return pitch_deg_; }
  double yawDeg() const { return yaw_deg_; }
  bool success() const { return success_; }
  int nUpdates() const { return n_updates_; }
  bool hasVp() const { return has_vp_; }
  double vpU() const { return vp_u_; }
  double vpV() const { return vp_v_; }
  int historySize() const { return static_cast<int>(history_.size()); }

private:
  bool addToHistory(double pitch_rad, double yaw_rad);

  int history_len_ = 50;
  double pitch_deg_ = -6.0;
  double yaw_deg_ = 0.0;
  bool success_ = false;
  int n_updates_ = 0;
  bool has_vp_ = false;
  double vp_u_ = 0.0;
  double vp_v_ = 0.0;
  std::vector<std::pair<double, double>> history_;
};

}  // namespace adas
