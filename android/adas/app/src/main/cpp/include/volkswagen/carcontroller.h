#pragma once
// Port of dragonpilot selfdrive/car/volkswagen/carcontroller.py (MQB path, lateral-only).

#include <cstdint>
#include <optional>
#include <vector>

#include "panda/can_frame.h"
#include "volkswagen/mqbcan.h"
#include "volkswagen/values.h"

namespace volkswagen {

struct Actuators {
  // Lateral: normalized [-1,1] like openpilot actuators.steer, scaled by STEER_MAX.
  float steer = 0.f;
  // Optional absolute cNm override (test inject). If set, used instead of steer*STEER_MAX.
  std::optional<int> steerTorqueCNm;
};

struct CarControl {
  bool latActive = false;
  Actuators actuators;
  HudControl hud;
  int visualAlert = 0;  // 0=none; use CarControllerParams::LDW_MSG_*
};

struct CarStateView {
  float vEgo = 0.f;
  bool standstill = false;
  float steeringTorque = 0.f;  // cNm signed (EPS_Lenkmoment)
  bool steeringPressed = false;
  uint8_t epsHcaStatus = 0;
  LdwStockValues ldwStock;
};

class CarController {
public:
  CarController() = default;

  // Call at 100Hz (every 10ms) to match dragonpilot DT_CTRL + STEER_STEP.
  std::vector<can_frame> update(const CarControl& CC, const CarStateView& CS);

  int applySteerLast() const { return apply_steer_last_; }
  int frame() const { return frame_; }

private:
  int apply_driver_steer_torque_limits(int apply_torque, float driver_torque) const;

  int apply_steer_last_ = 0;
  int frame_ = 0;
  int hca_same_torque_count_ = 0;
  int hca_enabled_frame_count_ = 0;
  uint8_t hca_counter_ = 0;
};

}  // namespace volkswagen
