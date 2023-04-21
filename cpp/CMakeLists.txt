cmake_minimum_required(VERSION 3.0)
project(main)

add_executable(
  main
  main.cpp
)

find_package(PCL 1.2 REQUIRED)

include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

add_executable(
  map_updater
  map_updater.cpp
)
target_link_libraries(map_updater ${PCL_LIBRARIES})
