#pragma once

#include <cmath>

#include <Eigen/Core>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace adas {

inline double normalizeAngle(double angle)
{
  while (angle > M_PI)
    angle -= 2.0 * M_PI;
  while (angle < -M_PI)
    angle += 2.0 * M_PI;
  return angle;
}

/** GPS bearing (deg, 0=N clockwise) → ENU yaw (rad, 0=+x east, CCW). */
inline double yawEnuFromBearingDeg(double bearing_deg)
{
  return normalizeAngle(M_PI / 2.0 - bearing_deg * (M_PI / 180.0));
}

using Vec2 = Eigen::Vector2d;
using Vec3 = Eigen::Vector3d;
using Mat3 = Eigen::Matrix3d;

}  // namespace adas
