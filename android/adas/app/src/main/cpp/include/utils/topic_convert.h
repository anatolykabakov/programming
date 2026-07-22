#pragma once

#include "messages.pb.h"
#include "utils/adas_topics.h"

namespace adas {

/**
 * Ego path for lane keep (X fwd, Y left), flowpilot/dp style:
 *   base = best PLAN (plan_x/y); optional blend toward near-lane mid by lane probs.
 * {@code min_lane_prob} zeros a lane's contribution below the threshold.
 */
LanePathMsg laneLinesToPath(const ai::flow::adas::LaneLines& ll, float min_lane_prob = 0.3f);

/** CarState → chassis sample for lane-keep / localization. */
ChassisSample carStateToChassis(const ai::flow::adas::CarState& cs, double steer_ratio = 15.7);

/** IMUData → phone-frame raw sample (for ImuCalibService). */
RawImuSample imuToRaw(const ai::flow::adas::IMUData& imu);

}  // namespace adas
