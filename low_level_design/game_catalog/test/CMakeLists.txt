include(GoogleTest)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(
  ${PROJECT_NAME}_test
  ${SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}_test
  PRIVATE
    ${PROJECT_NAME}_lib
    gtest
    gmock
    gmock_main
    gtest_main
)

target_include_directories(
  ${PROJECT_NAME}_test
  PRIVATE include
)

gtest_discover_tests(${PROJECT_NAME}_test)
