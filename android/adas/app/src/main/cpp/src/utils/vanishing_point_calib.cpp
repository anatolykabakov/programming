#include "utils/vanishing_point_calib.h"

#include <algorithm>
#include <cmath>

namespace adas {

std::optional<std::pair<double, double>> getIntersection(const ImageLine& a, const ImageLine& b)
{
  if (std::abs(a.m - b.m) < 1e-9)
    return std::nullopt;
  const double u = (b.c - a.c) / (a.m - b.m);
  const double v = a.m * u + a.c;
  return std::make_pair(u, v);
}

std::pair<double, double> getPitchYawFromVp(double u_i, double v_i, double fx, double fy, double cx, double cy)
{
  // r3 = K^{-1} [u,v,1], then yaw = -atan2(r3x,r3z), pitch = asin(r3y)
  const double x = (u_i - cx) / fx;
  const double y = (v_i - cy) / fy;
  const double z = 1.0;
  const double n = std::sqrt(x * x + y * y + z * z);
  const double r3x = x / n;
  const double r3y = y / n;
  const double r3z = z / n;
  const double yaw = -std::atan2(r3x, r3z);
  const double pitch = std::asin(std::clamp(r3y, -1.0, 1.0));
  return {pitch, yaw};
}

std::optional<ImageLine> fitLineVOfU(const std::vector<Vec2>& uv, double mean_residuals_thresh)
{
  std::vector<double> us, vs;
  us.reserve(uv.size());
  vs.reserve(uv.size());
  for (const auto& p : uv) {
    if (std::isfinite(p.x) && std::isfinite(p.y)) {
      us.push_back(p.x);
      vs.push_back(p.y);
    }
  }
  if (us.size() < 8)
    return std::nullopt;

  const double n = static_cast<double>(us.size());
  double sum_u = 0, sum_v = 0, sum_uu = 0, sum_uv = 0;
  for (size_t i = 0; i < us.size(); ++i) {
    sum_u += us[i];
    sum_v += vs[i];
    sum_uu += us[i] * us[i];
    sum_uv += us[i] * vs[i];
  }
  const double denom = n * sum_uu - sum_u * sum_u;
  if (std::abs(denom) < 1e-12)
    return std::nullopt;
  const double m = (n * sum_uv - sum_u * sum_v) / denom;
  const double c = (sum_v - m * sum_u) / n;

  double rss = 0;
  for (size_t i = 0; i < us.size(); ++i) {
    const double e = vs[i] - (m * us[i] + c);
    rss += e * e;
  }
  if (rss / n > mean_residuals_thresh)
    return std::nullopt;
  return ImageLine{m, c};
}

VanishingPointCalibrator::VanishingPointCalibrator(int history_len, double pitch0_deg, double yaw0_deg)
  : history_len_(history_len), pitch_deg_(pitch0_deg), yaw_deg_(yaw0_deg)
{
}

void VanishingPointCalibrator::reset()
{
  history_.clear();
  success_ = false;
  has_vp_ = false;
  n_updates_ = 0;
}

void VanishingPointCalibrator::setEstimate(double pitch_deg, double yaw_deg)
{
  pitch_deg_ = pitch_deg;
  yaw_deg_ = yaw_deg;
}

bool VanishingPointCalibrator::addToHistory(double pitch_rad, double yaw_rad)
{
  history_.emplace_back(pitch_rad, yaw_rad);
  if (static_cast<int>(history_.size()) <= history_len_)
    return false;
  double sp = 0, sy = 0;
  for (const auto& py : history_) {
    sp += py.first;
    sy += py.second;
  }
  const double inv = 1.0 / static_cast<double>(history_.size());
  pitch_deg_ = sp * inv * 180.0 / M_PI;
  yaw_deg_ = sy * inv * 180.0 / M_PI;
  success_ = true;
  ++n_updates_;
  history_.clear();
  return true;
}

bool VanishingPointCalibrator::updateFromLines(const ImageLine& left, const ImageLine& right, double fx, double fy,
                                               double cx, double cy)
{
  auto vp = getIntersection(left, right);
  if (!vp)
    return false;
  const double u_i = vp->first;
  const double v_i = vp->second;
  if (!(-0.5 * cx <= u_i && u_i <= 2.5 * cx && -0.5 * cy <= v_i && v_i <= 2.5 * cy)) {
    return false;
  }
  auto [pitch, yaw] = getPitchYawFromVp(u_i, v_i, fx, fy, cx, cy);
  if (std::abs(pitch * 180.0 / M_PI) > 25.0 || std::abs(yaw * 180.0 / M_PI) > 15.0) {
    return false;
  }
  if (v_i > cy + 0.05 * std::max(fy, 1.0))
    return false;
  if (std::abs(v_i - cy) > 0.45 * std::max(fy, 1.0))
    return false;

  has_vp_ = true;
  vp_u_ = u_i;
  vp_v_ = v_i;
  return addToHistory(pitch, yaw);
}

bool VanishingPointCalibrator::updateFromUv(const std::vector<Vec2>& left_uv, const std::vector<Vec2>& right_uv,
                                            double fx, double fy, double cx, double cy)
{
  auto left = fitLineVOfU(left_uv);
  auto right = fitLineVOfU(right_uv);
  if (!left || !right)
    return false;
  if (left->m * right->m >= 0.0)
    return false;  // not converging
  return updateFromLines(*left, *right, fx, fy, cx, cy);
}

}  // namespace adas
