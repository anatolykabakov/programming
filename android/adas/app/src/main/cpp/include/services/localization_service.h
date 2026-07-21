#pragma once

#include <optional>
#include <tuple>

#include "framework/service_manager.hpp"
#include "utils/adas_topics.h"
#include "utils/math_utils.h"
#include "utils/online_localizer.h"

namespace adas {

/**
 * Online EKF localization.
 * Topics: sub vehicle/chassis + gps + sensors/imu_yaw (from ImuCalibService) → pub localization/pose.
 * Standalone step() / resetPose() for Python.
 */
class LocalizationService : public microros::Service {
public:
  LocalizationService(double wheelbase = 2.636, double gps_noise_pos = 0.5, double gps_update_interval = 0.2);

  void configure() override;
  void reset() override;

  void resetPose(double x, double y, double yaw, double v = 0, double yaw_rate = 0);

  std::tuple<double, double, double> step(double dt, double speed_mps, double steer_rad,
                                          std::optional<double> yaw_rate = std::nullopt,
                                          std::optional<double> gps_x = std::nullopt,
                                          std::optional<double> gps_y = std::nullopt,
                                          std::optional<double> ref_x = std::nullopt,
                                          std::optional<double> ref_y = std::nullopt);

  OnlineLocalizer& localizer() { return loc_; }
  const OnlineLocalizer& localizer() const { return loc_; }
  const LocalizationPose& lastPose() const { return last_pose_; }

private:
  void onChassis(const ChassisSample& msg);
  void onGps(const GpsSample& msg);
  void onImu(const ImuSample& msg);
  void publishPose(int64_t timestamp_us);

  OnlineLocalizer loc_;
  ChassisSample chassis_;
  GpsSample gps_;
  ImuSample imu_;
  bool have_chassis_ = false;
  int64_t last_t_us_ = 0;
  LocalizationPose last_pose_;
};

}  // namespace adas
