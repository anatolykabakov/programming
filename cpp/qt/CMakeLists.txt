cmake_minimum_required(VERSION 3.1.0)

project(helloworld VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(
  Qt5
  COMPONENTS
    Widgets
    Core
    Gui
    REQUIRED
)

add_executable(
  helloworld
  qt.cpp
)

target_link_libraries(helloworld Qt5::Widgets Qt5::Core Qt5::Gui)
