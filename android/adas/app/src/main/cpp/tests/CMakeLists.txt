find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(
  adas_tests
  main.cpp test_middleware.cpp test_zmq_imu.cpp test_utils.cpp
  test_lat_control.cpp test_panda_mqb.cpp
)

target_include_directories(
  adas_tests
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
          ${CMAKE_CURRENT_BINARY_DIR}/..
)

target_compile_definitions(
  adas_tests
  PRIVATE GTEST_HAS_TR1_TUPLE=0
)

target_link_libraries(
  adas_tests
  PRIVATE
    adas_app_common
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    Threads::Threads
    rt
)

add_test(
  NAME adas_tests
  COMMAND adas_tests
)
set_tests_properties(
  adas_tests
  PROPERTIES TIMEOUT 60
)
