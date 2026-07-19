#pragma once

#include "framework/service_manager.hpp"
#include "messages.pb.h"

// ========== SensorReaderService - handles sensor data subscriptions ==========
//
// This service bridges external ZMQ sensor data to internal ServiceManager topics.
// It subscribes to external ZMQ topics and republishes data to internal topics
// that other services can consume.
//
// Internal Topics Published:
//   - "sensors/imu"            : Combined IMU data (accel, gyro, mag)
//   - "sensors/accelerometer"  : Accelerometer data only
//   - "sensors/gyroscope"      : Gyroscope data only
//   - "sensors/magnetometer"   : Magnetometer data only
//   - "sensors/gps/location"   : GPS location data
//   - "sensors/gps/data"       : GPS data
//   - "sensors/camera/state"   : Camera state information
//   - "sensors/camera/buffer"  : Camera frame buffers
//
// Example usage from another service:
//   subscribe<ai::flow::adas::ZMQMessage>("sensors/imu", [this](const auto& msg) {
//     // Process IMU data
//     if (msg.has_imu_data()) {
//       const auto& imu = msg.imu_data();
//       // Use imu.accel_x(), imu.gyro_x(), etc.
//     }
//   });
//
class SensorReaderService : public microros::Service {
public:
  SensorReaderService() = default;

  void configure() override;
  void reset() override;

private:
  void processImuData(const ai::flow::adas::ZMQMessage& msg);
  void processGpsLocation(const ai::flow::adas::ZMQMessage& msg);
  void processGpsData(const ai::flow::adas::ZMQMessage& msg);
  void processCameraBuffer(const ai::flow::adas::ZMQMessage& msg);
  void logSensorData(const ai::flow::adas::ZMQMessage& message);
};
