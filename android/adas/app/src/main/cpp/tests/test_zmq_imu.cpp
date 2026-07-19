#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>
#include <zmq.hpp>
#include "utils/logger.h"
#include "adas_app.h"

#include "test_utils.h"

// Test that starts AdasApp and then connects to its publisher
TEST(ZMQIMUTest, StartAdasAppAndConnect)
{
  LOGI("Starting test: StartAdasAppAndConnect");

  // Skip this test on Linux as it requires real USB device
  // This test is intended for Android platform with connected Panda device
  GTEST_SKIP() << "Test requires USB Panda device, skipping on Linux";

  // Start AdasApp with fake USB FD (this code won't run due to GTEST_SKIP)
  AdasApp app(-1);
  bool started = app.start();  // Fake FD for testing
  EXPECT_TRUE(started) << "AdasApp should start successfully";
  LOGI("AdasApp started successfully");

  // Give AdasApp time to initialize
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  LOGI("AdasApp initialization complete");

  // Create ZMQ context for test
  zmq::context_t context(1);

  // Create IMU publisher to send data to AdasApp (port 5558)
  zmq::socket_t imu_publisher(context, ZMQ_PUB);
  imu_publisher.bind("tcp://127.0.0.1:5558");
  LOGI("Test IMU publisher bound to tcp://127.0.0.1:5558");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Give time to bind

  // Create subscriber to connect to AdasApp's publisher (port 5564)
  zmq::socket_t subscriber(context, ZMQ_SUB);
  subscriber.connect("tcp://127.0.0.1:5564");
  subscriber.set(zmq::sockopt::subscribe, "");
  subscriber.set(zmq::sockopt::rcvtimeo, 1000);  // Increase timeout
  LOGI("Test subscriber connected to tcp://127.0.0.1:5564");

  // Give time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  LOGI("Connections established, preparing to send message");

  // Create and send protobuf IMU message to AdasApp
  ai::flow::adas::ZMQMessage zmq_msg;
  zmq_msg.set_topic("imuData");
  zmq_msg.set_timestamp(
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count());

  // Create IMU data
  auto* imu_data = zmq_msg.mutable_imu_data();
  imu_data->set_accel_x(1.1);
  imu_data->set_accel_y(2.2);
  imu_data->set_accel_z(3.3);
  imu_data->set_gyro_x(4.4);
  imu_data->set_gyro_y(5.5);
  imu_data->set_gyro_z(6.6);
  imu_data->set_mag_x(0.1);
  imu_data->set_mag_y(0.2);
  imu_data->set_mag_z(0.3);
  imu_data->set_timestamp(zmq_msg.timestamp());

  // Serialize to string
  std::string serialized_data;
  zmq_msg.SerializeToString(&serialized_data);

  // Send protobuf message
  imu_publisher.send(zmq::buffer(serialized_data), zmq::send_flags::none);
  LOGI("Sent protobuf IMU message to AdasApp: accel(%.1f, %.1f, %.1f), gyro(%.1f, %.1f, %.1f)", imu_data->accel_x(),
       imu_data->accel_y(), imu_data->accel_z(), imu_data->gyro_x(), imu_data->gyro_y(), imu_data->gyro_z());

  // Give AdasApp time to process and forward the message
  LOGI("Waiting for AdasApp to process message...");
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Try to receive a message from AdasApp
  LOGI("Attempting to receive message from AdasApp...");
  zmq::message_t msg;
  LOGI("Calling subscriber.recv()...");
  auto result = subscriber.recv(msg);
  LOGI("subscriber.recv() returned: %s", result ? "true" : "false");

  if (result) {
    LOGI("Message received successfully! Size: %ld", msg.size());
  } else {
    LOGE("Failed to receive message from AdasApp");
  }

  EXPECT_TRUE(result) << "Message should be received";
  std::string message_data(static_cast<char*>(msg.data()), msg.size());
  EXPECT_FALSE(message_data.empty()) << "Message should not be empty";

  // Deserialize received protobuf message
  ai::flow::adas::ZMQMessage received_msg;
  EXPECT_TRUE(received_msg.ParseFromString(message_data)) << "Should be able to parse received protobuf message";

  // Verify the message contains IMU data
  EXPECT_TRUE(received_msg.has_imu_data()) << "Received message should contain IMU data";

  const auto& received_imu = received_msg.imu_data();
  LOGI("Received IMU data: accel(%.1f, %.1f, %.1f), gyro(%.1f, %.1f, %.1f), mag(%.1f, %.1f, %.1f)",
       received_imu.accel_x(), received_imu.accel_y(), received_imu.accel_z(), received_imu.gyro_x(),
       received_imu.gyro_y(), received_imu.gyro_z(), received_imu.mag_x(), received_imu.mag_y(), received_imu.mag_z());

  // Verify IMU data matches what we sent
  EXPECT_NEAR(1.1, received_imu.accel_x(), 0.01) << "Accel X should match";
  EXPECT_NEAR(2.2, received_imu.accel_y(), 0.01) << "Accel Y should match";
  EXPECT_NEAR(3.3, received_imu.accel_z(), 0.01) << "Accel Z should match";
  EXPECT_NEAR(4.4, received_imu.gyro_x(), 0.01) << "Gyro X should match";
  EXPECT_NEAR(5.5, received_imu.gyro_y(), 0.01) << "Gyro Y should match";
  EXPECT_NEAR(6.6, received_imu.gyro_z(), 0.01) << "Gyro Z should match";
  EXPECT_NEAR(0.1, received_imu.mag_x(), 0.01) << "Mag X should match";
  EXPECT_NEAR(0.2, received_imu.mag_y(), 0.01) << "Mag Y should match";
  EXPECT_NEAR(0.3, received_imu.mag_z(), 0.01) << "Mag Z should match";

  LOGI("Test completed successfully, stopping AdasApp");
  app.stop();
  LOGI("Test finished");
}
