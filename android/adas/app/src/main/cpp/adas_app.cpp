#include "adas_app.h"
#include "protobuf_utils.h"
#include "logger.h"
#include <chrono>
#include <sstream>
#include <zmq.hpp>

AdasApp::AdasApp() : running_(false) {
}

AdasApp::~AdasApp() {
    stop();
}

bool AdasApp::start() {
    if (running_) {
        LOGI("AdasApp already running");
        return true;
    }

    setupZMQ();

    try {
        running_ = true;
        reader_thread_ = std::thread(&AdasApp::readerThread, this);
        return true;

    } catch (const std::exception& e) {
        LOGE("Exception in start(): %s", e.what());
        return false;
    }
}

void AdasApp::stop() {
    if (!running_) {
        return;
    }

    LOGI("Stopping AdasApp...");
    running_ = false;

    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }
    LOGI("AdasApp stopped");
}

void AdasApp::readerThread() {
    LOGI("Reader thread started");

    try {
        int loopCount = 0;
        while (running_) {
            loopCount++;

            if (loopCount % 100 == 0) {
                LOGI("Reader thread alive, iteration: %d", loopCount);
            }

            // Process messages from all subscribers using zmq::poll
            if (!topic_subscribers_.empty()) {
                // Prepare poll items dynamically from topic_subscribers_ map
                std::vector<zmq::pollitem_t> poll_items;
                std::vector<std::string> topic_names;
                
                // Reserve space for better performance
                poll_items.reserve(topic_subscribers_.size());
                topic_names.reserve(topic_subscribers_.size());
                
                for (const auto& pair : topic_subscribers_) {
                    poll_items.push_back({*pair.second, 0, ZMQ_POLLIN, 0});
                    topic_names.push_back(pair.first);
                }

                // Poll for messages with short timeout
                auto poll_result = zmq::poll(poll_items.data(), poll_items.size(), std::chrono::milliseconds(50));
                
                if (poll_result > 0) {
                    LOGD("Messages available on %d sensor socket(s)", poll_result);
                    processPollResults(poll_items, topic_names);
                }
            }
            
            // Sleep briefly to prevent excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

    } catch (const std::exception& e) {
        LOGE("Exception in readerThread(): %s", e.what());
    } catch (...) {
        LOGE("Unknown exception in readerThread()");
    }

    LOGI("Reader thread stopped");
}

void AdasApp::setupZMQ() {

    zmq_topics_ = {
        {"imuData", "tcp://127.0.0.1:5558"},
        {"accelerometerData", "tcp://127.0.0.1:5560"},
        {"gyroscopeData", "tcp://127.0.0.1:5561"},
        {"magnetometerData", "tcp://127.0.0.1:5562"},
        {"gpsData", "tcp://127.0.0.1:5557"},
        {"wideRoadCameraState", "tcp://127.0.0.1:5555"},
        {"wideRoadCameraBuffer", "tcp://127.0.0.1:5556"},
    };

    try {
        // Create ZMQ context (based on working test code)
        zmq_context_ = std::make_unique<zmq::context_t>(1);

        // Create publisher socket
        imu_publisher_ = std::make_unique<zmq::socket_t>(*zmq_context_, ZMQ_PUB);
        imu_publisher_->bind("tcp://127.0.0.1:5564"); // Use port 5564 for output
        LOGI("Direct ZMQ publisher bound to tcp://127.0.0.1:5564");

        // Create subscribers dynamically by iterating through zmq_topics_
        for (const auto& topic_pair : zmq_topics_) {
            const std::string& topic_name = topic_pair.first;
            const std::string& endpoint = topic_pair.second;
            
            // Skip publisher endpoints (we only want subscriber endpoints)
            if (topic_name == "imuDataOutput") {
                continue;
            }
            
            // Create subscriber socket for this topic
            auto subscriber = std::make_unique<zmq::socket_t>(*zmq_context_, ZMQ_SUB);
            subscriber->connect(endpoint);
            subscriber->set(zmq::sockopt::subscribe, ""); // Subscribe to all messages
            subscriber->set(zmq::sockopt::rcvtimeo, 100); // 100ms timeout
            
            // Store in topic_subscribers_ map
            topic_subscribers_[topic_name] = std::move(subscriber);
            
            LOGI("Subscriber for topic '%s' connected to %s", topic_name.c_str(), endpoint.c_str());
        }
        
        LOGI("Topic-subscriber mapping initialized with %zu entries", topic_subscribers_.size());

        // Give sockets time to establish connection
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        LOGI("Direct ZMQ setup completed successfully");

    } catch (const std::exception& e) {
        LOGE("Exception in setupDirectZMQ(): %s", e.what());
    }
}

void AdasApp::processPollResults(const std::vector<zmq::pollitem_t>& poll_items, 
                                const std::vector<std::string>& topic_names) {
    // Check each sensor socket for messages
    for (size_t i = 0; i < poll_items.size(); i++) {
        if (poll_items[i].revents & ZMQ_POLLIN) {
            const std::string& topic_name = topic_names[i];
            zmq::socket_t* current_socket = topic_subscribers_[topic_name].get();
            
            if (current_socket) {
                processMessage(topic_name, current_socket);
            }
        }
    }
}

void AdasApp::processMessage(const std::string& topic_name, zmq::socket_t* socket) {
    zmq::message_t recv_msg;
    auto result = socket->recv(recv_msg);
    
    if (!result) {
        LOGD("Failed to receive %s message", topic_name.c_str());
        return;
    }
    
    LOGI("Received %s message, size: %ld", topic_name.c_str(), recv_msg.size());
    
    auto message = std::make_unique<ai::flow::android::ZMQMessage>();
    if (!message->ParseFromArray(recv_msg.data(), recv_msg.size())) {
        LOGE("Failed to deserialize %s message", topic_name.c_str());
        return;
    }

    // Log sensor-specific data based on topic
    std::string received_topic = message->topic();
    LOGI("Successfully parsed %s message with topic: %s", topic_name.c_str(), received_topic.c_str());
    
    // Log sensor data
    logSensorData(*message);
    
    // Forward message via publisher
    forwardMessage(topic_name, *message);
}

void AdasApp::logSensorData(const ai::flow::android::ZMQMessage& message) {
    if (message.has_imu_data()) {
        const auto& imu_data = message.imu_data();
        LOGI("Combined IMU Data: accel=[%.6f, %.6f, %.6f], gyro=[%.6f, %.6f, %.6f], mag=[%.6f, %.6f, %.6f], timestamp=%ld",
             imu_data.accel_x(), imu_data.accel_y(), imu_data.accel_z(),
             imu_data.gyro_x(), imu_data.gyro_y(), imu_data.gyro_z(),
             imu_data.mag_x(), imu_data.mag_y(), imu_data.mag_z(),
             imu_data.timestamp());
             
    } else if (message.has_accelerometer_data()) {
        const auto& accel_data = message.accelerometer_data();
        LOGI("Accelerometer Data: [%.6f, %.6f, %.6f], accuracy=%d, calibrated=%s, timestamp=%ld",
             accel_data.x(), accel_data.y(), accel_data.z(),
             accel_data.accuracy(), accel_data.is_calibrated() ? "true" : "false",
             accel_data.timestamp());
             
    } else if (message.has_gyroscope_data()) {
        const auto& gyro_data = message.gyroscope_data();
        LOGI("Gyroscope Data: [%.6f, %.6f, %.6f], accuracy=%d, calibrated=%s, timestamp=%ld",
             gyro_data.x(), gyro_data.y(), gyro_data.z(),
             gyro_data.accuracy(), gyro_data.is_calibrated() ? "true" : "false",
             gyro_data.timestamp());
             
    } else if (message.has_magnetometer_data()) {
        const auto& mag_data = message.magnetometer_data();
        LOGI("Magnetometer Data: [%.6f, %.6f, %.6f], field_strength=%.6f, accuracy=%d, calibrated=%s, timestamp=%ld",
             mag_data.x(), mag_data.y(), mag_data.z(), mag_data.field_strength(),
             mag_data.accuracy(), mag_data.is_calibrated() ? "true" : "false",
             mag_data.timestamp());
             
    } else if (message.has_camera_state()) {
        const auto& camera_state = message.camera_state();
        LOGI("Camera State: connected=%s, recording=%s, resolution=%dx%d, format=%s, timestamp=%ld",
             camera_state.is_connected() ? "true" : "false",
             camera_state.is_recording() ? "true" : "false",
             camera_state.width(), camera_state.height(),
             camera_state.format().c_str(), camera_state.timestamp());
             
    } else if (message.has_camera_image()) {
        const auto& camera_image = message.camera_image();
        LOGI("Camera Image: size=%d bytes, resolution=%dx%d, format=%s, frame_id=%d, timestamp=%ld",
             camera_image.image_data().size(), camera_image.width(), camera_image.height(),
             camera_image.format().c_str(), camera_image.frame_id(), camera_image.timestamp());
    }
}

void AdasApp::forwardMessage(const std::string& topic_name, const ai::flow::android::ZMQMessage& message) {
    if (!imu_publisher_) {
        return;
    }
    
    try {
        std::string serialized;
        message.SerializeToString(&serialized);
        imu_publisher_->send(zmq::buffer(serialized), zmq::send_flags::none);
        LOGI("%s message forwarded via publisher successfully", topic_name.c_str());
    } catch (const std::exception& e) {
        LOGE("Failed to forward %s message: %s", topic_name.c_str(), e.what());
    }
}
