#pragma once

#include <cstdint>
#include <string>

// Lightweight CAN frame structure
// Used by log_parser and can_parser without requiring panda library
struct can_frame {
  long address;
  std::string dat;
  long busTime;
  long src;
};
