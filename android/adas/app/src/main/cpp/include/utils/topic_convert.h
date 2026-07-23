#pragma once

#include "messages.pb.h"
#include "utils/adas_topics.h"

namespace adas {

LanePathMsg laneLinesToPath(const ai::flow::adas::LaneLines& ll, float min_lane_prob = 0.3f);

ChassisSample carStateToChassis(const ai::flow::adas::CarState& cs, double steer_ratio = 15.7);

RawImuSample imuToRaw(const ai::flow::adas::IMUData& imu);

CameraOdometrySample cameraOdometryToSample(const ai::flow::adas::CameraOdometry& odom);

}  // namespace adas
