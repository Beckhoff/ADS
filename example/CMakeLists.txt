cmake_minimum_required(VERSION 3.13)

project(example)

set(SOURCES
  example.cpp
)

add_executable(example.bin ${SOURCES})

target_link_libraries(example.bin ads pthread)

target_compile_features(example.bin PRIVATE
  cxx_std_11
)

target_compile_options(example.bin PRIVATE
  -pedantic
  -Wall
  -Wextra
)

target_compile_definitions(example.bin PRIVATE
  _GNU_SOURCE
)
