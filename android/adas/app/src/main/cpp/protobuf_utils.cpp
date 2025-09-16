#include "protobuf_utils.h"
#include "logger.h"
#include <sstream>

ai::flow::android::ZMQMessage ProtobufUtils::createCameraImageMessage(
    const std::vector<uint8_t>& imageData,
    int width, int height,
    const std::string& format,
    int frameId,
    int64_t timestamp) {

    ai::flow::android::CameraImage cameraImage;
    cameraImage.set_timestamp(timestamp);
    cameraImage.set_width(width);
    cameraImage.set_height(height);
    cameraImage.set_format(format);
    cameraImage.set_frame_id(frameId);
    cameraImage.set_image_data(imageData.data(), imageData.size());
    cameraImage.set_camera_type(ai::flow::android::CameraImage::WIDE_ROAD);

    ai::flow::android::ZMQMessage message;
    message.set_timestamp(timestamp);
    message.set_topic("wideRoadCameraBuffer");
    message.mutable_camera_image()->CopyFrom(cameraImage);

    return message;
}

ai::flow::android::ZMQMessage ProtobufUtils::createCameraStateMessage(
    int frameId,
    bool isRecording,
    bool isConnected,
    int64_t timestamp) {

    ai::flow::android::CameraState cameraState;
    cameraState.set_timestamp(timestamp);
    cameraState.set_frame_id(frameId);
    cameraState.set_is_recording(isRecording);
    cameraState.set_is_connected(isConnected);
    cameraState.set_width(640);
    cameraState.set_height(480);
    cameraState.set_format("YUV420");

    ai::flow::android::ZMQMessage message;
    message.set_timestamp(timestamp);
    message.set_topic("wideRoadCameraState");
    message.mutable_camera_state()->CopyFrom(cameraState);

    return message;
}

ai::flow::android::ZMQMessage ProtobufUtils::createGPSLocationMessage(
    double latitude, double longitude, double altitude,
    float speed, float bearing, int64_t timestamp) {

    ai::flow::android::GPSLocation gpsLocation;
    gpsLocation.set_timestamp(timestamp);
    gpsLocation.set_latitude(latitude);
    gpsLocation.set_longitude(longitude);
    gpsLocation.set_altitude(altitude);
    gpsLocation.set_speed(speed);
    gpsLocation.set_bearing(bearing);
    gpsLocation.set_fix_type(ai::flow::android::GPSLocation::FIX_3D);

    ai::flow::android::ZMQMessage message;
    message.set_timestamp(timestamp);
    message.set_topic("gpsLocation");
    message.mutable_gps_location()->CopyFrom(gpsLocation);

    return message;
}

ai::flow::android::ZMQMessage ProtobufUtils::createGPSDataMessage(
    double latitude, double longitude, double altitude,
    float speed, float bearing, int64_t timestamp) {

    ai::flow::android::GPSData gpsData;
    gpsData.set_timestamp(timestamp);
    gpsData.set_latitude(latitude);
    gpsData.set_longitude(longitude);
    gpsData.set_altitude(altitude);
    gpsData.set_speed(speed);
    gpsData.set_bearing(bearing);
    gpsData.set_gps_timestamp(timestamp);
    gpsData.set_provider("gps");

    ai::flow::android::ZMQMessage message;
    message.set_timestamp(timestamp);
    message.set_topic("gpsData");
    message.mutable_gps_data()->CopyFrom(gpsData);

    return message;
}

ai::flow::android::ZMQMessage ProtobufUtils::createIMUDataMessage(
    const std::vector<float>& accel,
    const std::vector<float>& gyro,
    const std::vector<float>& mag,
    int64_t timestamp) {

    ai::flow::android::IMUData imuData;
    imuData.set_timestamp(timestamp);

    if (accel.size() >= 3) {
        imuData.set_accel_x(accel[0]);
        imuData.set_accel_y(accel[1]);
        imuData.set_accel_z(accel[2]);
    }

    if (gyro.size() >= 3) {
        imuData.set_gyro_x(gyro[0]);
        imuData.set_gyro_y(gyro[1]);
        imuData.set_gyro_z(gyro[2]);
    }

    if (mag.size() >= 3) {
        imuData.set_mag_x(mag[0]);
        imuData.set_mag_y(mag[1]);
        imuData.set_mag_z(mag[2]);
    }

    imuData.set_sample_count(1);

    ai::flow::android::ZMQMessage message;
    message.set_timestamp(timestamp);
    message.set_topic("imuData");
    message.mutable_imu_data()->CopyFrom(imuData);

    return message;
}

std::vector<uint8_t> ProtobufUtils::serializeMessage(const ai::flow::android::ZMQMessage& message) {
    std::string serialized;
    message.SerializeToString(&serialized);

    return std::vector<uint8_t>(serialized.begin(), serialized.end());
}

std::unique_ptr<ai::flow::android::ZMQMessage> ProtobufUtils::deserializeMessage(
    const std::vector<uint8_t>& data) {

    auto message = std::make_unique<ai::flow::android::ZMQMessage>();

    if (message->ParseFromArray(data.data(), data.size())) {
        return message;
    } else {
        LOGE("Failed to deserialize ZMQ message");
        return nullptr;
    }
}

int64_t ProtobufUtils::getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void ProtobufUtils::logMessageInfo(const ai::flow::android::ZMQMessage& message) {
    LOGD("Message: topic=%s, timestamp=%ld, size=%zu bytes",
         message.topic().c_str(), message.timestamp(), message.ByteSizeLong());
}
