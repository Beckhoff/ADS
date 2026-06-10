set(SOURCES
  example.cpp
)

add_executable(AdsExample ${SOURCES})
target_link_libraries(AdsExample PRIVATE AdsLib)

if(TcAdsDll_FOUND AND NOT ONLY_STANDALONE)
    add_executable(TcAdsExample ${SOURCES})
    target_link_libraries(TcAdsExample PRIVATE TcAdsLib)
endif ()
