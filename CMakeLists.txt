cmake_minimum_required(VERSION 3.10.2)

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
  # Compiler flags and definitions for Visual Studio come here
endif()

if (WIN32)
  # need __dllexport/_dllimport declarations on classes & free functions Windows, so default to static linking
  option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
else()
  option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
endif()

option(ADSLIB_ONLY "Build only the ADS library, not examples or tests" OFF)

add_subdirectory(AdsLib)
if(ADSLIB_ONLY STREQUAL "OFF")
    add_subdirectory(AdsLibTest)
    add_subdirectory(example)
endif()
