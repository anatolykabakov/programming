#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

// Generated protobuf headers
#include "messages.pb.h"
#include "camera.pb.h"
#include "gps.pb.h"
#include "imu.pb.h"

class ProtobufUtils {
public:
    // Create camera image message
    static ai::flow::android::ZMQMessage createCameraImageMessage(
        const std::vector<uint8_t>& imageData,
        int width, int height,
        const std::string& format,
        int frameId,
        int64_t timestamp);

    // Create camera state message
    static ai::flow::android::ZMQMessage createCameraStateMessage(
        int frameId,
        bool isRecording,
        bool isConnected,
        int64_t timestamp);

    // Create GPS location message
    static ai::flow::android::ZMQMessage createGPSLocationMessage(
        double latitude, double longitude, double altitude,
        float speed, float bearing, int64_t timestamp);

    // Create GPS data message
    static ai::flow::android::ZMQMessage createGPSDataMessage(
        double latitude, double longitude, double altitude,
        float speed, float bearing, int64_t timestamp);

    // Create IMU data message
    static ai::flow::android::ZMQMessage createIMUDataMessage(
        const std::vector<float>& accel,
        const std::vector<float>& gyro,
        const std::vector<float>& mag,
        int64_t timestamp);

    // Serialize message to bytes
    static std::vector<uint8_t> serializeMessage(const ai::flow::android::ZMQMessage& message);

    // Deserialize bytes to message
    static std::unique_ptr<ai::flow::android::ZMQMessage> deserializeMessage(
        const std::vector<uint8_t>& data);

    // Get current timestamp in milliseconds
    static int64_t getCurrentTimestamp();

    // Log message info
    static void logMessageInfo(const ai::flow::android::ZMQMessage& message);
};
