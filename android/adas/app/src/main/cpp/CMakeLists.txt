cmake_minimum_required(VERSION 3.17)
project(adas_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_FOR_ANDROID "Build for Android platform" ON)
option(BUILD_TESTING "Build tests" OFF)

find_package(Protobuf CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/messages.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/camera.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/gps.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/imu.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/can.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/panda.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/camera_intrinsics.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(protobuf_generated STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(
  protobuf_generated
  PUBLIC protobuf::libprotobuf
)
target_include_directories(
  protobuf_generated
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
         ${Protobuf_INCLUDE_DIRS}
)

add_subdirectory(panda)
add_subdirectory(utils)

add_library(
  adas_app_common
  STATIC
  adas_app.cpp
  adas_app.h
  services/sensor_reader_service.cpp
  services/sensor_reader_service.h
  services/zmq_bridge_service.cpp
  services/zmq_bridge_service.h
  services/panda_service.cpp
  services/panda_service.h
  framework/service_manager.hpp
  utils/logger.h
  utils/file_logger.cpp
  utils/file_logger.h
  utils/protobuf_utils.cpp
  utils/protobuf_utils.h
)
target_link_libraries(adas_app_common cppzmq protobuf_generated)
target_include_directories(
  adas_app_common
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/services
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
    ${CMAKE_CURRENT_SOURCE_DIR}/examples
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(BUILD_FOR_ANDROID)
  set(ANDROID_LIB_PATH ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android/27)
  add_library(adas_app_android SHARED adas_app_android.cpp)
  target_link_libraries(
    adas_app_android
    adas_app_common
    adas_panda
    android
    log
    ${ANDROID_LIB_PATH}/libc.so
    ${ANDROID_LIB_PATH}/libm.so
    ${ANDROID_LIB_PATH}/libdl.so
  )
  target_include_directories(
    adas_app_android
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${Protobuf_INCLUDE_DIRS}
  )
  target_compile_definitions(
    adas_app_android
    PUBLIC BUILD_FOR_ANDROID
  )
  set_target_properties(
    adas_app_android
    PROPERTIES POSITION_INDEPENDENT_CODE ON
  )
else()
  add_executable(
    adas_app_linux
    adas_app_linux.cpp
  )
  target_link_libraries(adas_app_linux adas_app_common adas_panda pthread rt)
  target_include_directories(
    adas_app_linux
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${Protobuf_INCLUDE_DIRS}
  )
endif()

# Enable testing if requested
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
  message(STATUS "Testing enabled - tests will be built")
endif()
