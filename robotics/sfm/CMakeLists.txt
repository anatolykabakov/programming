cmake_minimum_required(VERSION 3.10)
project(sfm)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

find_package(Ceres REQUIRED)
find_package(OpenMVG REQUIRED)
find_package(OpenMP REQUIRED)

find_package(CycloneDDS-CXX CONFIG REQUIRED)
find_package(dds_data_structures CONFIG REQUIRED)
find_package(PCL CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED program_options)


add_library(dds_helper src/dds.cpp)

target_link_libraries(
  dds_helper
  PUBLIC
    CycloneDDS-CXX::ddscxx
    CycloneDDS::ddsc
    Atom::dds_data_structures
    ${OpenCV_LIBS}
    OpenMVG::openMVG_sfm
    OpenMVG::openMVG_matching
    OpenMVG::openMVG_image
    stdc++
    ${PCL_LIBRARIES}
)

target_include_directories(
  dds_helper
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${CycloneDDS-CXX_INCLUDE_DIRS}
    ${CycloneDDS_INCLUDE_DIRS}
    ${CycloneDDS-CXX_DIR}
    ${PCL_INCLUDE_DIRS}
)

add_library(
  mono_vo
  src/utils.cpp
  src/tracker.cpp
  src/sfm.cpp
  src/mapping.cpp
  src/debug.cpp
  src/odom.cpp
)

target_link_libraries(
  mono_vo
  PUBLIC
    ${OpenCV_LIBS}
    stdc++fs
    Eigen3::Eigen
    OpenMVG::openMVG_image
    OpenMVG::openMVG_features
    OpenMVG::openMVG_matching
    OpenMVG::openMVG_system
    OpenMVG::openMVG_sfm
    OpenMP::OpenMP_CXX
    ${STLPLUS_LIBRARY}
    dds_helper
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
    opencv_core
    opencv_highgui
    opencv_imgproc
    opencv_features2d
    opencv_calib3d
    opencv_imgcodecs
    Boost::program_options
)

# add_executable(
#   simple_triangulation
#   src/simple_triangulation.cpp
# )

# target_link_libraries(
#   simple_triangulation
#   PRIVATE
#     mono_vo
#     opencv_core
#     opencv_highgui
#     opencv_imgproc
#     opencv_features2d
#     opencv_calib3d
#     opencv_imgcodecs
# )

# find_package(GTSAM REQUIRED)

# add_executable(
#   sfm_example
#   src/sfm_example.cpp
# )

# target_link_libraries(
#   sfm_example
#   PRIVATE
#     mono_vo
#     gtsam
#     opencv_core
#     opencv_highgui
#     opencv_imgproc
#     opencv_features2d
#     opencv_calib3d
#     opencv_imgcodecs
# )

# add_executable(
#   ba_example
#   src/ba_example.cpp
# )

# target_link_libraries(
#   ba_example
#   PRIVATE
#     OpenMVG::openMVG_image
#     OpenMVG::openMVG_features
#     OpenMVG::openMVG_matching
#     OpenMVG::openMVG_system
#     OpenMVG::openMVG_sfm
#     ${STLPLUS_LIBRARY}
#     Eigen3::Eigen
# )

# target_include_directories(
#   ba_example
#   PRIVATE "/opt/include"
# )
