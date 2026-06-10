# Skip this tests if no TcAdsDll
if (NOT TcAdsDll_FOUND)
    return()
endif ()

set(SOURCES
        main.cpp
)
add_executable(TcAdsLibTestRef ${SOURCES})
target_link_libraries(TcAdsLibTestRef PRIVATE TcAdsLib Fructose)