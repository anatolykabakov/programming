cmake_minimum_required(VERSION 3.10)
project(sfm)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

find_package(Ceres REQUIRED)
find_package(OpenMVG REQUIRED)
find_package(OpenMP REQUIRED)

add_library(mono_vo src/visual_odom_mono.cpp)

target_link_libraries(
  mono_vo
  PUBLIC
    ${OpenCV_LIBS}
    stdc++fs
    Eigen3::Eigen

    OpenMVG::openMVG_sfm
    OpenMP::OpenMP_CXX
    ${STLPLUS_LIBRARY}
)

target_include_directories(
  mono_vo
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
         ${OpenCV_INCLUDE_DIRS}
)

add_executable(
  kitti_vo_testbench
  src/kitti_vo_testbench.cpp
)

target_link_libraries(
  kitti_vo_testbench
  PRIVATE 
    mono_vo
)

add_executable(
  kitti_ba_testbench
  src/kitti_ba_testbench.cpp
)

target_link_libraries(
  kitti_ba_testbench
  PRIVATE
    mono_vo
    OpenMVG::openMVG_image
    OpenMVG::openMVG_features
    OpenMVG::openMVG_matching
    OpenMVG::openMVG_system
    OpenMVG::openMVG_sfm
    ${STLPLUS_LIBRARY}
    Eigen3::Eigen
)

add_executable(
  ba_example
  src/ba_example.cpp
)

target_link_libraries(
  ba_example
  PRIVATE
    OpenMVG::openMVG_image
    OpenMVG::openMVG_features
    OpenMVG::openMVG_matching
    OpenMVG::openMVG_system
    OpenMVG::openMVG_sfm
    ${STLPLUS_LIBRARY}
    Eigen3::Eigen
)

target_include_directories(
  ba_example
  PRIVATE "/opt/include"
)
