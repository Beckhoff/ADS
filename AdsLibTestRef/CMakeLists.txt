# Skip this tests if no TcAdsDll
if (NOT TcAdsDll_FOUND)
    return()
endif ()
# Skip this test if we are not on Windows or FreeBSD system
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    return()
endif ()

set(SOURCES
        main.cpp
)
add_executable(TcAdsLibTestRef ${SOURCES})
target_link_libraries(TcAdsLibTestRef PRIVATE TcAdsLib Fructose)