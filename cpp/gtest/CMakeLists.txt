include(GoogleTest)
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(
  gtest_tutorial
  ${SOURCES}
)
target_link_libraries(
  gtest_tutorial
  PRIVATE
    gtest
    gtest_main
    gmock
)
gtest_discover_tests(gtest_tutorial)
