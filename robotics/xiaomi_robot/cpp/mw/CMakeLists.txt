cmake_minimum_required(VERSION 3.16)
project(middleware LANGUAGES CXX)

# Keep middleware compatible with the top-level toolchain constraints.
# The parent project currently targets C++11 (trusty cross toolchain).
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(middleware STATIC src/middleware.cpp)

target_include_directories(
  middleware
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(
  middleware_example
  example.cpp
)

target_link_libraries(
  middleware_example
  PRIVATE middleware
)

add_library(microros STATIC src/microros.cpp src/callback_queue.cpp)

target_include_directories(
  microros
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(
  microros_example
  microros_example.cpp
)

target_link_libraries(
  microros_example
  PRIVATE microros
          pthread
)
