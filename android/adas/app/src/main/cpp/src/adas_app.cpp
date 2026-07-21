#include "adas_app.h"

#include "utils/logger.h"

namespace adas {
namespace {

LaneKeepOutput fromProto(const ai::flow::adas::LaneKeepState& p, int64_t t_us)
{
  LaneKeepOutput o;
  o.timestamp_us = t_us;
  o.steer_rad = p.steer_rad();
  o.steer_norm = p.steer_norm();
  o.throttle = p.throttle();
  o.brake = p.brake();
  o.lookahead_m = p.lookahead_m();
  o.target_x = p.target_x();
  o.target_y = p.target_y();
  o.has_target = p.has_target();
  o.curvature = p.curvature();
  o.status = p.status();
  return o;
}

LocalizationPose fromProto(const ai::flow::adas::LocalizationPose& p, int64_t t_us)
{
  LocalizationPose o;
  o.timestamp_us = t_us;
  o.x = p.x();
  o.y = p.y();
  o.yaw = p.yaw();
  o.v = p.v();
  o.yaw_rate = p.yaw_rate();
  o.odom_x = p.odom_x();
  o.odom_y = p.odom_y();
  o.ekf_x = p.ekf_x();
  o.ekf_y = p.ekf_y();
  return o;
}

CameraCalibrationState fromProto(const ai::flow::adas::CameraCalibrationState& p, int64_t t_us)
{
  CameraCalibrationState o;
  o.timestamp_us = t_us;
  o.roll_deg = p.roll_deg();
  o.pitch_deg = p.pitch_deg();
  o.yaw_deg = p.yaw_deg();
  o.camera_height_m = p.camera_height_m();
  o.fx = p.fx();
  o.fy = p.fy();
  o.cx = p.cx();
  o.cy = p.cy();
  o.calibration_success = p.calibration_success();
  o.n_updates = p.n_updates();
  o.vp_u = p.vp_u();
  o.vp_v = p.vp_v();
  o.has_vp = p.has_vp();
  return o;
}

}  // namespace

void InternalSubscriber::configure()
{
  subscribe<ai::flow::adas::ZMQMessage>(topics::kLaneKeep, [this](const ai::flow::adas::ZMQMessage& m) {
    if (!m.has_lane_keep())
      return;
    lane_keep_ = fromProto(m.lane_keep(), m.timestamp() * 1000);
    has_new_lane_keep = true;
  });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kLocalizationPose, [this](const ai::flow::adas::ZMQMessage& m) {
    if (!m.has_localization_pose())
      return;
    pose_ = fromProto(m.localization_pose(), m.timestamp() * 1000);
    has_new_pose = true;
  });
  subscribe<ai::flow::adas::ZMQMessage>(topics::kCameraCalib, [this](const ai::flow::adas::ZMQMessage& m) {
    if (!m.has_camera_calib())
      return;
    camera_calib_ = fromProto(m.camera_calib(), m.timestamp() * 1000);
    has_new_camera_calib = true;
  });
}

void InternalSubscriber::reset()
{
  has_new_lane_keep = false;
  has_new_pose = false;
  has_new_camera_calib = false;
}

}  // namespace adas

// ========== AdasApp ==========
AdasApp::AdasApp() : mode_(Mode::RealTime), running_(false), usb_fd_(-1) {}

AdasApp::AdasApp(int usb_fd, std::string dbc_path, adas::AdasRuntimeConfig cfg)
  : mode_(Mode::RealTime), running_(false), usb_fd_(usb_fd), dbc_path_(std::move(dbc_path)), runtime_cfg_(cfg)
{
}

AdasApp::AdasApp(Mode mode, double wheelbase, double desired_speed, double pitch0_deg, double yaw0_deg,
                 double camera_height)
  : mode_(mode), running_(false), usb_fd_(-1)
{
  runtime_cfg_.wheelbase_m = wheelbase;
  runtime_cfg_.pitch0_deg = pitch0_deg;
  runtime_cfg_.yaw0_deg = yaw0_deg;
  runtime_cfg_.camera_height_m = camera_height;
  runtime_cfg_.lane_keep = true;
  runtime_cfg_.localization = true;
  runtime_cfg_.camera_calib = true;
  runtime_cfg_.imu_calib = true;
  if (mode_ != Mode::Simulated) {
    return;
  }
  setupSimulatedServices(wheelbase, desired_speed, pitch0_deg, yaw0_deg, camera_height);
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
      size_t started = service_manager_->startAll();
      LOGI("Started %zu realtime services", started);
    } else {
      // Simulated: ServiceManager already built in ctor; nothing to startAll().
      LOGI("AdasApp Simulated ready (%zu services)", service_manager_->getServiceCount());
    }
    running_ = true;
  } catch (const std::exception& e) {
    LOGE("Exception in AdasApp::start(): %s", e.what());
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

  if (service_manager_ && mode_ == Mode::RealTime) {
    size_t stopped = service_manager_->stopAll();
    LOGI("Stopped %zu services", stopped);
    service_manager_->printStats();
  }

  LOGI("AdasApp stopped");
}

void AdasApp::setupRealtimeServices()
{
  LOGI("Setting up realtime services (lane_keep=%d localization=%d camera_calib=%d)...", runtime_cfg_.lane_keep ? 1 : 0,
       runtime_cfg_.localization ? 1 : 0, runtime_cfg_.camera_calib ? 1 : 0);

  std::vector<microros::ServicePtr> services;
  if (usb_fd_ != -1 && runtime_cfg_.panda) {
    panda_service_ = std::make_shared<PandaService>(usb_fd_, dbc_path_);
    panda_service_->setPriority(microros::Service::Priority::High);
    services.push_back(panda_service_);
  }

  if (runtime_cfg_.zmq_bridge) {
    zmq_bridge_service_ = std::make_shared<ZmqBridgeService>();
    zmq_bridge_service_->setPriority(microros::Service::Priority::High);
    services.push_back(zmq_bridge_service_);
  }

  topic_convert_service_ = std::make_shared<adas::TopicConvertService>(runtime_cfg_.steer_ratio);
  topic_convert_service_->setPriority(microros::Service::Priority::High);
  services.push_back(topic_convert_service_);

  const bool want_imu = runtime_cfg_.localization && runtime_cfg_.imu_calib;
  if (want_imu) {
    imu_calib_service_ = std::make_shared<adas::ImuCalibService>();
    imu_calib_service_->setMountPrior(runtime_cfg_.roll0_deg, runtime_cfg_.pitch0_deg, runtime_cfg_.yaw0_deg);
    imu_calib_service_->setPriority(microros::Service::Priority::High);
    services.push_back(imu_calib_service_);
  }

  if (runtime_cfg_.lane_keep) {
    lane_keep_service_ = std::make_shared<adas::LaneKeepService>(runtime_cfg_.wheelbase_m);
    lane_keep_service_->setSteerOutputEnabled(true);
    lane_keep_service_->setPriority(microros::Service::Priority::High);
    services.push_back(lane_keep_service_);
  }

  if (runtime_cfg_.localization) {
    localization_service_ = std::make_shared<adas::LocalizationService>(runtime_cfg_.wheelbase_m);
    localization_service_->setPriority(microros::Service::Priority::High);
    services.push_back(localization_service_);
  }

  if (runtime_cfg_.camera_calib) {
    camera_calib_service_ = std::make_shared<adas::CameraCalibService>(
        runtime_cfg_.pitch0_deg, runtime_cfg_.yaw0_deg, runtime_cfg_.camera_height_m, runtime_cfg_.fx, runtime_cfg_.fy,
        runtime_cfg_.cx, runtime_cfg_.cy);
    camera_calib_service_->setPriority(microros::Service::Priority::Normal);
    services.push_back(camera_calib_service_);
  }

  service_manager_ = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                                microros::ServiceManager::ThreadingMode::ThreadPool, 3);

  LOGI("Realtime services setup completed (%zu services)", services.size());
}

void AdasApp::setupSimulatedServices(double wheelbase, double desired_speed, double pitch0_deg, double yaw0_deg,
                                     double camera_height)
{
  lane_keep_service_ = std::make_shared<adas::LaneKeepService>(wheelbase, desired_speed);
  lane_keep_service_->setSteerOutputEnabled(true);
  localization_service_ = std::make_shared<adas::LocalizationService>(wheelbase);
  camera_calib_service_ = std::make_shared<adas::CameraCalibService>(pitch0_deg, yaw0_deg, camera_height);
  imu_calib_service_ = std::make_shared<adas::ImuCalibService>();
  imu_calib_service_->setMountPrior(0.0, pitch0_deg, yaw0_deg);
  internal_subscriber_ = std::make_shared<adas::InternalSubscriber>();

  lane_keep_service_->setPriority(microros::Service::Priority::High);
  localization_service_->setPriority(microros::Service::Priority::High);
  camera_calib_service_->setPriority(microros::Service::Priority::Normal);
  imu_calib_service_->setPriority(microros::Service::Priority::High);
  internal_subscriber_->setPriority(microros::Service::Priority::Low);

  std::vector<microros::ServicePtr> services = {lane_keep_service_, localization_service_, camera_calib_service_,
                                                imu_calib_service_, internal_subscriber_};
  service_manager_ = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::Simulated, services);
  LOGI("Simulated AdasApp services ready");
}

void AdasApp::publishChassis(const adas::ChassisSample& chassis)
{
  if (service_manager_)
    service_manager_->publish(adas::topics::kVehicleChassis, chassis);
}

void AdasApp::publishLanes(const adas::LanePathMsg& lanes)
{
  if (service_manager_)
    service_manager_->publish(adas::topics::kVisionPath, lanes);
}

void AdasApp::publishGps(const adas::GpsSample& gps)
{
  if (service_manager_)
    service_manager_->publish(adas::topics::kGpsLocation, gps);
}

void AdasApp::publishImu(const adas::ImuSample& imu)
{
  if (service_manager_)
    service_manager_->publish(adas::topics::kImuYaw, imu);
}

void AdasApp::publishLaneUv(const adas::LaneUvMsg& uv)
{
  if (service_manager_)
    service_manager_->publish(adas::topics::kCalibLaneUv, uv);
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

void AdasApp::step(uint64_t timestamp_us)
{
  if (!service_manager_ || mode_ != Mode::Simulated)
    return;
  service_manager_->setTime(timestamp_us);
  service_manager_->step();
}
