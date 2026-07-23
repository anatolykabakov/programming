#pragma once

#include <cstdint>

#include "panda/can_frame.h"
#include "volkswagen/values.h"

namespace volkswagen {

struct HudControl {
  bool leftLaneVisible = false;
  bool rightLaneVisible = false;
  bool leftLaneDepart = false;
  bool rightLaneDepart = false;
};

struct LdwStockValues {
  uint8_t data[8]{};
  bool valid = false;
};

can_frame create_steering_control(int bus, int apply_steer, bool lkas_enabled, uint8_t* counter);

can_frame create_lka_hud_control(int bus, const LdwStockValues& ldw_stock, bool enabled, bool steering_pressed,
                                 int hud_alert, const HudControl& hud);

}  // namespace volkswagen
