include(GoogleTest)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(
  main
  ${SOURCES}
)

target_link_libraries(
  main
  PRIVATE # GTest::gtest
          # GTest::gtest_main
)

target_include_directories(
  main
  PRIVATE include
)

gtest_discover_tests(main)
