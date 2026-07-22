#pragma once

#include <memory>
#include <string>
#include <vector>
#include <zmq.hpp>

#include "framework/service_manager.hpp"
#include "messages.pb.h"

/** Default inbound bind (overridden by assets/config.json → zmq.endpoint_in). */
inline constexpr const char* kZmqEndpointIn = "tcp://127.0.0.1:5555";
/** Default outbound bind (overridden by assets/config.json → zmq.endpoint_out). */
inline constexpr const char* kZmqEndpointOut = "tcp://127.0.0.1:5556";

/** Internal topics forwarded to the outbound ZMQ PUB. */
inline const std::vector<std::string> kZmqOutboundTopics = {
    "can/rx",         "panda/health", "vehicle/state", "control/lane_keep", "localization/pose", "calibration/camera",
    "controls/steer",
};

/**
 * Bridges one inbound + one outbound ZMQ socket to the internal topic bus.
 *
 * Wire format (multipart):
 *   [0] topic UTF-8 string
 *   [1] ai.flow.adas.ZMQMessage protobuf
 *
 * Bind: SUB @ endpoint_in, PUB @ endpoint_out (from AdasRuntimeConfig / config.json).
 * External peers connect (Java sensors/tools PUB→IN, Java BagLogger SUB←OUT).
 */
class ZmqBridgeService : public microros::Service {
public:
  ZmqBridgeService() = default;
  explicit ZmqBridgeService(std::string endpoint_in, std::string endpoint_out);

  void configure() override;
  void reset() override;

private:
  void initSockets();
  void zmqPollTimerCallback();
  void processInbound();
  void onInternalMessage(const std::string& topic_name, const ai::flow::adas::ZMQMessage& msg);

  std::string endpoint_in_ = kZmqEndpointIn;
  std::string endpoint_out_ = kZmqEndpointOut;

  std::unique_ptr<zmq::context_t> zmq_context_;
  std::unique_ptr<zmq::socket_t> sub_in_;   // bind SUB
  std::unique_ptr<zmq::socket_t> pub_out_;  // bind PUB
  std::vector<zmq::pollitem_t> poll_items_;
};
