
set(SOURCES
        main.cpp
)
# Create the standalone variant
add_executable(AdsTool ${SOURCES})
target_link_libraries(AdsTool PRIVATE AdsLib)
set_target_properties(AdsTool PROPERTIES
        OUTPUT_NAME "adstool"
)
list(APPEND LIST_TARGETS_TO_INSTALL AdsTool)

# Create the TwinCat variant
if (TcAdsDll_FOUND AND NOT ONLY_STANDALONE)
    add_executable(TcAdsTool ${SOURCES})
    target_link_libraries(TcAdsTool PRIVATE TcAdsLib)
    set_target_properties(TcAdsTool PROPERTIES
            OUTPUT_NAME "tcadstool"
    )
    list(APPEND LIST_TARGETS_TO_INSTALL TcAdsTool)
endif ()

# Pass the created targets to the parent scope
set(LIST_TARGETS_TO_INSTALL "${LIST_TARGETS_TO_INSTALL}" PARENT_SCOPE)