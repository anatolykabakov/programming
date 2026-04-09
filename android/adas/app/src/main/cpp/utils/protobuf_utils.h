#pragma once

#include <vector>
#include <chrono>
#include "logger.h"
#include "messages.pb.h"
#include "can.pb.h"
#include "panda.pb.h"
#include "panda/health.h"
#include "panda/can.h"
#include "panda/can_frame.h"

namespace utils {
// Get current timestamp in milliseconds
int64_t getCurrentTimestamp();

ai::flow::android::ZMQMessage createCANMessage(const std::vector<can_frame>& frames);
ai::flow::android::ZMQMessage createHealthMessage(const health_t& health);
} // namespace utils
