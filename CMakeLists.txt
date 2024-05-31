cmake_minimum_required(VERSION 3.13.0)

project(ads)

find_package(Threads)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMPILE_OPTIONS
    -pedantic
    -Wall
    -Wextra
  )
  add_definitions(-D_GNU_SOURCE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_definitions(-DNOMINMAX)
endif()

add_definitions(-DCONFIG_DEFAULT_LOGLEVEL=1)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_subdirectory(AdsLib)
add_subdirectory(AdsTool)
add_subdirectory(AdsLibTest)
add_subdirectory(example)


# Install library
include(GenerateExportHeader)
set(ADSLIB_VERSION 0.0.20)

set_property(TARGET ads PROPERTY VERSION ${ADSLIB_VERSION})
set_property(TARGET ads PROPERTY SOVERSION 3)


# export library (either static or shared depending on BUILD_SHARED_LIBS)
install(TARGETS ads AdsTool
  EXPORT adsTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AdsLib/ DESTINATION include/ads/
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")


set(cmake_configfile_install lib/cmake/ads)
set(target_install_dest_name "${cmake_configfile_install}/adsTargets.cmake")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/adsConfigVersion.cmake"
  VERSION ${ADSLIB_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/cmake/adsConfig.cmake"
                INSTALL_DESTINATION "${cmake_configfile_install}"
                PATH_VARS target_install_dest_name)

install(EXPORT adsTargets
    FILE adsTargets.cmake
    NAMESPACE ads::
    DESTINATION "${cmake_configfile_install}"
    EXPORT_LINK_INTERFACE_LIBRARIES)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/adsConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/adsConfigVersion.cmake"
    DESTINATION "${cmake_configfile_install}")

