#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "middleware/middleware.hpp"
#include "messages.pb.h"
#include "panda/panda.h"
#include "utils/can_parser.h"
#include "volkswagen/carcontroller.h"
#include "volkswagen/mqb_car_state_decoder.h"
#include "volkswagen/panda_safety_supervisor.h"

class PandaService : public adas::Service {
public:
  struct Config {
    int usb_fd = -1;
    std::string dbc_path;
  };

  explicit PandaService(Config config);
  ~PandaService();

  void configure() override;
  void reset() override;
  std::string_view getName() const override { return "panda"; }
  const Config& config() const { return config_; }

private:
  void pandaRxCallback();
  void pandaStateCallback();
  void carControllerCallback();
  void steerCommandCallback(const ai::flow::adas::ZMQMessage& msg);
  void initializePanda();
  void publishCarState();

  Config config_;
  int usb_fd_;
  std::string dbc_path_;
  std::shared_ptr<Panda> panda_;
  std::unique_ptr<DBSParser> dbc_;

  volkswagen::MqbCarStateDecoder decoder_;
  volkswagen::PandaSafetySupervisor safety_;
  volkswagen::CarController car_controller_;

  int hca_cmd_steer_ = 0;
  bool hca_cmd_enabled_ = false;
  int64_t hca_cmd_ts_ms_ = 0;
};
