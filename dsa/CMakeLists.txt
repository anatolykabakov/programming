cmake_minimum_required(VERSION 3.10)
project(dsa)

if(BUILD_TESTS)
  file(GLOB_RECURSE CPP_FILES ${PROJECT_SOURCE_DIR}/cpp/*.cpp)

  foreach(cpp_file ${CPP_FILES})
    get_filename_component(FILENAME ${cpp_file} NAME)
    string(REPLACE ".cpp" "" TARGETNAME ${FILENAME})
    add_executable(${TARGETNAME} ${cpp_file})
    add_test(${TARGETNAME}Test ${TARGETNAME})
    add_optional_sanitizers(${TARGETNAME})
    add_optional_coverage(${TARGETNAME})
  endforeach()

  file(GLOB_RECURSE PY_FILES ${PROJECT_SOURCE_DIR}/python3/*.py)

  foreach(py_file ${PY_FILES})
    get_filename_component(FILENAME ${py_file} NAME)
    string(REPLACE ".py" "" TARGETNAME ${FILENAME})
    add_test(NAME ${TARGETNAME}PythonTest COMMAND python3 ${py_file})
  endforeach()
endif()
