include(GoogleTest)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(
  ${PROJECT_NAME}_test
  ${SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}_test
  PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${PROJECT_NAME}_lib
)

target_include_directories(
  ${PROJECT_NAME}_test
  PRIVATE include
)

gtest_discover_tests(${PROJECT_NAME}_test)
