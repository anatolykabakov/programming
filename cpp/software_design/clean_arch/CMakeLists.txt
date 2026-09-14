cmake_minimum_required(VERSION 3.10)
project(notebook)

option(BUILD_TESTS "Build unit tests" OFF)

find_package(httplib CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(tclap CONFIG REQUIRED)

add_library(notebook STATIC src/notebook.cpp)
target_include_directories(
  notebook
  PUBLIC include
)
target_link_libraries(notebook httplib::httplib JsonCpp::JsonCpp tclap::tclap)

add_executable(
  notebook_app
  src/main.cpp
)
target_link_libraries(notebook_app notebook)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
