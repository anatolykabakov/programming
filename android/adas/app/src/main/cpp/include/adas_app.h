#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "framework/service_manager.hpp"
#include "messages.pb.h"
#include "services/camera_calib_service.h"
#include "services/imu_calib_service.h"
#include "services/lane_keep_service.h"
#include "services/localization_service.h"
#include "services/panda_service.h"
#include "services/topic_convert_service.h"
#include "services/zmq_bridge_service.h"
#include "utils/adas_config.h"
#include "utils/adas_topics.h"

namespace adas {

/** Captures algorithm ZMQ outputs for Python pop_messages(). */
class InternalSubscriber : public microros::Service {
public:
  LaneKeepOutput lane_keep_;
  bool has_new_lane_keep = false;
  LocalizationPose pose_;
  bool has_new_pose = false;
  CameraCalibrationState camera_calib_;
  bool has_new_camera_calib = false;

  void configure() override;
  void reset() override;
};

}  // namespace adas

/**
 * ADAS application: ServiceManager + panda/ZMQ (Realtime) or algorithm services (Simulated).
 */
class AdasApp {
public:
  enum class Mode { RealTime, Simulated };

  AdasApp();
  explicit AdasApp(int usb_fd, std::string dbc_path = {}, adas::AdasRuntimeConfig cfg = {});

  AdasApp(Mode mode, double wheelbase = 2.636, double desired_speed = 12.0, double pitch0_deg = -6.0,
          double yaw0_deg = 0.0, double camera_height = 1.40);

  ~AdasApp();

  bool start();
  void stop();

  void setRuntimeConfig(const adas::AdasRuntimeConfig& cfg) { runtime_cfg_ = cfg; }
  const adas::AdasRuntimeConfig& runtimeConfig() const { return runtime_cfg_; }

  std::shared_ptr<microros::ServiceManager> getServiceManager() { return service_manager_; }
  Mode mode() const { return mode_; }

  void publishChassis(const adas::ChassisSample& chassis);
  void publishLanes(const adas::LanePathMsg& lanes);
  void publishGps(const adas::GpsSample& gps);
  void publishImu(const adas::ImuSample& imu);
  void publishLaneUv(const adas::LaneUvMsg& uv);

  void resetLocalization(double x = 0, double y = 0, double yaw = 0, double v = 0, double yaw_rate = 0);
  void setCameraIntrinsics(double fx, double fy, double cx, double cy);
  void setCameraEstimate(double pitch_deg, double yaw_deg);

  void step(uint64_t timestamp_us);

  adas::LaneKeepService* laneKeep() { return lane_keep_service_.get(); }
  adas::LocalizationService* localization() { return localization_service_.get(); }
  adas::CameraCalibService* cameraCalib() { return camera_calib_service_.get(); }
  adas::ImuCalibService* imuCalib() { return imu_calib_service_.get(); }
  adas::InternalSubscriber& subscriber() { return *internal_subscriber_; }

private:
  void setupRealtimeServices();
  void setupSimulatedServices(double wheelbase, double desired_speed, double pitch0_deg, double yaw0_deg,
                              double camera_height);

  Mode mode_ = Mode::RealTime;
  std::atomic<bool> running_{false};
  int usb_fd_ = -1;
  std::string dbc_path_;
  adas::AdasRuntimeConfig runtime_cfg_;

  std::shared_ptr<microros::ServiceManager> service_manager_;

  std::shared_ptr<PandaService> panda_service_;
  std::shared_ptr<ZmqBridgeService> zmq_bridge_service_;
  std::shared_ptr<adas::TopicConvertService> topic_convert_service_;

  std::shared_ptr<adas::LaneKeepService> lane_keep_service_;
  std::shared_ptr<adas::LocalizationService> localization_service_;
  std::shared_ptr<adas::CameraCalibService> camera_calib_service_;
  std::shared_ptr<adas::ImuCalibService> imu_calib_service_;
  std::shared_ptr<adas::InternalSubscriber> internal_subscriber_;
};
