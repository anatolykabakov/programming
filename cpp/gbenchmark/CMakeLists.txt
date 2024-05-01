file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(
  gbenchmark_tutorial
  ${SOURCES}
)
target_link_libraries(
  gbenchmark_tutorial
  PRIVATE benchmark
)
