cmake_minimum_required(VERSION 3.10)
file(STRINGS VERSION CURRENT_VERSION)
project(programming VERSION ${CURRENT_VERSION})
include(cmake/helper.cmake)

if(BUILD_TESTS)
  enable_testing()

  add_subdirectory(thirdparty/googletest)
  add_subdirectory(cpp/gtest)

  set(BENCHMARK_ENABLE_INSTALL OFF)
  set(BENCHMARK_ENABLE_TESTING OFF)
  add_subdirectory(thirdparty/benchmark)
  add_subdirectory(cpp/gbenchmark)
endif()

add_subdirectory(dsa)
add_subdirectory(low_level_design/snake_and_ladder)
add_subdirectory(low_level_design/game_catalog)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dsa
  DESTINATION share/${PROJECT_NAME}
)

setup_cpack(
  MAINTAINER "Anatoly Kabakov <anatoly.kabakov@mail.ru>" DESCRIPTION
  "Interview preparation matherials." DEPENDENCIES "python3-pip clang-format \
  build-essential gdb cmake cppcheck"
)
