#pragma once

#include <memory>
#include <vector>
#include "framework/service_manager.hpp"
#include "panda/panda.h"
#include "utils/protobuf_utils.h"

class PandaService : public microros::Service {
public:
  PandaService(int usb_fd) : usb_fd_(usb_fd) {}
  ~PandaService();

  void configure() override;
  void reset() override;

private:
  void pandaRxCallback();
  void pandaStateCallback();
  void canTxCallback(const ai::flow::android::ZMQMessage& msg);

  void initializePanda();

  int usb_fd_;
  std::shared_ptr<Panda> panda_;

  bool engaged_ = false;
  bool engaged_mads_ = false;
  bool initialized_ = false;
  bool safety_configured_ = false;
};
