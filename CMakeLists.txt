cmake_minimum_required(VERSION 3.10)
file(STRINGS VERSION CURRENT_VERSION)
project(programming VERSION ${CURRENT_VERSION})
include(cmake/helper.cmake)

enable_testing()
add_subdirectory(dsa)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dsa DESTINATION share/${PROJECT_NAME})

setup_cpack(
  MAINTAINER "Anatoly Kabakov <anatoly.kabakov@mail.ru>" DESCRIPTION "Interview preparation matherials." DEPENDENCIES
  "python3-pip clang-format \
  build-essential gdb cmake cppcheck")
