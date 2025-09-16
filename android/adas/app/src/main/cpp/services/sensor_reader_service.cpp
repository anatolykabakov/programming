#include "sensor_reader_service.h"
#include "utils/logger.h"

void SensorReaderService::configure()
{
  LOGI("Configuring SensorReaderService...");

  // Subscribe to all sensor topics
  subscribe<ai::flow::android::ZMQMessage>("sensors/imu",
                                           [this](const ai::flow::android::ZMQMessage& msg) { processImuData(msg); });

  subscribe<ai::flow::android::ZMQMessage>(
      "sensors/gps/location", [this](const ai::flow::android::ZMQMessage& msg) { processGpsLocation(msg); });

  subscribe<ai::flow::android::ZMQMessage>("sensors/gps/data",
                                           [this](const ai::flow::android::ZMQMessage& msg) { processGpsData(msg); });

  subscribe<ai::flow::android::ZMQMessage>(
      "sensors/camera/image", [this](const ai::flow::android::ZMQMessage& msg) { processCameraBuffer(msg); });

  LOGI("SensorReaderService configured with all subscriptions");
}

void SensorReaderService::reset() { LOGI("Resetting SensorReaderService..."); }

void SensorReaderService::processImuData(const ai::flow::android::ZMQMessage& msg)
{
  LOGI("Received imuData message");
  logSensorData(msg);

  LOGI("Published imuData to internal topic: sensors/imu");
}

void SensorReaderService::processGpsLocation(const ai::flow::android::ZMQMessage& msg)
{
  LOGI("Received gpsLocation message");
  logSensorData(msg);
}

void SensorReaderService::processGpsData(const ai::flow::android::ZMQMessage& msg)
{
  LOGI("Received gpsData message");
  logSensorData(msg);
}

void SensorReaderService::processCameraBuffer(const ai::flow::android::ZMQMessage& msg)
{
  LOGI("Received wideRoadCameraBuffer message");
  logSensorData(msg);
}

void SensorReaderService::logSensorData(const ai::flow::android::ZMQMessage& message)
{
  if (message.has_imu_data()) {
    const auto& imu_data = message.imu_data();
    LOGI("Combined IMU Data: accel=[%.6f, %.6f, %.6f], gyro=[%.6f, %.6f, %.6f], mag=[%.6f, %.6f, %.6f], timestamp=%ld",
         imu_data.accel_x(), imu_data.accel_y(), imu_data.accel_z(), imu_data.gyro_x(), imu_data.gyro_y(),
         imu_data.gyro_z(), imu_data.mag_x(), imu_data.mag_y(), imu_data.mag_z(), imu_data.timestamp());
  } else if (message.has_camera_image()) {
    const auto& camera_image = message.camera_image();
    LOGI("Camera Image: size=%zu bytes, resolution=%dx%d, format=%s, frame_id=%d, timestamp=%ld",
         camera_image.image_data().size(), camera_image.width(), camera_image.height(), camera_image.format().c_str(),
         camera_image.frame_id(), camera_image.timestamp());
  }
}
