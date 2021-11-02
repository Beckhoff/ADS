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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_link_libraries(ads PRIVATE wsock32 ws2_32)
endif()


target_link_libraries(ads PUBLIC Threads::Threads)
