cmake_minimum_required(VERSION 3.14)
project(tictactoe)

file(GLOB_RECURSE TICTACTOE_SRC ${PROJECT_SOURCE_DIR}/src/tictactoe/*.cpp)

add_library(${PROJECT_NAME}_lib ${TICTACTOE_SRC})
target_include_directories(
  ${PROJECT_NAME}_lib
  PUBLIC include
)

add_executable(
  ${PROJECT_NAME}_exec
  src/main.cpp
)

target_link_libraries(${PROJECT_NAME}_exec ${PROJECT_NAME}_lib)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()
