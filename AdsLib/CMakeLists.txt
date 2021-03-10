cmake_minimum_required(VERSION 3.13)

set(SOURCES
  AdsDevice.cpp
  AdsDef.cpp
  Log.cpp
  Sockets.cpp
  Frame.cpp
  standalone/AdsLib.cpp
  standalone/AmsConnection.cpp
  standalone/AmsNetId.cpp
  standalone/AmsPort.cpp
  standalone/AmsRouter.cpp
  standalone/NotificationDispatcher.cpp
)

add_library(ads ${SOURCES})

target_include_directories(ads PUBLIC .)
