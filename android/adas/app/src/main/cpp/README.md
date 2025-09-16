# ADAS C++ Application

Advanced Driver Assistance System (ADAS) application built with ServiceManager framework for Android.

## Project Structure

```
cpp/
├── docs/                           # Documentation
│   └── SENSOR_TOPICS.md           # Internal sensor topics reference
│
├── framework/                      # Core framework
│   └── service_manager.hpp        # Service management framework (1042 lines)
│
├── services/                       # Application services
│   ├── panda_service.h/cpp        # CAN data from Panda device (117 lines)
│   ├── sensor_reader_service.h/cpp # Sensor data processing (167 lines)
│   └── zmq_bridge_service.h/cpp   # External ZMQ to internal bridge (132 lines)
│
├── examples/                       # Example implementations
│   └── example_sensor_consumer_service.h/cpp  # Example consumer (155 lines)
│
├── tests/                          # Unit tests
│   ├── test_service_manager.cpp   # ServiceManager tests (10 tests, 947 lines)
│   ├── test_zmq_imu.cpp          # ZMQ integration test
│   ├── test_utils.h/cpp          # Test utilities
│   └── main.cpp                  # Test runner
│
├── panda/                          # Panda device library
│   ├── panda.h/cc                # Panda device interface
│   └── panda_comms.h/cc          # USB communications
│
├── utils/                          # Utility tools
│   ├── can_parser.cc             # CAN message parser
│   ├── log_parser.cc             # Log file parser
│   └── steering_control.cc       # Steering control utilities
│
├── adas_app.h/cpp                 # Main application class (86 lines)
├── adas_app_android.cpp           # Android platform implementation
├── adas_app_linux.cpp             # Linux platform implementation
├── logger.h                       # Logging utilities
├── protobuf_utils.h/cpp          # Protobuf helpers
├── CMakeLists.txt                # Build configuration
├── vcpkg.json                    # Dependencies
└── build_cpp.sh                  # Build script

```

## Architecture

### Service-Based Design

The application uses a service-oriented architecture with the ServiceManager framework:

```
┌─────────────────────────────────────────────────────────────┐
│                    ServiceManager                           │
│  (ThreadPool: 3 workers, Priority scheduling, Statistics)  │
└─────────────────────────────────────────────────────────────┘
              │         │         │
    ┌─────────┘         │         └──────────┐
    │                   │                    │
    ▼                   ▼                    ▼
┌──────────┐    ┌──────────────┐    ┌────────────────┐
│  Panda   │    │  ZmqBridge   │    │ SensorReader   │
│ Service  │    │   Service    │    │    Service     │
└──────────┘    └──────────────┘    └────────────────┘
    │                   │                    │
    │ sensors/can       │ External ZMQ       │ sensors/*
    │                   │ (tcp://...)        │
    ▼                   ▼                    ▼
┌───────────────────────────────────────────────────────┐
│          Internal Topic Bus (Type-safe Pub/Sub)       │
│  • sensors/imu            • sensors/gps/location      │
│  • sensors/accelerometer  • sensors/gps/data          │
│  • sensors/gyroscope      • sensors/camera/state      │
│  • sensors/magnetometer   • sensors/camera/buffer     │
│  • sensors/can                                        │
└───────────────────────────────────────────────────────┘
                        │
                        ▼
            ┌───────────────────────┐
            │  Consumer Services     │
            │  (Your custom logic)   │
            └───────────────────────┘
```

### Data Flow

1. **External Sources** → Sensors (Android), Panda device
2. **ZmqBridgeService** → Polls external ZMQ topics (10ms timer)
3. **External Topics** → Raw data published to ZMQ (tcp://127.0.0.1:5555-5563)
4. **SensorReaderService** → Subscribes to external topics
5. **Internal Topics** → Republishes to `sensors/*` topics
6. **Consumer Services** → Subscribe to internal topics, process data
7. **PandaService** → Reads CAN data, publishes to `sensors/can`

## Building

### Android (ARM64)
```bash
./build_cpp.sh -t android
# Output: build/libadas_app.so (64MB)
# Copies to: ../libs/arm64-v8a/
```

### Linux (x86_64)
```bash
./build_cpp.sh -t linux
# Output: build/libadas_app.so
```

### With Tests
```bash
./build_cpp.sh -t linux --test
# Runs: 10 ServiceManager tests + 1 ZMQ integration test
```

### Clean Build
```bash
./build_cpp.sh -c -t android  # Clean + Android
./build_cpp.sh -c -t linux    # Clean + Linux
```

## Services

### PandaService
- **Priority**: High
- **Timer**: 50ms (20Hz)
- **Function**: Reads CAN data from Panda device
- **Publishes**: `sensors/can` with filtered CAN frames
- **Filters**: Only addresses: 0xFC, 0x86, 0xFD, 0x3DC, 0x13D

### ZmqBridgeService
- **Priority**: High
- **Timer**: 10ms (100Hz)
- **Function**: Bridges external ZMQ to internal topics
- **Subscribes**: 8 external ZMQ endpoints
- **Publishes**: Raw messages to internal topics

### SensorReaderService
- **Priority**: Normal
- **Function**: Processes and republishes sensor data
- **Subscribes**: External ZMQ topics (imuData, gpsLocation, etc.)
- **Publishes**: 8 internal `sensors/*` topics
- **Features**: Logging, validation, republishing

## Internal Topics

See [docs/SENSOR_TOPICS.md](docs/SENSOR_TOPICS.md) for complete reference.

### Available Topics

| Topic | Data Type | Frequency | Source |
|-------|-----------|-----------|--------|
| `sensors/imu` | IMU Data | ~100Hz | Android IMU |
| `sensors/accelerometer` | Accel | ~100Hz | Android |
| `sensors/gyroscope` | Gyro | ~100Hz | Android |
| `sensors/magnetometer` | Mag | ~100Hz | Android |
| `sensors/gps/location` | Location | ~1Hz | Android GPS |
| `sensors/gps/data` | GPS Info | ~1Hz | Android GPS |
| `sensors/camera/state` | State | ~30Hz | Camera |
| `sensors/camera/buffer` | Frames | ~30Hz | Camera |
| `sensors/can` | CAN Frames | ~20Hz | Panda |

## Creating Custom Services

See `examples/example_sensor_consumer_service.cpp` for a complete example.

### Basic Template

```cpp
#include "framework/service_manager.hpp"
#include "messages.pb.h"

class MyService : public microros::Service
{
public:
    void configure() override {
        // Subscribe to topics
        subscribe<ai::flow::android::ZMQMessage>("sensors/imu",
            [this](const auto& msg) {
                if (msg.has_imu_data()) {
                    const auto& imu = msg.imu_data();
                    // Process IMU data
                    float accel = sqrt(
                        imu.accel_x() * imu.accel_x() +
                        imu.accel_y() * imu.accel_y() +
                        imu.accel_z() * imu.accel_z()
                    );
                    // Use acceleration...
                }
            });

        // Set priority
        setPriority(Priority::Normal);
    }

    void reset() override {
        // Reset state
    }
};
```

### Adding to AdasApp

Edit `adas_app.cpp`:

```cpp
void AdasApp::setupServices()
{
  // ... existing services ...

  // Add your custom service
  auto my_service = std::make_shared<MyService>();
  my_service->setPriority(microros::Service::Priority::Normal);

  std::vector<microros::ServicePtr> services = {
    panda_service_,
    zmq_bridge_service_,
    sensor_service_,
    my_service  // <-- Add here
  };

  service_manager_ = std::make_shared<microros::ServiceManager>(
    microros::ServiceManager::Mode::RealTime,
    services,
    microros::ServiceManager::ThreadingMode::ThreadPool,
    4  // Increase worker threads
  );
}
```

## Testing

### Run All Tests
```bash
cd build
./tests/adas_tests
```

### Run Specific Test
```bash
./tests/adas_tests --gtest_filter="ServiceManagerTest.InternalTopicPublishing"
```

### Test Coverage
- ✅ Service lifecycle (start, stop, reset)
- ✅ Pub/Sub functionality
- ✅ Timer scheduling
- ✅ Multi-hop message chains
- ✅ Service priorities
- ✅ Statistics collection
- ✅ Threading modes (OneThreadPerService, ThreadPool, SingleThreaded)
- ✅ Simulated mode (deterministic testing)
- ✅ Internal topic publishing
- ✅ Full ZMQ integration (External → Internal → Consumer)

## Performance

### Thread Configuration
- **Mode**: ThreadPool
- **Workers**: 3 threads
- **Scheduling**: Priority-based (Critical > High > Normal > Low)

### Measured Performance
- **Message processing**: <100μs per message
- **Timer accuracy**: ±1ms
- **ZMQ latency**: <10ms (external → internal)
- **Throughput**: >1000 messages/second

## Dependencies

Managed via `vcpkg.json`:
- **protobuf** 5.29.3 - Message serialization
- **cppzmq** 4.10.0 - ZMQ C++ bindings
- **libusb** 1.0.27 - Panda USB communication
- **gtest** 1.16.0 - Unit testing (Linux only)

## Platform Support

### Android
- **ABI**: arm64-v8a
- **Min SDK**: 26 (Android 8.0)
- **NDK**: 27.0.12077973
- **Threading**: Android priority (setpriority)
- **Logging**: Android logcat

### Linux
- **Arch**: x86_64
- **Threading**: SCHED_RR (requires CAP_SYS_NICE)
- **Logging**: stdout/stderr
- **Testing**: Full test suite

## Statistics & Monitoring

```cpp
// Get statistics for a service
auto stats = service_manager->getServiceStats(my_service);
if (stats) {
    LOGI("Messages: %lu", stats->messages_processed);
    LOGI("Timers: %lu", stats->timers_fired);
    LOGI("Exceptions: %lu", stats->exceptions_caught);
    LOGI("Avg time: %lu μs", stats->avg_processing_time_us);
}

// Print all statistics
service_manager->printStats();
```

## Troubleshooting

### Build Issues

**Problem**: `messaging/impl_zmq.h` not found
- **Solution**: Use `messages.pb.h` directly (already fixed)

**Problem**: Cannot link `-ludev`
- **Solution**: `sudo apt-get install libudev-dev`

**Problem**: Template errors in service_manager.hpp
- **Solution**: Ensure C++17 enabled and all headers included

### Runtime Issues

**Problem**: No messages received
- **Solution**: Check ZMQ port conflicts, increase wait times, verify topic names

**Problem**: High CPU usage
- **Solution**: Reduce timer frequencies, unsubscribe from unused topics

**Problem**: Messages dropped
- **Solution**: Increase worker thread count, optimize message handlers

## License

This project is part of the ADAS Android application.

## Contributing

When adding new services:
1. Create service files in `services/` directory
2. Inherit from `microros::Service`
3. Implement `configure()` and `reset()`
4. Add to CMakeLists.txt
5. Register in `adas_app.cpp::setupServices()`
6. Write tests in `tests/`

## Support

For questions or issues, refer to:
- `docs/SENSOR_TOPICS.md` - Topic reference
- `examples/example_sensor_consumer_service.cpp` - Usage examples
- `tests/test_service_manager.cpp` - Test examples
