#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "framework/service_manager.hpp"
#include "panda/panda.h"
#include "utils/can_parser.h"
#include "utils/protobuf_utils.h"
#include "car_state.pb.h"
#include "volkswagen/carcontroller.h"

class PandaService : public microros::Service {
public:
  explicit PandaService(int usb_fd, std::string dbc_path = {});
  ~PandaService();

  void configure() override;
  void reset() override;

private:
  void pandaRxCallback();
  void pandaStateCallback();
  void carControllerCallback();
  void steerCommandCallback(const ai::flow::adas::ZMQMessage& msg);

  void initializePanda();
  void updateCarStateFromFrame(const can_frame& frame);
  void publishCarState();
  volkswagen::CarStateView buildCarStateView() const;

  int usb_fd_;
  std::string dbc_path_;
  std::shared_ptr<Panda> panda_;
  std::unique_ptr<DBSParser> dbc_;

  ai::flow::adas::CarState car_state_;
  bool car_state_dirty_ = false;
  double prev_v_ego_ = 0.0;
  int64_t prev_v_ts_ms_ = 0;
  bool brake_esp_ = false;
  bool brake_motor_ = false;

  bool initialized_ = false;
  bool safety_configured_ = false;

  // Injected lateral command from controls/steer (test_steer_tx / planner).
  int hca_cmd_steer_ = 0;
  bool hca_cmd_enabled_ = false;
  int64_t hca_cmd_ts_ms_ = 0;

  uint8_t eps_hca_status_ = 0;
  uint16_t last_safety_mode_ = 19;
  bool last_controls_allowed_ = false;
  bool last_ignition_ = false;

  // Ignition sticky: avoid NOOUTPUT↔VW thrash on voltage noise (wipes controls_allowed).
  bool ignition_sticky_ = false;
  int64_t ignition_low_since_ms_ = 0;
  bool alt_exp_configured_ = false;
  int last_tsk_status_ = -1;

  volkswagen::CarController car_controller_;
  volkswagen::LdwStockValues ldw_stock_;
};
