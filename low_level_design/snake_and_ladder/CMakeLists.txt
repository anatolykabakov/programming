cmake_minimum_required(VERSION 3.10)
project(snake_and_ladder)

add_library(
  ${PROJECT_NAME}_lib src/snake_and_ladder/game.cpp
  src/snake_and_ladder/board.cpp src/snake_and_ladder/player.cpp
)

target_include_directories(
  ${PROJECT_NAME}_lib
  PUBLIC include
)

add_executable(
  ${PROJECT_NAME}
  src/main.cpp
)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC ${PROJECT_NAME}_lib
)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
