#include "adas_app.h"
#include "utils/logger.h"

// ========== AdasApp Implementation ==========
AdasApp::AdasApp() : running_(false), usb_fd_(-1) {}

AdasApp::AdasApp(int usb_fd, std::string dbc_path) : running_(false), usb_fd_(usb_fd), dbc_path_(std::move(dbc_path)) {}

AdasApp::~AdasApp() { stop(); }

bool AdasApp::start()
{
  if (running_) {
    LOGI("AdasApp already running");
    return true;
  }

  try {
    setupServices();

    size_t started = service_manager_->startAll();
    LOGI("Started %zu services", started);

    running_ = true;

  } catch (const std::exception& e) {
    LOGE("Exception in startWithUsbFd(): %s", e.what());
  }
  return running_;
}

void AdasApp::stop()
{
  if (!running_) {
    return;
  }

  LOGI("Stopping AdasApp...");
  running_ = false;

  if (service_manager_) {
    size_t stopped = service_manager_->stopAll();
    LOGI("Stopped %zu services", stopped);
    service_manager_->printStats();
  }

  LOGI("AdasApp stopped");
}

void AdasApp::setupServices()
{
  LOGI("Setting up services...");

  std::vector<microros::ServicePtr> services;
  if (usb_fd_ != -1) {
    panda_service_ = std::make_shared<PandaService>(usb_fd_, dbc_path_);
    panda_service_->setPriority(microros::Service::Priority::High);
    services.push_back(panda_service_);
  }

  zmq_bridge_service_ = std::make_shared<ZmqBridgeService>();
  zmq_bridge_service_->setPriority(microros::Service::Priority::High);
  services.push_back(zmq_bridge_service_);

  sensor_service_ = std::make_shared<SensorReaderService>();
  sensor_service_->setPriority(microros::Service::Priority::Normal);
  services.push_back(sensor_service_);

  service_manager_ = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                                microros::ServiceManager::ThreadingMode::ThreadPool,
                                                                3  // Use 3 worker threads
  );

  LOGI("Services setup completed");
}
