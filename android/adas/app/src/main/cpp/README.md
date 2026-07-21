# ADAS C++ Application

Advanced Driver Assistance System (ADAS) application built with ServiceManager framework for Android.

## Project Structure

```
cpp/
├── include/                        # Public headers
│   ├── adas_app.h
│   ├── framework/service_manager.hpp
│   ├── panda/                      # Panda USB / CAN headers
│   ├── services/                   # Service headers
│   ├── utils/
│   └── volkswagen/                 # MQB CarController / mqbcan
│
├── src/                            # Implementation
│   ├── CMakeLists.txt
│   ├── adas_app.cpp
│   ├── adas_app_android.cpp
│   ├── adas_app_linux.cpp
│   ├── panda/
│   ├── services/
│   ├── utils/
│   └── volkswagen/
│
├── scripts/
│   └── build_cpp.sh                # Conan + CMake build
│
├── tests/                          # Unit tests
├── profiles/                       # Conan profiles
├── CMakeLists.txt
└── conanfile.py
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
│  Panda   │    │  ZmqBridge   │    │ TopicConvert / │
│ Service  │    │   Service    │    │ LaneKeep / …   │
└──────────┘    └──────────────┘    └────────────────┘
    │                   │                    │
    │ sensors/can       │ External ZMQ       │ typed topics
    │                   │ (tcp://...)        │
    ▼                   ▼                    ▼
┌───────────────────────────────────────────────────────┐
│          Internal Topic Bus (Type-safe Pub/Sub)       │
│  • sensors/imu            • sensors/gps/location      │
│  • sensors/imu_raw        • sensors/imu_yaw           │
│  • vehicle/chassis        • vision/path               │
│  • control/lane_keep      • localization/pose         │
│  • sensors/can                                        │
└───────────────────────────────────────────────────────┘
```

### Data Flow

1. **External Sources** → Sensors (Android), Panda device
2. **ZmqBridgeService** → Polls external ZMQ topics (10ms timer)
3. **External Topics** → Raw data published to ZMQ (`:5555` IN / `:5556` OUT, multipart topic+proto)
4. **TopicConvert / ImuCalib / LaneKeep / Localization** → Subscribe to internal topics, process data
5. **PandaService** → Reads CAN data, publishes to `sensors/can`; consumes `controls/steer`

## Building

### Android (ARM64)
```bash
./scripts/build_cpp.sh -t android
# Output: build/libadas_app.so (64MB)
# Copies to: ../libs/arm64-v8a/
```

### Linux (x86_64)
```bash
./scripts/build_cpp.sh -t linux
# Output: build/libadas_app.so
```

### With Tests
```bash
./scripts/build_cpp.sh -t linux --test
# Runs: 10 ServiceManager tests + 1 ZMQ integration test
```

### Clean Build
```bash
./scripts/build_cpp.sh -c -t android  # Clean + Android
./scripts/build_cpp.sh -c -t linux    # Clean + Linux
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
- **Function**: Bridges external ZMQ ↔ internal topics (one IN + one OUT socket)
- **IN** `tcp://127.0.0.1:5555`: bind SUB — sensors / commands, multipart `[topic][proto]`
- **OUT** `tcp://127.0.0.1:5556`: bind PUB — can / vehicle / algorithms → Java BagLogger
- **Publishes**: Raw messages to matching internal topics

### TopicConvertService
- **Priority**: High
- **Function**: ZMQ protobuf → typed samples (`vision/path`, `vehicle/chassis`, `imu_raw`, GPS ENU)
- **Subscribes**: `vision/lanes`, `vehicle/state`, `sensors/imu`, `sensors/gps/location`

## Internal Topics

See `include/utils/adas_topics.h` for the canonical list.

### Available Topics

| Topic | Data Type | Source |
|-------|-----------|--------|
| `sensors/imu` | IMU protobuf | Android ZMQ |
| `sensors/imu_raw` | RawImuSample | TopicConvert |
| `sensors/imu_yaw` | ImuSample | ImuCalib |
| `sensors/gps/location` | GPS / GpsSample ENU | Android → TopicConvert |
| `vision/lanes` | LaneLines | Android vision |
| `vision/path` | LanePathMsg | TopicConvert |
| `vehicle/state` | CarState | Android / bag |
| `vehicle/chassis` | ChassisSample | TopicConvert / Panda |
| `control/lane_keep` | LaneKeepState | LaneKeep |
| `controls/steer` | SteerCommand | LaneKeep → Panda |
| `localization/pose` | LocalizationPose | Localization |
| `sensors/can` | CAN Frames | Panda |

## Creating Custom Services

### Basic Template

```cpp
#include "framework/service_manager.hpp"
#include "messages.pb.h"

class MyService : public microros::Service
{
public:
    void configure() override {
        // Subscribe to topics
        subscribe<ai::flow::adas::ZMQMessage>("sensors/imu",
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

Edit `adas_app.cpp::setupRealtimeServices()` and push your service into the `services` vector (gated by `runtime_cfg_` if needed).

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
5. Register in `adas_app.cpp::setupRealtimeServices()`
6. Write tests in `tests/`

## Support

For questions or issues, refer to:
- `include/utils/adas_topics.h` - Topic reference
- `tests/test_service_manager.cpp` - Test examples
