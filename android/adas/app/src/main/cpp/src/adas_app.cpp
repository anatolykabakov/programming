#include "adas_app.h"

#include "utils/logger.h"

AdasApp::AdasApp() : mode_(Mode::RealTime), running_(false) {}

AdasApp::AdasApp(Config cfg) : mode_(Mode::RealTime), running_(false), cfg_(std::move(cfg)) {}

AdasApp::AdasApp(int usb_fd) : AdasApp(usb_fd, {}, Config{}) {}

AdasApp::AdasApp(int usb_fd, std::string dbc_path) : AdasApp(usb_fd, std::move(dbc_path), Config{}) {}

AdasApp::AdasApp(int usb_fd, std::string dbc_path, Config cfg)
  : mode_(Mode::RealTime), running_(false), cfg_(std::move(cfg))
{
  cfg_.panda.usb_fd = usb_fd;
  cfg_.panda.dbc_path = std::move(dbc_path);
}

AdasApp::AdasApp(Mode mode, double wheelbase, double pitch0_deg, double yaw0_deg, double camera_height,
                 int camera_calib_history_len, double gps_noise_pos, double gps_update_interval)
  : mode_(mode), running_(false), cfg_(Config::forSimulated(wheelbase, pitch0_deg, yaw0_deg, camera_height))
{
  cfg_.camera_calib.history_len = camera_calib_history_len;
  cfg_.localization.gps_noise_pos = gps_noise_pos;
  cfg_.localization.gps_update_interval = gps_update_interval;
  if (mode_ != Mode::Simulated)
    return;
  setupSimulatedServices();
}

AdasApp::~AdasApp() { stop(); }

bool AdasApp::start()
{
  if (running_) {
    LOGI("AdasApp already running");
    return true;
  }

  try {
    if (mode_ == Mode::RealTime) {
      setupRealtimeServices();
      size_t started = middleware_->startAll();
      LOGI("Started %zu realtime services", started);
    } else {
      LOGI("AdasApp Simulated ready (%zu services)", middleware_->getServiceCount());
    }
    running_ = true;
  } catch (const std::exception& e) {
    LOGE("Exception in AdasApp::start(): %s", e.what());
  }
  return running_;
}

void AdasApp::stop()
{
  if (!running_)
    return;

  LOGI("Stopping AdasApp...");
  running_ = false;

  if (middleware_ && mode_ == Mode::RealTime) {
    size_t stopped = middleware_->stopAll();
    LOGI("Stopped %zu services", stopped);
    middleware_->printStats();
  }

  LOGI("AdasApp stopped");
}

void AdasApp::setupRealtimeServices()
{
  const auto& f = cfg_.feature_flags;

  LOGI("Setting up realtime services (lane_keep=%d localization=%d camera_calib=%d)...", f.enable_lane_keep ? 1 : 0,
       f.enable_localization ? 1 : 0, f.enable_camera_calib ? 1 : 0);

  middleware_ = std::make_shared<adas::Middleware>(adas::Middleware::Mode::RealTime);

  if (cfg_.panda.usb_fd != -1 && f.enable_panda)
    panda_service_ = middleware_->registerService<PandaService>(cfg_.panda);

  if (f.enable_zmq_bridge)
    zmq_bridge_service_ = middleware_->registerService<ZmqBridgeService>(cfg_.zmq_bridge);

  topic_convert_service_ = middleware_->registerService<adas::TopicConvertService>(cfg_.topic_convert);

  if (f.enable_localization && f.enable_imu_calib)
    imu_calib_service_ = middleware_->registerService<adas::ImuCalibService>(cfg_.imu_calib);

  if (f.enable_lane_keep) {
    auto lk = cfg_.lane_keep;
    lk.steer_output_enabled = true;
    lane_keep_service_ = middleware_->registerService<adas::LaneKeepService>(lk);
    LOGI("LaneKeepService max_steer=%.1f° ratio=%.1f max_tq=%.0f pp=%.2f/[%.1f,%.1f] pid=%.2f/%.2f/%.5f",
         lk.max_steer_deg, lk.steer_ratio, lk.max_torque_cnm, lk.pp_k_dd, lk.pp_ld_min, lk.pp_ld_max, lk.pid_kp,
         lk.pid_ki, lk.pid_kf);
  }

  if (f.enable_localization)
    localization_service_ = middleware_->registerService<adas::LocalizationService>(cfg_.localization);

  if (f.enable_camera_calib)
    camera_calib_service_ = middleware_->registerService<adas::CameraCalibService>(cfg_.camera_calib);

  middleware_->registerService<adas::MiddlewareStatsService>();

  LOGI("Realtime services setup completed (%zu services)", middleware_->getServiceCount());
}

void AdasApp::setupSimulatedServices()
{
  auto lk = cfg_.lane_keep;
  lk.steer_output_enabled = true;

  middleware_ = std::make_shared<adas::Middleware>(adas::Middleware::Mode::Simulated);
  lane_keep_service_ = middleware_->registerService<adas::LaneKeepService>(lk);
  localization_service_ = middleware_->registerService<adas::LocalizationService>(cfg_.localization);
  camera_calib_service_ = middleware_->registerService<adas::CameraCalibService>(cfg_.camera_calib);
  imu_calib_service_ = middleware_->registerService<adas::ImuCalibService>(cfg_.imu_calib);
  internal_subscriber_ = middleware_->registerService<adas::InternalSubscriber>();
  LOGI("Simulated AdasApp services ready (%zu)", middleware_->getServiceCount());
}

void AdasApp::publishChassis(const adas::ChassisSample& chassis)
{
  if (middleware_)
    middleware_->publish(adas::topics::kVehicleChassis, chassis);
}

void AdasApp::publishLanes(const adas::LanePathMsg& lanes)
{
  if (middleware_)
    middleware_->publish(adas::topics::kVisionPath, lanes);
}

void AdasApp::publishGps(const adas::GpsSample& gps)
{
  if (middleware_)
    middleware_->publish(adas::topics::kGpsLocation, gps);
}

void AdasApp::publishImu(const adas::ImuSample& imu)
{
  if (middleware_)
    middleware_->publish(adas::topics::kImuYaw, imu);
}

void AdasApp::publishLaneUv(const adas::LaneUvMsg& uv)
{
  if (middleware_)
    middleware_->publish(adas::topics::kCalibLaneUv, uv);
}

void AdasApp::resetLocalization(double x, double y, double yaw, double v, double yaw_rate)
{
  if (localization_service_)
    localization_service_->resetPose(x, y, yaw, v, yaw_rate);
}

void AdasApp::setCameraIntrinsics(double fx, double fy, double cx, double cy)
{
  if (camera_calib_service_)
    camera_calib_service_->setIntrinsics(fx, fy, cx, cy);
}

void AdasApp::setCameraEstimate(double pitch_deg, double yaw_deg)
{
  if (camera_calib_service_)
    camera_calib_service_->setEstimate(pitch_deg, yaw_deg);
}

void AdasApp::setCameraHeight(double height_m)
{
  if (camera_calib_service_)
    camera_calib_service_->setHeight(height_m);
}

void AdasApp::setLaneKeepPp(double k_dd, double ld_min, double ld_max, double shift)
{
  if (lane_keep_service_)
    lane_keep_service_->setPurePursuit(k_dd, ld_min, ld_max, shift);
}

void AdasApp::setLaneKeepMaxSteerDeg(double max_steer_deg)
{
  if (lane_keep_service_)
    lane_keep_service_->setMaxSteerDeg(max_steer_deg);
}

void AdasApp::setLaneKeepSteerRatio(double ratio)
{
  if (lane_keep_service_)
    lane_keep_service_->setSteerRatio(ratio);
}

void AdasApp::setLaneKeepSteerSign(double sign)
{
  if (lane_keep_service_)
    lane_keep_service_->setSteerSign(sign);
}

void AdasApp::step(uint64_t timestamp_us)
{
  if (!middleware_ || mode_ != Mode::Simulated)
    return;
  middleware_->setTime(timestamp_us);
  middleware_->step();
}

std::vector<adas::HostOutMsg> AdasApp::popMessages()
{
  if (!internal_subscriber_)
    return {};
  return internal_subscriber_->popMessages();
}

void AdasApp::resetCameraCalib()
{
  if (camera_calib_service_)
    camera_calib_service_->reset();
}
