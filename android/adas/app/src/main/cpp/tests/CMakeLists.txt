# Find Google Test using vcpkg
find_package(GTest CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Test sources
set(TEST_SOURCES main.cpp test_service_manager.cpp test_zmq_imu.cpp
    test_utils.cpp
)

# Create test executable
add_executable(
  adas_tests
  ${TEST_SOURCES}
)

# Include directories
target_include_directories(
  adas_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Add compile definitions for tests
target_compile_definitions(
  adas_tests
  PRIVATE GTEST_HAS_TR1_TUPLE=0
)

# Link with the main adas_app library and test libraries
target_link_libraries(
  adas_tests
  adas_app_common
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
  Threads::Threads
  rt
)

# Enable testing
enable_testing()

# Add test to CTest
add_test(
  NAME adas_tests
  COMMAND adas_tests
)

# Set test properties
set_tests_properties(
  adas_tests
  PROPERTIES TIMEOUT 60 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
