#include "services/zmq_bridge_service.h"
#include "utils/logger.h"
#include "panda/health.h"
#include "panda/can.h"
#include "utils/protobuf_utils.h"
#include <thread>

ZmqBridgeService::ZmqBridgeService(const std::map<std::string, std::string>& sub_topics) : sub_topics_(sub_topics) {}

void ZmqBridgeService::configure()
{
  LOGI("Configuring ZmqBridgeService...");

  try {
    zmq_context_ = std::make_unique<zmq::context_t>(1);

    initSubscribers();
    initPublishers();

    if (!topic_subscribers_.empty()) {
      scheduleTimer(10000, [this]() { zmqPollTimerCallback(); });
    }

    LOGI("ZMQ services initialized: %zu subscribers, %zu publishers", topic_subscribers_.size(),
         topic_publishers_.size());
  } catch (const std::exception& e) {
    LOGE("Exception in ZmqBridgeService::configure(): %s", e.what());
    throw;
  }
}

void ZmqBridgeService::reset() {}

void ZmqBridgeService::zmqPollTimerCallback()
{
  auto poll_result = zmq::poll(poll_items_.data(), poll_items_.size(), std::chrono::milliseconds(1));

  if (poll_result > 0) {
    for (size_t i = 0; i < poll_items_.size(); i++) {
      if (poll_items_[i].revents & ZMQ_POLLIN) {
        const std::string& topic_name = topic_names_[i];
        const std::unique_ptr<zmq::socket_t>& current_socket = topic_subscribers_[topic_name];
        if (current_socket) {
          processExternalMessage(topic_name, current_socket);
        }
      }
    }
  }
}

void ZmqBridgeService::processExternalMessage(const std::string& topic_name,
                                              const std::unique_ptr<zmq::socket_t>& socket)
{
  zmq::message_t recv_msg;
  auto result = socket->recv(recv_msg);

  if (!result) {
    LOGD("No message received from %s", topic_name.c_str());
    return;
  }

  LOGI("✓✓✓ Received external ZMQ message for topic '%s', size: %zu bytes", topic_name.c_str(), recv_msg.size());

  // Parse the protobuf message
  ai::flow::adas::ZMQMessage message;
  if (!message.ParseFromArray(recv_msg.data(), recv_msg.size())) {
    LOGE("Failed to deserialize %s message from external ZMQ", topic_name.c_str());
    return;
  }

  publish(topic_name, message);
}

void ZmqBridgeService::onInternalMessage(const std::string& topic_name, const ai::flow::adas::ZMQMessage& msg)
{
  LOGI("✓✓✓ Received internal message for topic '%s'", topic_name.c_str());

  auto publisher_it = topic_publishers_.find(topic_name);
  if (publisher_it == topic_publishers_.end()) {
    LOGE("No ZMQ publisher found for topic '%s'", topic_name.c_str());
    return;
  }

  std::string serialized_data;
  if (!msg.SerializeToString(&serialized_data)) {
    LOGE("Failed to serialize message for topic '%s'", topic_name.c_str());
    return;
  }

  zmq::message_t zmq_msg(serialized_data.data(), serialized_data.size());
  auto result = publisher_it->second->send(zmq_msg, zmq::send_flags::dontwait);

  if (result) {
    LOGI("✓✓✓ Sent internal message to external ZMQ for topic '%s' SUCCESS", topic_name.c_str());
  } else {
    LOGE("Failed to send message to external ZMQ for topic '%s'", topic_name.c_str());
  }
}

void ZmqBridgeService::initSubscribers()
{
  for (const auto& [topic_name, endpoint] : sub_topics_) {
    try {
      std::unique_ptr<zmq::socket_t> subscriber;
      subscriber = std::make_unique<zmq::socket_t>(*zmq_context_, ZMQ_SUB);
      subscriber->connect(endpoint);
      subscriber->set(zmq::sockopt::subscribe, "");  // Subscribe to all messages
      subscriber->set(zmq::sockopt::rcvtimeo, 10);   // 10ms timeout (non-blocking)

      topic_subscribers_[topic_name] = std::move(subscriber);
      LOGI("✓ ZMQ SUB socket for topic '%s' CONNECTED to %s", topic_name.c_str(), endpoint.c_str());

      topic_names_.push_back(topic_name);
      poll_items_.push_back(zmq::pollitem_t{*topic_subscribers_[topic_name], 0, ZMQ_POLLIN, 0});

    } catch (const std::exception& e) {
      LOGE("Failed to initialize subscriber for topic '%s': %s", topic_name.c_str(), e.what());
    }
  }
}

void ZmqBridgeService::initPublishers()
{
  LOGI("Initializing ZMQ publishers...");

  for (const auto& [topic_name, endpoint] : pub_topics_) {
    try {
      auto publisher = std::make_unique<zmq::socket_t>(*zmq_context_, ZMQ_PUB);
      publisher->bind(endpoint);
      topic_publishers_[topic_name] = std::move(publisher);

      subscribe<ai::flow::adas::ZMQMessage>(topic_name, [this, topic_name](const ai::flow::adas::ZMQMessage& msg) {
        onInternalMessage(topic_name, msg);
      });

      LOGI("✓ ZMQ PUB socket for topic '%s' BOUND to %s", topic_name.c_str(), endpoint.c_str());

    } catch (const std::exception& e) {
      LOGE("Failed to initialize publisher for topic '%s': %s", topic_name.c_str(), e.what());
    }
  }
}
