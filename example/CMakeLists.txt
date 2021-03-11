set(SOURCES
  example.cpp
)

add_executable(example.bin ${SOURCES})

target_link_libraries(example.bin PUBLIC ads)
