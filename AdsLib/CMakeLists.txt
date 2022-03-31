file(GLOB BASE_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE MY_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")

if (ADS_USE_TWINCAT_ROUTER STREQUAL "ON")
    if (NOT DEFINED ENV{TWINCAT3DIR}) 
        message(FATAL_ERROR "Expected TWINCAT3DIR env-var to be set. Please install TwinCAT/XAE!")
    endif()
    file(GLOB ROUTER_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/TwinCAT/*.cpp")
else ()
    file(GLOB ROUTER_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/standalone/*.cpp")
endif()

add_library(ads
    ${BASE_SOURCES}
    ${ROUTER_SOURCES}
    ${MY_HEADERS}
)

target_include_directories(ads PUBLIC .)

if (ADS_USE_TWINCAT_ROUTER STREQUAL "ON")
    target_compile_definitions(ads PUBLIC USE_TWINCAT_ROUTER)
    target_include_directories(ads PUBLIC "$ENV{TWINCAT3DIR}\\..\\AdsApi\\TcAdsDll\\include")
    target_link_libraries(ads PUBLIC "$ENV{TWINCAT3DIR}\\..\\AdsApi\\TcAdsDll\\x64\\lib\\TcAdsDll.lib")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_link_libraries(ads PUBLIC wsock32)
else()
  target_compile_options(ads PUBLIC -fPIC)
endif()

if(WIN32 EQUAL 1)
    target_link_libraries(ads PUBLIC ws2_32)
endif()

target_link_libraries(ads PUBLIC Threads::Threads)
