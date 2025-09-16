# Find libusb - different approach for Android vs Linux
if(BUILD_FOR_ANDROID)
  # For Android, find libusb directly from vcpkg
  find_library(
    LIBUSB_LIBRARY
    NAMES usb-1.0
    PATHS ${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/arm64-android/lib
    NO_DEFAULT_PATH
  )
  find_path(
    LIBUSB_INCLUDE_DIR
    NAMES libusb.h
    PATHS ${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/arm64-android/include
    PATH_SUFFIXES libusb-1.0
    NO_DEFAULT_PATH
  )

  if(LIBUSB_LIBRARY
     AND LIBUSB_INCLUDE_DIR
  )
    add_library(libusb::libusb INTERFACE IMPORTED)
    set_target_properties(
      libusb::libusb
      PROPERTIES INTERFACE_LINK_LIBRARIES ${LIBUSB_LIBRARY}
                 INTERFACE_INCLUDE_DIRECTORIES ${LIBUSB_INCLUDE_DIR}
    )
    set(libusb_FOUND TRUE)
  else()
    message(FATAL_ERROR "libusb not found for Android")
  endif()
else()
  # For Linux, use pkg-config
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)

  # Alternative: if pkg-config fails, try to find libusb directly
  if(NOT libusb_FOUND)
    find_library(
      LIBUSB_LIBRARY
      NAMES usb-1.0
      PATHS ${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-linux/lib
      NO_DEFAULT_PATH
    )
    find_path(
      LIBUSB_INCLUDE_DIR
      NAMES libusb.h
      PATHS ${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-linux/include
      PATH_SUFFIXES libusb-1.0
      NO_DEFAULT_PATH
    )

    if(LIBUSB_LIBRARY
       AND LIBUSB_INCLUDE_DIR
    )
      add_library(libusb::libusb INTERFACE IMPORTED)
      set_target_properties(
        libusb::libusb
        PROPERTIES INTERFACE_LINK_LIBRARIES ${LIBUSB_LIBRARY}
                   INTERFACE_INCLUDE_DIRECTORIES ${LIBUSB_INCLUDE_DIR}
      )
      set(libusb_FOUND TRUE)
    endif()
  endif()
endif()

add_library(adas_panda STATIC panda.cc panda_comms.cc panda.h panda_comms.h)

set_target_properties(
  adas_panda
  PROPERTIES POSITION_INDEPENDENT_CODE ON
)

if(BUILD_FOR_ANDROID)
  target_include_directories(
    adas_panda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
           ${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/arm64-android/include
  )
  target_compile_definitions(
    adas_panda
    PUBLIC LIBUSB_OPTION_WEAK_AUTHORITY
  )
else()
  target_include_directories(
    adas_panda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
           ${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-linux/include
  )
endif()

# Link libusb to adas_panda library
if(libusb_FOUND)
  if(TARGET PkgConfig::libusb)
    target_link_libraries(
      adas_panda
      PRIVATE PkgConfig::libusb
    )
  elseif(TARGET libusb::libusb)
    target_link_libraries(
      adas_panda
      PRIVATE libusb::libusb
    )
    # For Android, add system libraries that libusb needs
    if(BUILD_FOR_ANDROID)
      set(ANDROID_LIB_PATH ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android/27)
      target_link_libraries(
        adas_panda
        PRIVATE
          log
          android
          ${ANDROID_LIB_PATH}/libc.so
          ${ANDROID_LIB_PATH}/libm.so
      )
    endif()
  else()
    message(FATAL_ERROR "libusb target not found for adas_panda library")
  endif()
else()
  message(FATAL_ERROR "libusb not found for adas_panda library")
endif()
