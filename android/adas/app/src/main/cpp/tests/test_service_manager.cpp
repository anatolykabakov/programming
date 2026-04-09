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
#include "framework/service_manager.hpp"
#include "messages.pb.h"
#include "services/zmq_bridge_service.h"
#include "services/sensor_reader_service.h"

struct TestMessage {
  int value;
  std::string text;
  uint64_t timestamp;

  TestMessage() : value(0), timestamp(0) {}
  TestMessage(int v, const std::string& t)
    : value(v)
    , text(t)
    , timestamp(
          std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count())
  {
  }
};

struct CounterMessage {
  int counter;
  CounterMessage(int c = 0) : counter(c) {}
};

// Request-Response test structures
struct DataRequest {
  std::string query;
  int limit;

  DataRequest() : query(""), limit(0) {}
  DataRequest(const std::string& q, int l) : query(q), limit(l) {}
};

struct DataResponse {
  std::vector<std::string> data;
  bool success;
  std::string error_message;

  DataResponse() : success(false) {}
  DataResponse(const std::vector<std::string>& d, bool s, const std::string& e = "")
    : data(d), success(s), error_message(e)
  {
  }
};

class PublisherService : public microros::Service {
public:
  PublisherService() = default;

  void configure() override { LOGI("PublisherService: configured"); }

  void reset() override { LOGI("PublisherService: reset"); }

  void publishTestMessage(int value, const std::string& text)
  {
    TestMessage msg(value, text);
    publish("test_topic", msg);
    LOGI("PublisherService: published message (value=%d, text=%s)", value, text.c_str());
  }
};

// Request-Response test services
class DataServerService : public microros::Service {
public:
  DataServerService() = default;

  void configure() override
  {
    LOGI("DataServerService: configured");

    // Регистрируем обработчик запросов
    respond<DataRequest, DataResponse>("data/get", [this](const DataRequest& request) -> std::optional<DataResponse> {
      return this->handleDataRequest(request);
    });
  }

  void reset() override { LOGI("DataServerService: reset"); }

private:
  std::optional<DataResponse> handleDataRequest(const DataRequest& request)
  {
    LOGI("DataServerService: handling request (query=%s, limit=%d)", request.query.c_str(), request.limit);

    if (request.query == "sensors") {
      std::vector<std::string> data = {"sensor1", "sensor2", "sensor3"};
      if (request.limit > 0 && request.limit < data.size()) {
        data.resize(request.limit);
      }
      return DataResponse(data, true);
    } else if (request.query == "error") {
      return DataResponse({}, false, "Test error");
    } else {
      return std::nullopt;  // Нет ответа
    }
  }
};

class DataClientService : public microros::Service {
public:
  DataClientService() : response_received_(false), response_success_(false) {}

  void configure() override { LOGI("DataClientService: configured"); }

  void reset() override
  {
    LOGI("DataClientService: reset");
    response_received_ = false;
    response_success_ = false;
    response_data_.clear();
  }

  void requestData(const std::string& query, int limit = 0)
  {
    LOGI("DataClientService: requesting data (query=%s, limit=%d)", query.c_str(), limit);

    auto response =
        request<DataRequest, DataResponse>("data/get", DataRequest(query, limit), 3000);  // 3 секунды timeout

    if (response) {
      handleResponse(*response);
    } else {
      LOGI("DataClientService: no response received");
    }
  }

  bool isResponseReceived() const { return response_received_; }
  bool isResponseSuccessful() const { return response_success_; }
  const std::vector<std::string>& getResponseData() const { return response_data_; }

private:
  void handleResponse(const DataResponse& response)
  {
    LOGI("DataClientService: received response (success=%s, data_size=%zu)", response.success ? "true" : "false",
         response.data.size());

    response_received_ = true;
    response_success_ = response.success;
    response_data_ = response.data;
  }

  bool response_received_;
  bool response_success_;
  std::vector<std::string> response_data_;
};

class SubscriberService : public microros::Service {
public:
  SubscriberService() = default;

  void configure() override
  {
    subscribe<TestMessage>("test_topic", [this](const TestMessage& msg) { handleTestMessage(msg); });
    LOGI("SubscriberService: configured and subscribed to test_topic");
  }

  void reset() override
  {
    LOGI("SubscriberService: reset");
    std::lock_guard<std::mutex> lock(mutex_);
    received_messages_.clear();
  }

  void handleTestMessage(const TestMessage& msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    received_messages_.push_back(msg);
    LOGI("SubscriberService: received message (value=%d, text=%s)", msg.value, msg.text.c_str());
  }

  size_t getReceivedCount() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return received_messages_.size();
  }

  std::vector<TestMessage> getReceivedMessages() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return received_messages_;
  }

private:
  mutable std::mutex mutex_;
  std::vector<TestMessage> received_messages_;
};

// Timer service that fires periodically
class TimerService : public microros::Service {
public:
  TimerService() : timer_count_(0) {}

  void configure() override
  {
    // Schedule a timer every 10ms
    scheduleTimer(10000, [this]() { onTimer(); });
    LOGI("TimerService: configured with 10ms timer");
  }

  void reset() override
  {
    LOGI("TimerService: reset");
    timer_count_ = 0;
  }

  void onTimer()
  {
    timer_count_++;
    LOGI("TimerService: timer fired (count=%d)", timer_count_.load());
  }

  int getTimerCount() const { return timer_count_.load(); }

private:
  std::atomic<int> timer_count_;
};

// Counter service for pub/sub chain testing
class CounterService : public microros::Service {
public:
  CounterService(const std::string& input_topic, const std::string& output_topic)
    : input_topic_(input_topic), output_topic_(output_topic), counter_(0)
  {
  }

  void configure() override
  {
    subscribe<CounterMessage>(input_topic_, [this](const CounterMessage& msg) { handleInput(msg); });
    LOGI("CounterService: configured (input=%s, output=%s)", input_topic_.c_str(), output_topic_.c_str());
  }

  void reset() override { counter_ = 0; }

  void handleInput(const CounterMessage& msg)
  {
    counter_++;
    CounterMessage output(msg.counter + 1);
    publish(output_topic_, output);
    LOGI("CounterService: received %d, sent %d (total received: %d)", msg.counter, output.counter, counter_.load());
  }

  // Public method for testing
  void publishMessage(const std::string& topic, const CounterMessage& msg) { publish(topic, msg); }

  int getCounter() const { return counter_.load(); }

private:
  std::string input_topic_;
  std::string output_topic_;
  std::atomic<int> counter_;
};

TEST(ServiceManagerTest, BasicServiceCreation)
{
  LOGI("Test: BasicServiceCreation - START");

  auto pub_service = std::make_shared<PublisherService>();
  auto sub_service = std::make_shared<SubscriberService>();

  std::vector<microros::ServicePtr> services = {pub_service, sub_service};

  auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                            microros::ServiceManager::ThreadingMode::ThreadPool, 2);

  EXPECT_EQ(2, manager->getServiceCount());
  EXPECT_EQ(0, manager->getRunningCount());

  size_t started = manager->startAll();
  EXPECT_EQ(2, started);
  EXPECT_EQ(2, manager->getRunningCount());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  size_t stopped = manager->stopAll();
  EXPECT_EQ(2, stopped);
  EXPECT_EQ(0, manager->getRunningCount());

  LOGI("Test: BasicServiceCreation - PASSED");
}

TEST(ServiceManagerTest, PubSubFunctionality)
{
  LOGI("Test: PubSubFunctionality - START");

  auto pub_service = std::make_shared<PublisherService>();
  auto sub_service = std::make_shared<SubscriberService>();

  std::vector<microros::ServicePtr> services = {pub_service, sub_service};

  auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                            microros::ServiceManager::ThreadingMode::ThreadPool, 2);

  manager->startAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Publish some messages
  pub_service->publishTestMessage(1, "Hello");
  pub_service->publishTestMessage(2, "World");
  pub_service->publishTestMessage(3, "Test");

  // Give time for messages to be processed
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Verify messages were received
  EXPECT_EQ(3, sub_service->getReceivedCount());

  auto messages = sub_service->getReceivedMessages();
  ASSERT_EQ(3, messages.size());

  EXPECT_EQ(1, messages[0].value);
  EXPECT_EQ("Hello", messages[0].text);

  EXPECT_EQ(2, messages[1].value);
  EXPECT_EQ("World", messages[1].text);

  EXPECT_EQ(3, messages[2].value);
  EXPECT_EQ("Test", messages[2].text);

  manager->stopAll();

  LOGI("Test: PubSubFunctionality - PASSED");
}

TEST(ServiceManagerTest, TimerFunctionality)
{
  LOGI("Test: TimerFunctionality - START");

  auto timer_service = std::make_shared<TimerService>();

  std::vector<microros::ServicePtr> services = {timer_service};

  auto manager = std::make_shared<microros::ServiceManager>(
      microros::ServiceManager::Mode::RealTime, services, microros::ServiceManager::ThreadingMode::OneThreadPerService);

  manager->startAll();

  // Wait for timers to fire multiple times (10ms timer, wait 100ms = ~10 times)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  int timer_count = timer_service->getTimerCount();
  LOGI("Timer fired %d times in 100ms", timer_count);

  // Should have fired at least 8 times (allowing for some timing variation)
  EXPECT_GE(timer_count, 8);
  EXPECT_LE(timer_count, 12);

  manager->stopAll();

  LOGI("Test: TimerFunctionality - PASSED");
}

TEST(ServiceManagerTest, PubSubChain)
{
  LOGI("Test: PubSubChain - START");

  auto counter1 = std::make_shared<CounterService>("input", "middle");
  auto counter2 = std::make_shared<CounterService>("middle", "output");
  auto subscriber = std::make_shared<SubscriberService>();

  // Manually subscribe the subscriber to "output" topic
  class OutputSubscriber : public microros::Service {
  public:
    OutputSubscriber() : received_value_(-1) {}

    void configure() override
    {
      subscribe<CounterMessage>("output", [this](const CounterMessage& msg) {
        received_value_ = msg.counter;
        LOGI("OutputSubscriber: received counter=%d", msg.counter);
      });
    }

    void reset() override { received_value_ = -1; }

    int getReceivedValue() const { return received_value_.load(); }

  private:
    std::atomic<int> received_value_;
  };

  auto output_sub = std::make_shared<OutputSubscriber>();

  std::vector<microros::ServicePtr> services = {counter1, counter2, output_sub};

  auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                            microros::ServiceManager::ThreadingMode::ThreadPool, 3);

  manager->startAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Publish initial message with counter=0
  counter1->publishMessage("input", CounterMessage(0));

  // Give time for message to propagate through chain
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Verify chain: input(0) -> counter1 -> middle(1) -> counter2 -> output(2)
  EXPECT_EQ(1, counter1->getCounter());
  EXPECT_EQ(1, counter2->getCounter());
  EXPECT_EQ(2, output_sub->getReceivedValue());

  manager->stopAll();

  LOGI("Test: PubSubChain - PASSED");
}

// Test 5: Service priority
TEST(ServiceManagerTest, ServicePriority)
{
  LOGI("Test: ServicePriority - START");

  auto high_priority_service = std::make_shared<TimerService>();
  auto normal_priority_service = std::make_shared<TimerService>();
  auto low_priority_service = std::make_shared<TimerService>();

  high_priority_service->setPriority(microros::Service::Priority::High);
  normal_priority_service->setPriority(microros::Service::Priority::Normal);
  low_priority_service->setPriority(microros::Service::Priority::Low);

  EXPECT_EQ(microros::Service::Priority::High, high_priority_service->getPriority());
  EXPECT_EQ(microros::Service::Priority::Normal, normal_priority_service->getPriority());
  EXPECT_EQ(microros::Service::Priority::Low, low_priority_service->getPriority());

  std::vector<microros::ServicePtr> services = {high_priority_service, normal_priority_service, low_priority_service};

  auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                            microros::ServiceManager::ThreadingMode::ThreadPool, 2);

  manager->startAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  int high_count = high_priority_service->getTimerCount();
  int normal_count = normal_priority_service->getTimerCount();
  int low_count = low_priority_service->getTimerCount();

  LOGI("Timer counts: High=%d, Normal=%d, Low=%d", high_count, normal_count, low_count);

  // High priority should fire more or equal times than normal
  EXPECT_GE(high_count, normal_count);

  // Normal should fire more or equal times than low
  EXPECT_GE(normal_count, low_count);

  manager->stopAll();

  LOGI("Test: ServicePriority - PASSED");
}

TEST(ServiceManagerTest, Statistics)
{
  LOGI("Test: Statistics - START");

  auto pub_service = std::make_shared<PublisherService>();
  auto sub_service = std::make_shared<SubscriberService>();

  std::vector<microros::ServicePtr> services = {pub_service, sub_service};

  auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                            microros::ServiceManager::ThreadingMode::ThreadPool, 2);

  manager->startAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Send multiple messages
  for (int i = 0; i < 10; i++) {
    pub_service->publishTestMessage(i, "Message " + std::to_string(i));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Get statistics
  auto sub_stats = manager->getServiceStats(sub_service);
  ASSERT_TRUE(sub_stats.has_value());

  LOGI("Subscriber statistics: messages=%lu, timers=%lu, exceptions=%lu", sub_stats->messages_processed,
       sub_stats->timers_fired, sub_stats->exceptions_caught);

  EXPECT_GE(sub_stats->messages_processed, 10);
  EXPECT_EQ(0, sub_stats->timers_fired);
  EXPECT_EQ(0, sub_stats->exceptions_caught);

  // Print all statistics
  manager->printStats();

  manager->stopAll();

  LOGI("Test: Statistics - PASSED");
}

// Test 7: Threading modes
TEST(ServiceManagerTest, ThreadingModes)
{
  LOGI("Test: ThreadingModes - START");

  auto pub_service = std::make_shared<PublisherService>();
  auto sub_service = std::make_shared<SubscriberService>();

  // Test 1: OneThreadPerService
  {
    std::vector<microros::ServicePtr> services = {pub_service, sub_service};

    auto manager =
        std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                   microros::ServiceManager::ThreadingMode::OneThreadPerService);

    manager->startAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    pub_service->publishTestMessage(1, "OneThreadPerService");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(1, sub_service->getReceivedCount());

    manager->stopAll();
    sub_service->reset();
  }

  // Test 2: ThreadPool
  {
    std::vector<microros::ServicePtr> services = {pub_service, sub_service};

    auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                              microros::ServiceManager::ThreadingMode::ThreadPool, 2);

    manager->startAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    pub_service->publishTestMessage(2, "ThreadPool");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(1, sub_service->getReceivedCount());

    manager->stopAll();
    sub_service->reset();
  }

  // Test 3: SingleThreaded
  {
    std::vector<microros::ServicePtr> services = {pub_service, sub_service};

    auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                              microros::ServiceManager::ThreadingMode::SingleThreaded);

    manager->startAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    pub_service->publishTestMessage(3, "SingleThreaded");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(1, sub_service->getReceivedCount());

    manager->stopAll();
  }

  LOGI("Test: ThreadingModes - PASSED");
}

// Test 8: Simulated mode with manual time control
TEST(ServiceManagerTest, SimulatedMode)
{
  LOGI("Test: SimulatedMode - START");

  auto pub_service = std::make_shared<PublisherService>();
  auto sub_service = std::make_shared<SubscriberService>();

  std::vector<microros::ServicePtr> services = {pub_service, sub_service};

  auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::Simulated, services);

  // In simulated mode, manually control time
  manager->setTime(0);

  // Publish message
  pub_service->publishTestMessage(1, "Simulated");

  // Process messages with step()
  manager->step();

  // Verify message was received
  EXPECT_EQ(1, sub_service->getReceivedCount());

  auto messages = sub_service->getReceivedMessages();
  ASSERT_EQ(1, messages.size());
  EXPECT_EQ(1, messages[0].value);
  EXPECT_EQ("Simulated", messages[0].text);

  LOGI("Test: SimulatedMode - PASSED");
}

// Test 9: Internal topic publishing - sensor data flow
TEST(ServiceManagerTest, InternalTopicPublishing)
{
  LOGI("Test: InternalTopicPublishing - START");

  // Create a mock sensor publisher service
  class MockSensorPublisher : public microros::Service {
  public:
    void configure() override { LOGI("MockSensorPublisher: configured"); }

    void reset() override {}

    void publishMockImuData()
    {
      ai::flow::android::ZMQMessage msg;
      msg.set_topic("imuData");
      msg.set_timestamp(123456789);

      auto* imu = msg.mutable_imu_data();
      imu->set_accel_x(1.5f);
      imu->set_accel_y(2.5f);
      imu->set_accel_z(9.8f);
      imu->set_gyro_x(0.1f);
      imu->set_gyro_y(0.2f);
      imu->set_gyro_z(0.3f);
      imu->set_mag_x(25.0f);
      imu->set_mag_y(30.0f);
      imu->set_mag_z(35.0f);
      imu->set_timestamp(123456789);

      // Publish to internal topic (simulating SensorReaderService behavior)
      publish("sensors/imu", msg);
      LOGI("MockSensorPublisher: published IMU data to sensors/imu");
    }

    void publishMockCanData()
    {
      ai::flow::android::ZMQMessage msg;
      msg.set_topic("pandaData");
      msg.set_timestamp(987654321);

      auto* can = msg.mutable_can_data();
      can->set_timestamp_us(987654321);

      // Add a CAN frame (vehicle speed)
      auto* frame = can->add_frames();
      frame->set_address(0xFD);  // ESP_21 - vehicle speed
      frame->set_data("\x12\x34\x56\x78", 4);
      frame->set_bus_time(987654321);
      frame->set_src(0);

      // Publish to internal topic (simulating PandaService behavior)
      publish("sensors/can", msg);
      LOGI("MockSensorPublisher: published CAN data to sensors/can");
    }
  };

  // Create a consumer service that subscribes to internal topics
  class InternalTopicConsumer : public microros::Service {
  public:
    InternalTopicConsumer() : imu_count_(0), can_count_(0), last_accel_z_(0.0f), last_can_address_(0) {}

    void configure() override
    {
      // Subscribe to internal sensor topics
      subscribe<ai::flow::android::ZMQMessage>("sensors/imu",
                                               [this](const ai::flow::android::ZMQMessage& msg) { handleImu(msg); });

      subscribe<ai::flow::android::ZMQMessage>("sensors/can",
                                               [this](const ai::flow::android::ZMQMessage& msg) { handleCan(msg); });

      LOGI("InternalTopicConsumer: configured with subscriptions");
    }

    void reset() override
    {
      imu_count_ = 0;
      can_count_ = 0;
      last_accel_z_ = 0.0f;
      last_can_address_ = 0;
    }

    void handleImu(const ai::flow::android::ZMQMessage& msg)
    {
      imu_count_++;
      if (msg.has_imu_data()) {
        const auto& imu = msg.imu_data();
        last_accel_z_ = imu.accel_z();
        LOGI("InternalTopicConsumer: received IMU #%d, accel_z=%.2f", imu_count_.load(), last_accel_z_);
      }
    }

    void handleCan(const ai::flow::android::ZMQMessage& msg)
    {
      can_count_++;
      if (msg.has_can_data()) {
        const auto& can = msg.can_data();
        if (can.frames_size() > 0) {
          last_can_address_ = can.frames(0).address();
          LOGI("InternalTopicConsumer: received CAN #%d, address=0x%X", can_count_.load(), last_can_address_);
        }
      }
    }

    int getImuCount() const { return imu_count_.load(); }
    int getCanCount() const { return can_count_.load(); }
    float getLastAccelZ() const { return last_accel_z_; }
    uint32_t getLastCanAddress() const { return last_can_address_; }

  private:
    std::atomic<int> imu_count_;
    std::atomic<int> can_count_;
    float last_accel_z_;
    uint32_t last_can_address_;
  };

  // Create services
  auto publisher = std::make_shared<MockSensorPublisher>();
  auto consumer = std::make_shared<InternalTopicConsumer>();

  std::vector<microros::ServicePtr> services = {publisher, consumer};

  auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                            microros::ServiceManager::ThreadingMode::ThreadPool, 2);

  // Start services
  manager->startAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Publish mock IMU data
  publisher->publishMockImuData();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Verify IMU data was received
  EXPECT_EQ(1, consumer->getImuCount());
  EXPECT_NEAR(9.8f, consumer->getLastAccelZ(), 0.01f);

  // Publish mock CAN data
  publisher->publishMockCanData();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Verify CAN data was received
  EXPECT_EQ(1, consumer->getCanCount());
  EXPECT_EQ(0xFD, consumer->getLastCanAddress());

  // Publish multiple messages
  for (int i = 0; i < 5; i++) {
    publisher->publishMockImuData();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Verify multiple messages received
  EXPECT_EQ(6, consumer->getImuCount());  // 1 + 5 = 6

  // Print statistics
  manager->printStats();

  manager->stopAll();

  LOGI("Test: InternalTopicPublishing - PASSED");
}

// Test 10: Full integration - External ZMQ -> ZmqBridge -> SensorReader -> Internal Topics -> Consumer
TEST(ServiceManagerTest, FullZmqIntegration)
{
  LOGI("Test: FullZmqIntegration - START");

  // Create a consumer service that tracks received messages
  class SensorDataConsumer : public microros::Service {
  public:
    SensorDataConsumer() : imu_received_(false), gps_received_(false), accel_received_(false), last_accel_z_(0.0f) {}

    void configure() override
    {
      // Subscribe to internal sensor topics
      subscribe<ai::flow::android::ZMQMessage>("sensors/imu", [this](const ai::flow::android::ZMQMessage& msg) {
        if (msg.has_imu_data()) {
          const auto& imu = msg.imu_data();
          last_accel_z_ = imu.accel_z();
          imu_received_ = true;
          LOGI("SensorDataConsumer: IMU received, accel_z=%.2f", last_accel_z_);
        }
      });

      subscribe<ai::flow::android::ZMQMessage>("sensors/accelerometer",
                                               [this](const ai::flow::android::ZMQMessage& msg) {
                                                 if (msg.has_accelerometer_data()) {
                                                   accel_received_ = true;
                                                   LOGI("SensorDataConsumer: Accelerometer received");
                                                 }
                                               });

      subscribe<ai::flow::android::ZMQMessage>("sensors/gps/location",
                                               [this](const ai::flow::android::ZMQMessage& msg) {
                                                 gps_received_ = true;
                                                 LOGI("SensorDataConsumer: GPS location received");
                                               });

      LOGI("SensorDataConsumer: configured");
    }

    void reset() override
    {
      imu_received_ = false;
      gps_received_ = false;
      accel_received_ = false;
      last_accel_z_ = 0.0f;
    }

    bool isImuReceived() const { return imu_received_.load(); }
    bool isGpsReceived() const { return gps_received_.load(); }
    bool isAccelReceived() const { return accel_received_.load(); }
    float getLastAccelZ() const { return last_accel_z_; }

  private:
    std::atomic<bool> imu_received_;
    std::atomic<bool> gps_received_;
    std::atomic<bool> accel_received_;
    float last_accel_z_;
  };

  // Create external ZMQ publishers FIRST (before services start)
  zmq::context_t zmq_context(1);

  // Use test ports (60000+) to avoid conflicts with running services
  const int TEST_IMU_PORT = 60558;
  const int TEST_ACCEL_PORT = 60560;
  const int TEST_GPS_PORT = 60557;

  // IMU publisher
  zmq::socket_t imu_pub(zmq_context, ZMQ_PUB);
  imu_pub.bind("tcp://127.0.0.1:" + std::to_string(TEST_IMU_PORT));
  LOGI("External IMU publisher bound to tcp://127.0.0.1:%d", TEST_IMU_PORT);

  // Accelerometer publisher
  zmq::socket_t accel_pub(zmq_context, ZMQ_PUB);
  accel_pub.bind("tcp://127.0.0.1:" + std::to_string(TEST_ACCEL_PORT));
  LOGI("External Accelerometer publisher bound to tcp://127.0.0.1:%d", TEST_ACCEL_PORT);

  // GPS publisher
  zmq::socket_t gps_pub(zmq_context, ZMQ_PUB);
  gps_pub.bind("tcp://127.0.0.1:" + std::to_string(TEST_GPS_PORT));
  LOGI("External GPS publisher bound to tcp://127.0.0.1:%d", TEST_GPS_PORT);

  // Give ZMQ publishers time to bind
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Now create services with custom port mapping using internal topic names
  std::map<std::string, std::string> test_topics = {
      {"sensors/imu", "tcp://127.0.0.1:" + std::to_string(TEST_IMU_PORT)},
      {"sensors/accelerometer", "tcp://127.0.0.1:" + std::to_string(TEST_ACCEL_PORT)},
      {"sensors/gps/location", "tcp://127.0.0.1:" + std::to_string(TEST_GPS_PORT)},
  };

  auto zmq_bridge = std::make_shared<ZmqBridgeService>(test_topics);
  auto sensor_reader = std::make_shared<SensorReaderService>();
  auto consumer = std::make_shared<SensorDataConsumer>();

  std::vector<microros::ServicePtr> services = {zmq_bridge, sensor_reader, consumer};

  auto manager = std::make_shared<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services,
                                                            microros::ServiceManager::ThreadingMode::ThreadPool, 3);

  // Start services
  manager->startAll();
  LOGI("Services started, waiting for ZMQ connections to stabilize...");
  // ZMQ subscribers need time to connect to publishers
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  LOGI("Sending test messages...");

  // Create and send IMU data
  {
    ai::flow::android::ZMQMessage imu_msg;
    imu_msg.set_topic("sensors/imu");
    imu_msg.set_timestamp(111111111);

    auto* imu = imu_msg.mutable_imu_data();
    imu->set_accel_x(0.5f);
    imu->set_accel_y(1.5f);
    imu->set_accel_z(9.81f);
    imu->set_gyro_x(0.01f);
    imu->set_gyro_y(0.02f);
    imu->set_gyro_z(0.03f);
    imu->set_mag_x(20.0f);
    imu->set_mag_y(25.0f);
    imu->set_mag_z(30.0f);
    imu->set_timestamp(111111111);

    std::string serialized;
    imu_msg.SerializeToString(&serialized);
    imu_pub.send(zmq::buffer(serialized), zmq::send_flags::none);
    LOGI("Sent IMU message via external ZMQ");
  }

  {
    ai::flow::android::ZMQMessage accel_msg;
    accel_msg.set_topic("sensors/accelerometer");
    accel_msg.set_timestamp(222222222);

    auto* accel = accel_msg.mutable_accelerometer_data();
    accel->set_x(1.0f);
    accel->set_y(2.0f);
    accel->set_z(9.8f);
    accel->set_accuracy(3);
    accel->set_is_calibrated(true);
    accel->set_timestamp(222222222);

    std::string serialized;
    accel_msg.SerializeToString(&serialized);
    accel_pub.send(zmq::buffer(serialized), zmq::send_flags::none);
    LOGI("Sent Accelerometer message via external ZMQ");
  }

  // Create and send GPS location data
  {
    ai::flow::android::ZMQMessage gps_msg;
    gps_msg.set_topic("sensors/gps/location");
    gps_msg.set_timestamp(333333333);

    std::string serialized;
    gps_msg.SerializeToString(&serialized);
    gps_pub.send(zmq::buffer(serialized), zmq::send_flags::none);
    LOGI("Sent GPS location message via external ZMQ");
  }

  // Give time for messages to flow through the entire chain:
  // External ZMQ -> ZmqBridge -> Internal Topic -> SensorReader -> Internal sensors/* -> Consumer
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Verify all messages were received by consumer
  EXPECT_TRUE(consumer->isImuReceived()) << "IMU data should have been received";
  EXPECT_TRUE(consumer->isAccelReceived()) << "Accelerometer data should have been received";
  EXPECT_TRUE(consumer->isGpsReceived()) << "GPS data should have been received";

  // Verify IMU data values
  EXPECT_NEAR(9.81f, consumer->getLastAccelZ(), 0.01f) << "IMU accel_z should match";

  // Print final statistics
  LOGI("=== Final Test Statistics ===");
  LOGI("IMU received: %s", consumer->isImuReceived() ? "YES" : "NO");
  LOGI("Accel received: %s", consumer->isAccelReceived() ? "YES" : "NO");
  LOGI("GPS received: %s", consumer->isGpsReceived() ? "YES" : "NO");
  manager->printStats();

  // Cleanup
  manager->stopAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  LOGI("Test: FullZmqIntegration - PASSED");
}

// ========== Request-Response Tests ==========

TEST(ServiceManagerTest, RequestResponseBasic)
{
  LOGI("Test: RequestResponseBasic - START");

  // Create services
  auto server = std::make_shared<DataServerService>();
  auto client = std::make_shared<DataClientService>();
  std::vector<microros::ServicePtr> services = {server, client};
  auto manager = std::make_unique<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services);

  // Start manager
  manager->startAll();

  // Give services time to configure
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send request
  client->requestData("sensors", 2);

  // Wait for response
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Verify response
  EXPECT_TRUE(client->isResponseReceived()) << "Response should have been received";
  EXPECT_TRUE(client->isResponseSuccessful()) << "Response should be successful";

  auto data = client->getResponseData();
  EXPECT_EQ(data.size(), 2) << "Should return 2 items as requested";
  EXPECT_EQ(data[0], "sensor1") << "First item should be sensor1";
  EXPECT_EQ(data[1], "sensor2") << "Second item should be sensor2";

  // Cleanup
  manager->stopAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  LOGI("Test: RequestResponseBasic - PASSED");
}

TEST(ServiceManagerTest, RequestResponseError)
{
  LOGI("Test: RequestResponseError - START");

  // Create services
  auto server = std::make_shared<DataServerService>();
  auto client = std::make_shared<DataClientService>();
  std::vector<microros::ServicePtr> services = {server, client};
  auto manager = std::make_unique<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services);

  // Start manager
  manager->startAll();

  // Give services time to configure
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send error request
  client->requestData("error");

  // Wait for response
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Verify error response
  EXPECT_TRUE(client->isResponseReceived()) << "Response should have been received";
  EXPECT_FALSE(client->isResponseSuccessful()) << "Response should be error";

  // Cleanup
  manager->stopAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  LOGI("Test: RequestResponseError - PASSED");
}

TEST(ServiceManagerTest, RequestResponseTimeout)
{
  LOGI("Test: RequestResponseTimeout - START");

  // Create only client (no server)
  auto client = std::make_shared<DataClientService>();
  std::vector<microros::ServicePtr> services = {client};
  auto manager = std::make_unique<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services);

  // Start manager
  manager->startAll();

  // Give service time to configure
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send request (will timeout since no server)
  client->requestData("sensors");

  // Wait longer than timeout (3 seconds + buffer)
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));

  // Verify no response received
  EXPECT_FALSE(client->isResponseReceived()) << "Response should not have been received (timeout)";

  // Cleanup
  manager->stopAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  LOGI("Test: RequestResponseTimeout - PASSED");
}

TEST(ServiceManagerTest, RequestResponseMultipleClients)
{
  LOGI("Test: RequestResponseMultipleClients - START");

  // Create services
  auto server = std::make_shared<DataServerService>();
  auto client1 = std::make_shared<DataClientService>();
  auto client2 = std::make_shared<DataClientService>();
  std::vector<microros::ServicePtr> services = {server, client1, client2};
  auto manager = std::make_unique<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services);

  // Start manager
  manager->startAll();

  // Give services time to configure
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send requests from both clients
  client1->requestData("sensors", 1);
  client2->requestData("sensors", 2);

  // Wait for responses
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Verify both clients received responses
  EXPECT_TRUE(client1->isResponseReceived()) << "Client1 should have received response";
  EXPECT_TRUE(client2->isResponseReceived()) << "Client2 should have received response";

  EXPECT_TRUE(client1->isResponseSuccessful()) << "Client1 response should be successful";
  EXPECT_TRUE(client2->isResponseSuccessful()) << "Client2 response should be successful";

  auto data1 = client1->getResponseData();
  auto data2 = client2->getResponseData();

  EXPECT_EQ(data1.size(), 1) << "Client1 should get 1 item";
  EXPECT_EQ(data2.size(), 2) << "Client2 should get 2 items";

  // Cleanup
  manager->stopAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  LOGI("Test: RequestResponseMultipleClients - PASSED");
}

TEST(ServiceManagerTest, RequestResponseNoResponse)
{
  LOGI("Test: RequestResponseNoResponse - START");

  // Create services
  auto server = std::make_shared<DataServerService>();
  auto client = std::make_shared<DataClientService>();
  std::vector<microros::ServicePtr> services = {server, client};
  auto manager = std::make_unique<microros::ServiceManager>(microros::ServiceManager::Mode::RealTime, services);

  // Start manager
  manager->startAll();

  // Give services time to configure
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send request for unknown query (server returns nullopt)
  client->requestData("unknown");

  // Wait for timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));

  // Verify no response received
  EXPECT_FALSE(client->isResponseReceived()) << "Response should not have been received (no handler)";

  // Cleanup
  manager->stopAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  LOGI("Test: RequestResponseNoResponse - PASSED");
}
