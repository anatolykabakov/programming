#include "utils/pure_pursuit.h"

#include <algorithm>
#include <cmath>

namespace adas {
namespace {

std::vector<Vec2> circleLineSegmentIntersection(Vec2 center, double radius, Vec2 pt1, Vec2 pt2, bool full_line)
{
  const double p1x = pt1.x, p1y = pt1.y, p2x = pt2.x, p2y = pt2.y;
  const double cx = center.x, cy = center.y;
  const double x1 = p1x - cx, y1 = p1y - cy;
  const double x2 = p2x - cx, y2 = p2y - cy;
  const double dx = x2 - x1, dy = y2 - y1;
  const double dr = std::sqrt(dx * dx + dy * dy);
  if (dr < 1e-12)
    return {};
  const double big_d = x1 * y2 - x2 * y1;
  const double discriminant = radius * radius * dr * dr - big_d * big_d;
  if (discriminant < 0)
    return {};

  std::vector<Vec2> intersections;
  const int dy_sign = (dy < 0) ? -1 : 1;
  const int signs[2] = {(dy < 0) ? 1 : -1, (dy < 0) ? -1 : 1};
  for (int s = 0; s < 2; ++s) {
    const int sign = signs[s];
    const double xi = cx + (big_d * dy + sign * dy_sign * dx * std::sqrt(discriminant)) / (dr * dr);
    const double yi = cy + (-big_d * dx + sign * std::abs(dy) * std::sqrt(discriminant)) / (dr * dr);
    intersections.push_back({xi, yi});
  }

  if (!full_line) {
    std::vector<Vec2> filtered;
    for (const auto& pt : intersections) {
      const double frac = (std::abs(dx) > std::abs(dy)) ? (pt.x - p1x) / dx : (pt.y - p1y) / dy;
      if (frac >= 0.0 && frac <= 1.0)
        filtered.push_back(pt);
    }
    intersections.swap(filtered);
  }
  if (intersections.size() == 2 && std::abs(discriminant) <= 1e-9) {
    intersections.resize(1);
  }
  return intersections;
}

}  // namespace

double PurePursuitResult::curvature() const { return std::tan(steer_rad) / std::max(wheel_base, 1e-6); }

PurePursuit::PurePursuit(double K_dd_, double wheel_base_, double waypoint_shift_, double ld_min_, double ld_max_)
  : K_dd(K_dd_), wheel_base(wheel_base_), waypoint_shift(waypoint_shift_), ld_min(ld_min_), ld_max(ld_max_)
{
}

std::optional<Vec2> getTargetPoint(double lookahead, const std::vector<Vec2>& polyline)
{
  if (polyline.size() < 2)
    return std::nullopt;
  std::vector<Vec2> intersections;
  for (size_t j = 0; j + 1 < polyline.size(); ++j) {
    auto hits = circleLineSegmentIntersection({0.0, 0.0}, lookahead, polyline[j], polyline[j + 1], false);
    intersections.insert(intersections.end(), hits.begin(), hits.end());
  }
  for (const auto& p : intersections) {
    if (p.x > 0.0)
      return p;
  }
  return std::nullopt;
}

PurePursuitResult PurePursuit::compute(const std::vector<Vec2>& polyline_ego, double speed_mps) const
{
  PurePursuitResult out;
  out.speed_mps = std::max(0.0, speed_mps);
  out.wheel_base = wheel_base;
  out.lookahead_m = std::clamp(K_dd * out.speed_mps, ld_min, ld_max);

  std::vector<Vec2> poly_ra = polyline_ego;
  for (auto& p : poly_ra)
    p.x += waypoint_shift;

  out.target_ra = getTargetPoint(out.lookahead_m, poly_ra);
  if (out.target_ra) {
    out.alpha_rad = std::atan2(out.target_ra->y, out.target_ra->x);
    out.steer_rad = std::atan((2.0 * wheel_base * std::sin(out.alpha_rad)) / std::max(out.lookahead_m, 1e-3));
    out.target_ego = Vec2{out.target_ra->x - waypoint_shift, out.target_ra->y};
  }
  return out;
}

}  // namespace adas
