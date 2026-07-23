#pragma once

#include <deque>
#include <string_view>
#include <variant>
#include <vector>

#include "middleware/middleware.hpp"
#include "services/lane_keep_service.h"
#include "utils/adas_topics.h"

namespace adas {

/** Host-facing outputs published by services (type identifies the source). */
using HostOutMsg = std::variant<LaneKeepOutput, LocalizationPose, CameraCalibrationState>;

class InternalSubscriber : public adas::Service {
public:
  void configure() override;
  void reset() override;
  std::string_view getName() const override { return "internal_sub"; }

  /** Drain queued outputs since last pop (FIFO). */
  std::vector<HostOutMsg> popMessages();

private:
  std::deque<HostOutMsg> out_;
};

}  // namespace adas
