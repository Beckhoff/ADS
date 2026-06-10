set(SOURCES
        main.cpp
)

add_executable(AdsLibTest ${SOURCES})
target_link_libraries(AdsLibTest PRIVATE AdsLib Fructose)
