set(SOURCES
        main.cpp
)

add_executable(AdsLibOOITest ${SOURCES})
target_link_libraries(AdsLibOOITest PRIVATE AdsLib Fructose)