include(CMakeParseArguments)
#cpack configuration
# cmake-lint: disable=C0103
function(setup_cpack)
  set(oneValueArgs MAINTAINER DESCRIPTION DEPENDENCIES)
  cmake_parse_arguments(SETUP_CPACK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_PACKAGING_INSTALL_PREFIX "")

  string(REPLACE "_" "-" DASH_PROJECT_NAME ${PROJECT_NAME})
  set(DASH_PROJECT_NAME ${DASH_PROJECT_NAME})

  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_NAME "${DASH_PROJECT_NAME}")

  set(CPACK_COMPONENTS_ALL ${COMPONENT_NAME})
  set(CPACK_DEBIAN_${COMPONENT_NAME}_PACKAGE_NAME ${DASH_PROJECT_NAME})

  set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${SETUP_CPACK_MAINTAINER})
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${SETUP_CPACK_DESCRIPTION})
  set(CPACK_DEBIAN_PACKAGE_DEPENDS ${SETUP_CPACK_DEPENDENCIES})
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
  set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md)

  #   set(CPACK_OUTPUT_CONFIG_FILE
  #   "${CMAKE_BINARY_DIR}/configs/${PROJECT_NAME}_Config.cmake"
  #   )
  set(CPACK_GENERATOR "DEB")
  include(CPack)
endfunction()

function(add_optional_sanitizers target_name)
  set(one_value_args IS_INTERFACE)

  cmake_parse_arguments(PARSE_ARGV 1 ADD_OPTIONAL_SANITIZERS "" "${one_value_args}" "")

  if(${ADD_OPTIONAL_SANITIZERS_IS_INTERFACE})
    set(LINK_TYPE "INTERFACE")
  else()
    set(LINK_TYPE "PRIVATE")
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")

    set(SANITIZERS "")

    option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    if(ENABLE_SANITIZER_ADDRESS)
      list(APPEND SANITIZERS "address")
    endif()

    option(ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    if(ENABLE_SANITIZER_LEAK)
      list(APPEND SANITIZERS "leak")
    endif()

    option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" OFF)
    if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
      list(APPEND SANITIZERS "undefined")
    endif()

    option(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    if(ENABLE_SANITIZER_THREAD)
      if("address" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
        message(WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled")
      else()
        list(APPEND SANITIZERS "thread")
      endif()
    endif()

    option(ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    if(ENABLE_SANITIZER_MEMORY AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
      if("address" IN_LIST SANITIZERS OR "thread" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
        message(WARNING "Memory sanitizer does not work with Address, Thread and Leak sanitizer enabled")
      else()
        list(APPEND SANITIZERS "memory")
      endif()
    endif()

    list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)

  endif()

  if(LIST_OF_SANITIZERS)
    if(NOT "${LIST_OF_SANITIZERS}" STREQUAL "")
      target_compile_options(${target_name} ${LINK_TYPE} -fsanitize=${LIST_OF_SANITIZERS})
      target_link_options(${target_name} BEFORE ${LINK_TYPE} -fsanitize=${LIST_OF_SANITIZERS})
    endif()
  endif()

endfunction()

function(add_optional_coverage target_name)
  set(one_value_args IS_INTERFACE)

  cmake_parse_arguments(PARSE_ARGV 1 ADD_OPTIONAL_COVERAGE "" "${one_value_args}" "")

  if(${ADD_OPTIONAL_COVERAGE_IS_INTERFACE})
    set(LINK_TYPE "INTERFACE")
  else()
    set(LINK_TYPE "PRIVATE")
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")

    option(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang" OFF)

    if(ENABLE_COVERAGE)
      target_compile_options(${target_name} ${LINK_TYPE} --coverage -fprofile-arcs -ftest-coverage)
      target_link_libraries(${target_name} ${LINK_TYPE} -lgcov --coverage)
    endif()

  endif()

endfunction()
