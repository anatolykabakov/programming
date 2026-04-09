#pragma once

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <thread>
#include <memory>
#include "framework/service_manager.hpp"
#include "services/panda_service.h"
#include "services/sensor_reader_service.h"
#include "services/zmq_bridge_service.h"

class AdasApp {
public:
  AdasApp();
  AdasApp(int usb_fd);
  ~AdasApp();

  bool start();
  void stop();

  std::shared_ptr<microros::ServiceManager> getServiceManager() { return service_manager_; }

private:
  void setupServices();

  std::atomic<bool> running_;
  int usb_fd_ = -1;

  std::shared_ptr<microros::ServiceManager> service_manager_;
  std::shared_ptr<PandaService> panda_service_;
  std::shared_ptr<SensorReaderService> sensor_service_;
  std::shared_ptr<ZmqBridgeService> zmq_bridge_service_;
};
