set(SOURCES
  main.cpp
)

add_executable(AdsLibTest.bin ${SOURCES})

target_link_libraries(AdsLibTest.bin PUBLIC ads)

target_include_directories(AdsLibTest.bin
  PRIVATE ../tools/
)
