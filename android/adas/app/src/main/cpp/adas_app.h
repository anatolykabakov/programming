#pragma once

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <thread>
#include <memory>
#include <zmq.hpp>
#include "messages.pb.h"

class AdasApp {
public:
    AdasApp();
    ~AdasApp();

    bool start();
    void stop();

private:
    void readerThread();
    void setupZMQ();
    
    // Helper methods for message processing
    void processPollResults(const std::vector<zmq::pollitem_t>& poll_items, 
                           const std::vector<std::string>& topic_names);
    void processMessage(const std::string& topic_name, zmq::socket_t* socket);
    void logSensorData(const ai::flow::android::ZMQMessage& message);
    void forwardMessage(const std::string& topic_name, const ai::flow::android::ZMQMessage& message);

    std::atomic<bool> running_;
    std::thread reader_thread_;
    long long start_time_;

    // Direct ZMQ sockets based on working test code
    std::unique_ptr<zmq::context_t> zmq_context_;
    std::map<std::string, std::string> zmq_topics_;
    std::unique_ptr<zmq::socket_t> imu_publisher_;
    
    // Map topic names to subscriber sockets for easy iteration
    std::map<std::string, std::unique_ptr<zmq::socket_t>> topic_subscribers_;
};
