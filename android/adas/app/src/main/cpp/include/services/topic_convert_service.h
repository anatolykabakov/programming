#pragma once

#include "middleware/middleware.hpp"
#include "messages.pb.h"
#include "utils/gps_local_projector.h"

namespace adas {

class TopicConvertService : public adas::Service {
public:
  struct Config {
    double steer_ratio = 15.7;
  };

  TopicConvertService() : TopicConvertService(Config{}) {}
  explicit TopicConvertService(Config config);

  void configure() override;
  void reset() override;
  const Config& config() const { return config_; }

private:
  void onVisionLanes(const ai::flow::adas::ZMQMessage& msg);
  void onVehicleState(const ai::flow::adas::ZMQMessage& msg);
  void onImu(const ai::flow::adas::ZMQMessage& msg);
  void onGps(const ai::flow::adas::ZMQMessage& msg);
  void onLaneUv(const ai::flow::adas::ZMQMessage& msg);
  void onCameraOdometry(const ai::flow::adas::ZMQMessage& msg);

  Config config_;
  double steer_ratio_ = 15.7;
  GpsLocalProjector gps_proj_;
};

}  // namespace adas
