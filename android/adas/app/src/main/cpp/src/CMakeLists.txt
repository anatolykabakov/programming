add_subdirectory(panda)
add_subdirectory(utils)

add_library(
  adas_app_common
  STATIC
  adas_app.cpp
  services/sensor_reader_service.cpp
  services/zmq_bridge_service.cpp
  services/panda_service.cpp
  volkswagen/mqbcan.cpp
  volkswagen/carcontroller.cpp
  utils/file_logger.cpp
  utils/protobuf_utils.cpp
  utils/can_parser.cc
)
target_link_libraries(
  adas_app_common
  PUBLIC
    cppzmq
    protobuf_generated
    adas_panda
)
target_include_directories(
  adas_app_common
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
)

if(BUILD_FOR_ANDROID)
  target_compile_definitions(
    adas_app_common
    PUBLIC BUILD_FOR_ANDROID
  )
  target_link_libraries(
    adas_app_common
    PUBLIC log
           android
  )
  add_library(adas_app_android SHARED adas_app_android.cpp)
  target_link_libraries(
    adas_app_android
    PUBLIC
      adas_app_common
      adas_panda
      android
      log
  )
  target_compile_definitions(
    adas_app_android
    PUBLIC BUILD_FOR_ANDROID
  )
else()
  add_executable(
    adas_app_linux
    adas_app_linux.cpp
  )
  target_link_libraries(
    adas_app_linux
    PUBLIC
      adas_app_common
      adas_panda
      pthread
      rt
  )
endif()
