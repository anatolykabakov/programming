cmake_minimum_required(VERSION 3.10)
project(dsa)

if(BUILD_TESTS)
  file(GLOB_RECURSE CPP_FILES ${PROJECT_SOURCE_DIR}/cpp/*.cpp)

  foreach(cpp_file ${CPP_FILES})
    get_filename_component(FILENAME ${cpp_file} NAME)
    string(REPLACE ".cpp" "" FILENAME ${FILENAME})
    add_executable(${FILENAME} ${cpp_file})
    add_test(${FILENAME}Test ${FILENAME})
  endforeach()

  file(GLOB_RECURSE PY_FILES ${PROJECT_SOURCE_DIR}/python3/*.py)

  foreach(py_file ${PY_FILES})
    get_filename_component(FILENAME ${py_file} NAME)
    string(REPLACE ".py" "" FILENAME ${FILENAME})
    message(${FILENAME})
    add_test(NAME ${FILENAME}PythonTest COMMAND python3 ${py_file})
  endforeach()
endif()
