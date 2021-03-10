cmake_minimum_required(VERSION 3.13)

project(ads-test)

set(SOURCES
  main.cpp
)

add_executable(AdsLibTest.bin ${SOURCES})

target_link_libraries(AdsLibTest.bin ads pthread)

target_include_directories(AdsLibTest.bin
  PRIVATE ${PROJECT_SOURCE_DIR}/../tools/
)
