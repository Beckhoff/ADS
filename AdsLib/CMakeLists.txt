cmake_minimum_required(VERSION 3.13)

set(SOURCES
  AdsDef.cpp
  AdsDevice.cpp
  AdsLib.cpp
  AdsNotification.cpp
  AmsConnection.cpp
  AmsPort.cpp
  AmsRouter.cpp
  Log.cpp
  NotificationDispatcher.cpp
  Sockets.cpp
  Frame.cpp
)

add_library(ads ${SOURCES})

target_include_directories(ads INTERFACE .)

target_compile_features(ads PRIVATE
  cxx_std_11
)

target_compile_options(ads PRIVATE
  -pedantic
  -Wall
  -Wextra
)

target_compile_definitions(ads PRIVATE
  _GNU_SOURCE
)
