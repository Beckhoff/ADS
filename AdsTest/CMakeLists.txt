set(SOURCES
        main.cpp
        RegistryAccessTest.cpp
        RegistryAccessTest.h
)

add_executable(AdsTest ${SOURCES})
target_link_libraries(AdsTest PRIVATE AdsLib)

if(TcAdsDll_FOUND AND NOT ONLY_STANDALONE)
    add_executable(TcAdsTest ${SOURCES})
    target_link_libraries(TcAdsTest PRIVATE TcAdsLib)
endif ()
