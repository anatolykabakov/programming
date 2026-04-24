cmake_minimum_required(VERSION 3.10)

project(exception_handler)

set(BREAKPAD_DIR ${CMAKE_SOURCE_DIR}/breakpad/src/src)

add_executable(
  exception_handler
  src/exception_handler.cpp
)

target_link_libraries(
  exception_handler "${BREAKPAD_DIR}/client/linux/libbreakpad_client.a"
)

target_include_directories(
  exception_handler
  PRIVATE ${BREAKPAD_DIR}
)

add_executable(
  simple_backtrace
  src/simple_backtrace.cpp
)
target_compile_options(
  simple_backtrace
  PRIVATE -g
          -fno-omit-frame-pointer
)
target_link_options(simple_backtrace PRIVATE -rdynamic)

add_executable(
  coredump
  src/coredump.cpp
)
target_compile_options(
  coredump
  PRIVATE -g
          -fno-omit-frame-pointer
)
target_link_options(coredump PRIVATE -rdynamic)
