cmake_minimum_required(VERSION 3.13)

project(example)

set(SOURCES
  example.cpp
)

add_executable(example.bin ${SOURCES})

target_link_libraries(example.bin ads pthread)
