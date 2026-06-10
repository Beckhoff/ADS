set(LIB_SOURCES
        AdsDef.cpp
        AdsDevice.cpp
        AdsFile.cpp
        AdsLib.cpp
        ECatAccess.cpp
        Frame.cpp
        LicenseAccess.cpp
        Log.cpp
        MasterDcStatAccess.cpp
        RTimeAccess.cpp
        RegistryAccess.cpp
        RouterAccess.cpp
        Sockets.cpp
        SymbolAccess.cpp
)
set(LIB_HEADERS
        AdsDef.h
        AdsDevice.h
        AdsException.h
        AdsFile.h
        AdsLib.h
        AdsNotification.h
        AdsNotificationOOI.h
        AdsVariable.h
        AmsConnection.h
        AmsHeader.h
        AmsPort.h
        AmsRouter.h
        ECatAccess.h
        Frame.h
        LicenseAccess.h
        Log.h
        MasterDcStatAccess.h
        NotificationDispatcher.h
        RegistryAccess.h
        RingBuffer.h
        Router.h
        RouterAccess.h
        RTimeAccess.h
        Semaphore.h
        Sockets.h
        SymbolAccess.h
        wrap_socket.h
        wrap_registry.h
        wrap_endian.h
)

set(BHF_SOURCES
        bhf/ParameterList.cpp
)
set(BHF_HEADERS
        bhf/ParameterList.h
        bhf/StringToInteger.h
        bhf/WindowsQuirks.h
)

set(TWINCAT_SOURCES
        TwinCAT/AdsLib.cpp
)
set(TWINCAT_HEADERS
        TwinCAT/AdsDef.h
        TwinCAT/AdsLib.h
)

set(STANDALONE_SOURCES
        standalone/AdsLib.cpp
        standalone/AmsConnection.cpp
        standalone/AmsNetId.cpp
        standalone/AmsPort.cpp
        standalone/AmsRouter.cpp
        standalone/NotificationDispatcher.cpp
)
set(STANDALONE_HEADERS
        standalone/AdsDef.h
        standalone/AdsLib.h
)

# Create the standalone variant
add_library(AdsLib)
target_sources(AdsLib PRIVATE
        ${STANDALONE_SOURCES}
        ${LIB_SOURCES}
        ${BHF_SOURCES}
)
target_sources(AdsLib PUBLIC
        FILE_SET HEADERS
        FILES
        ${LIB_HEADERS}
        ${BHF_HEADERS}
        ${STANDALONE_HEADERS}
)
target_include_directories(AdsLib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/ads>
)

target_link_libraries(AdsLib
        PUBLIC
        Threads::Threads
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(AdsLib
            PUBLIC
            wsock32
            ws2_32
    )
endif ()

if (BUILD_SHARED_LIBS)
    target_compile_definitions(TcAdsLib
            PUBLIC
            BHF_ADS_EXPORT_C
            BHF_ADS_USE_TWINCAT_ORDER
    )
endif ()

list(APPEND LIST_TARGETS_TO_INSTALL AdsLib)

# Create the TwinCat variant.
if (TcAdsDll_FOUND AND NOT ONLY_STANDALONE)
    add_library(TcAdsLib)
    target_sources(TcAdsLib PRIVATE
            ${TWINCAT_SOURCES}
            ${LIB_SOURCES}
            ${BHF_SOURCES}
    )
    target_sources(TcAdsLib PUBLIC
            FILE_SET HEADERS
            FILES
            ${LIB_HEADERS}
            ${BHF_HEADERS}
            ${TWINCAT_HEADERS}
    )
    target_include_directories(TcAdsLib PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include/ads>
    )

    target_link_libraries(TcAdsLib
            PUBLIC
            Threads::Threads
            TcAdsDll::TcAdsDll
    )
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_link_libraries(TcAdsLib
                PUBLIC
                wsock32
                ws2_32
        )
    endif ()
    target_compile_definitions(TcAdsLib
            PUBLIC
            USE_TWINCAT_ROUTER
    )
    if (BUILD_SHARED_LIBS)
        target_compile_definitions(TcAdsLib
                PUBLIC
                BHF_ADS_EXPORT_C
        )
    endif ()
    list(APPEND LIST_TARGETS_TO_INSTALL TcAdsLib)
endif ()

# Pass the created targets to the parent scope
set(LIST_TARGETS_TO_INSTALL "${LIST_TARGETS_TO_INSTALL}" PARENT_SCOPE)

