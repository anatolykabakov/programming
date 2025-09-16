cmake_minimum_required(VERSION 3.17)
project(adas_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android build option
# Usage: cmake -DBUILD_FOR_ANDROID=ON ..
# This will link Android-specific libraries: android, log, libc, libm, libdl
option(BUILD_FOR_ANDROID "Build for Android platform" ON)

# Testing build option
# Usage: cmake -DBUILD_TESTING=ON ..
# This will build and enable tests (Linux only)
option(BUILD_TESTING "Build tests" OFF)

# Find libraries using vcpkg
find_package(Protobuf CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

# Generate protobuf files
set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/messages.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/camera.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/gps.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/imu.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Common source files
set(ADAS_APP_SOURCES
    adas_app.cpp
    adas_app.h
    logger.h
    ${PROTO_SRCS}
)

set(ADAS_APP_LINK_LIBRARIES protobuf::libprotobuf cppzmq)

# Add platform-specific sources to COMMON_SOURCES
if(BUILD_FOR_ANDROID)
  list(APPEND ADAS_APP_SOURCES adas_app_android.cpp)
  set(ANDROID_LIB_PATH ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android/27)
  list(APPEND ADAS_APP_LINK_LIBRARIES android log ${ANDROID_LIB_PATH}/libc.so
    ${ANDROID_LIB_PATH}/libm.so ${ANDROID_LIB_PATH}/libdl.so)
else()
  list(APPEND ADAS_APP_LINK_LIBRARIES pthread rt)
endif()

# Add the library
add_library(adas_app SHARED ${ADAS_APP_SOURCES})

# Include directories
target_include_directories(
  adas_app
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

# Link libraries
if(BUILD_FOR_ANDROID)
  target_compile_definitions(adas_app PUBLIC BUILD_FOR_ANDROID)
endif()

target_link_libraries(adas_app ${ADAS_APP_LINK_LIBRARIES})

# Enable testing if requested
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
  message(STATUS "Testing enabled - tests will be built")
endif()

add_subdirectory(panda)