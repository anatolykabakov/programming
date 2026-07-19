#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <zmq.hpp>
#include "framework/service_manager.hpp"
#include "messages.pb.h"

const std::map<std::string, std::string> DEFAULT_SUB_TOPICS = {
    {"controls/steer", "tcp://127.0.0.1:5564"},         // SteerCommand inject (test / planner)
    {"sensors/imu", "tcp://127.0.0.1:5558"},            // IMU data
    {"sensors/gps/location", "tcp://127.0.0.1:5557"},   // GPS location
    {"sensors/gps/data", "tcp://127.0.0.1:5559"},       // GPS data
    {"sensors/camera/state", "tcp://127.0.0.1:5555"},   // Camera state
    {"sensors/camera/image", "tcp://127.0.0.1:5556"},   // Camera image
    {"sensors/accelerometer", "tcp://127.0.0.1:5560"},  // Accelerometer
    {"sensors/gyroscope", "tcp://127.0.0.1:5561"},      // Gyroscope
    {"sensors/magnetometer", "tcp://127.0.0.1:5562"},   // Magnetometer
};

const std::map<std::string, std::string> DEFAULT_PUB_TOPICS = {
    {"can/rx", "tcp://127.0.0.1:5563"},
    {"panda/health", "tcp://127.0.0.1:5565"},
    {"vehicle/state", "tcp://127.0.0.1:5566"},
};

class ZmqBridgeService : public microros::Service {
public:
  ZmqBridgeService() = default;
  explicit ZmqBridgeService(const std::map<std::string, std::string>& sub_topics);

  void configure() override;
  void reset() override;

private:
  void initSubscribers();
  void initPublishers();
  void zmqPollTimerCallback();
  void processExternalMessage(const std::string& topic_name, const std::unique_ptr<zmq::socket_t>& socket);
  void onInternalMessage(const std::string& topic_name, const ai::flow::adas::ZMQMessage& msg);

  std::unique_ptr<zmq::context_t> zmq_context_;
  std::vector<zmq::pollitem_t> poll_items_;
  std::vector<std::string> topic_names_;
  std::map<std::string, std::string> sub_topics_ = DEFAULT_SUB_TOPICS;       // topic -> endpoint mapping
  std::map<std::string, std::string> pub_topics_ = DEFAULT_PUB_TOPICS;       // topic -> endpoint mapping
  std::map<std::string, std::unique_ptr<zmq::socket_t>> topic_subscribers_;  // External ZMQ → Internal
  std::map<std::string, std::unique_ptr<zmq::socket_t>> topic_publishers_;   // Internal → External ZMQ
};
