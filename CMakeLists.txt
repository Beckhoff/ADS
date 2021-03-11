cmake_minimum_required(VERSION 3.13)

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
  add_compile_definitions(_GNU_SOURCE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Compiler flags and definitions for Visual Studio come here
endif()

add_subdirectory(AdsLib)
add_subdirectory(AdsLibTest)
add_subdirectory(example)
