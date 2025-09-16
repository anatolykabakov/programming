#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include "protobuf_utils.h"
#include "messaging/impl_zmq.h"

// Test configuration
struct TestConfig {
    static constexpr int DEFAULT_TIMEOUT_MS = 5000;
    static constexpr int MESSAGE_DELAY_MS = 100;
    static constexpr int THREAD_STARTUP_DELAY_MS = 200;
    static constexpr int ZMQ_BIND_DELAY_MS = 100;
};

// Test IMU data structure
struct TestIMUData {
    float accel_x = 1.0f;
    float accel_y = 2.0f;
    float accel_z = 3.0f;
    float gyro_x = 4.0f;
    float gyro_y = 5.0f;
    float gyro_z = 6.0f;
    float mag_x = 7.0f;
    float mag_y = 8.0f;
    float mag_z = 9.0f;
    long timestamp = 1234567890;
};

// Test utilities
class TestUtils {
public:
    // Wait for a condition to be true
    template<typename Predicate>
    static bool waitForCondition(Predicate pred, int timeoutMs = TestConfig::DEFAULT_TIMEOUT_MS) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start).count() < timeoutMs) {
            if (pred()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }
    
    // Wait for a specific duration
    static void waitFor(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    
    // Create test IMU data
    static TestIMUData createTestIMUData() {
        return TestIMUData{};
    }
    
    static TestIMUData createTestIMUData(float accel_x, float accel_y, float accel_z,
                                        float gyro_x, float gyro_y, float gyro_z,
                                        float mag_x, float mag_y, float mag_z,
                                        long timestamp) {
        TestIMUData data;
        data.accel_x = accel_x;
        data.accel_y = accel_y;
        data.accel_z = accel_z;
        data.gyro_x = gyro_x;
        data.gyro_y = gyro_y;
        data.gyro_z = gyro_z;
        data.mag_x = mag_x;
        data.mag_y = mag_y;
        data.mag_z = mag_z;
        data.timestamp = timestamp;
        return data;
    }
    
    // Create test IMU message
    static std::vector<uint8_t> createTestIMUMessage(const TestIMUData& data) {
        // Create vectors for accel, gyro, mag
        std::vector<float> accel = {data.accel_x, data.accel_y, data.accel_z};
        std::vector<float> gyro = {data.gyro_x, data.gyro_y, data.gyro_z};
        std::vector<float> mag = {data.mag_x, data.mag_y, data.mag_z};
        
        auto zmqMessage = ProtobufUtils::createIMUDataMessage(accel, gyro, mag, data.timestamp);
        return ProtobufUtils::serializeMessage(zmqMessage);
    }
    
    // Verify IMU data in a message
    static bool verifyIMUData(const ai::flow::android::ZMQMessage& message,
                             const TestIMUData& expectedData) {
        if (!message.has_imu_data()) {
            return false;
        }
        
        const auto& imuData = message.imu_data();
        
        return (std::abs(imuData.accel_x() - expectedData.accel_x) < 0.001f &&
                std::abs(imuData.accel_y() - expectedData.accel_y) < 0.001f &&
                std::abs(imuData.accel_z() - expectedData.accel_z) < 0.001f &&
                std::abs(imuData.gyro_x() - expectedData.gyro_x) < 0.001f &&
                std::abs(imuData.gyro_y() - expectedData.gyro_y) < 0.001f &&
                std::abs(imuData.gyro_z() - expectedData.gyro_z) < 0.001f &&
                std::abs(imuData.mag_x() - expectedData.mag_x) < 0.001f &&
                std::abs(imuData.mag_y() - expectedData.mag_y) < 0.001f &&
                std::abs(imuData.mag_z() - expectedData.mag_z) < 0.001f &&
                imuData.timestamp() == expectedData.timestamp);
    }
    
    // Create ZMQ message from data
    static std::unique_ptr<ZMQMessage> createZMQMessage(const std::vector<uint8_t>& data) {
        auto message = std::make_unique<ZMQMessage>();
        message->init(reinterpret_cast<char*>(const_cast<uint8_t*>(data.data())), data.size());
        return message;
    }
};
