cmake_minimum_required(VERSION 3.23.0)

# Read the changelog file so that we can extract the version.
# The changelog has entries like 'adstool (113.0.33-1) trixie; urgency=medium'
file(READ "${CMAKE_SOURCE_DIR}/debian/changelog" CHANGELOG_CONTENT)
# Find the first instance of (VERSION-N)" and capture that string
string(REGEX MATCH "\\([0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+\\)" VERSION_MATCH "${CHANGELOG_CONTENT}")
# Strip the surrounding parentheses and leave only the semantic version e.g., 113.0.33
string(REGEX REPLACE "\\(([0-9]+\\.[0-9]+\\.[0-9]+)-[0-9]+\\)" "\\1" ADSLIB_VERSION "${VERSION_MATCH}")
# Do some checking to avoid issues.
if ("${ADSLIB_VERSION}" STREQUAL "")
    message(FATAL_ERROR "Unable to extract AdsLib version from /debian/changelog")
endif ()
message(STATUS "AdsLib Version: ${ADSLIB_VERSION}")
# We extend the CMake module path to add our own customized *.cmake
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
project(ads VERSION ${ADSLIB_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (BUILD_SHARED_LIBS AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Shared library (.dll) not available in MSVC builds.")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_definitions(_GNU_SOURCE)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
endif ()
add_compile_definitions(CONFIG_DEFAULT_LOGLEVEL=1)

include(GNUInstallDirs)

option(WITH_EXAMPLES "Build the examples (for the variant)" ON)
option(WITH_TESTS "Build the test (for the variant)" ON)
option(STANDALONE_ONLY "Build only the AdsLib as stand alone (no TwinCat TcAdsDll targets)" OFF)

#Find the needed dependencies
find_package(Threads REQUIRED)
if (NOT STANDALONE_ONLY)
    find_package(TcAdsDll)
    if (NOT TcAdsDll_FOUND)
        message(WARNING "Could NOT find TcAdsDll. TwinCat project targets will not be added.")
    endif ()
endif ()
# This list variable will contain all the targets that are created and need to be install.
# It will be populated as the targets are declare on the cmake files. This is
# in order to deal with the possibility to create both targets if TcAdsDll is found.
set(LIST_TARGETS_TO_INSTALL)
# Add the source code.
add_subdirectory(AdsLib)
add_subdirectory(AdsTool)
if (WITH_EXAMPLES)
    add_subdirectory(example)
endif ()
if(WITH_TESTS)
    include(UseFructose)
    add_subdirectory(AdsLibTest)
    add_subdirectory(AdsTest)
    add_subdirectory(AdsLibTestRef)
    add_subdirectory(AdsLibOOITest)
endif ()

# Create CMakeConfig .
message(STATUS "Available Targets: ${LIST_TARGETS_TO_INSTALL}")
install(
        TARGETS ${LIST_TARGETS_TO_INSTALL}
        EXPORT AdsConfig
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ads
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ads
)

install(EXPORT AdsConfig
        NAMESPACE Ads::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ads
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/AdsConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/AdsConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ads
)

