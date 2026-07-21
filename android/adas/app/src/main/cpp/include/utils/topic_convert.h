#pragma once

#include "messages.pb.h"
#include "utils/adas_topics.h"

namespace adas {

/** Build ego centerline (X fwd, Y left) from supercombo LaneLines proto. */
LanePathMsg laneLinesToPath(const ai::flow::adas::LaneLines& ll, float min_lane_prob = 0.3f);

/** CarState → chassis sample for lane-keep / localization. */
ChassisSample carStateToChassis(const ai::flow::adas::CarState& cs, double steer_ratio = 15.7);

/** IMUData → phone-frame raw sample (for ImuCalibService). */
RawImuSample imuToRaw(const ai::flow::adas::IMUData& imu);

}  // namespace adas
