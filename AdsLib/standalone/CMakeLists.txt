cmake_minimum_required(VERSION 3.13)

set(SOURCES
  AdsLib.cpp
  AmsConnection.cpp
  AmsNetId.cpp
  AmsPort.cpp
  AmsRouter.cpp
  NotificationDispatcher.cpp
)

add_library(ads_standalone ${SOURCES})

target_include_directories(ads_standalone PUBLIC ..)

target_link_libraries(ads_standalone ads)
