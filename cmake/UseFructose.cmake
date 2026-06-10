include_guard(GLOBAL)

# Find the Fructose copy inside the tools directory and set it as an imported target
add_library(Fructose INTERFACE)
target_include_directories(Fructose INTERFACE
        ${CMAKE_SOURCE_DIR}/tools
)