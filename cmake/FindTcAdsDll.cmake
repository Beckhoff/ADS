# FindTcAdsDll.cmake — locate TcAdsDll (TwinCAT ADS DLL) library
#
# - Attempts to locate the ADS include directory and library file.
# - On success defines:
#     TcAdsDll_FOUND
#     TcAdsDll_INCLUDE_DIRS
#     TcAdsDll_LIBRARIES
#
# Usage:
#   find_package(TcAdsDll [REQUIRED])
#   if (TcAdsDll_FOUND)
#     target_link_libraries(my_target PRIVATE TcAdsDll::TcAdsDll)
#   endif()

if (NOT TcAdsDll_FIND_QUIETLY)
    message(STATUS "Looking for TcAdsDll (TwinCAT ADS-DLL)...")
endif ()

if (NOT WIN32)
    message(WARNING "FindTcAdsDll.cmake only tested on WINDOWS")
endif ()

if (WIN32)
    # Typical install locations on Windows
    set(_TcAdsDll_PATH "$ENV{SystemDrive}/TwinCAT/AdsApi/TcAdsDll")
else ()
    # TODO: Linux not tested. Set additional known default locations to search.
    set(_TcAdsDll_PATH)
endif ()
# Find the include headers
find_path(TcAdsDll_INCLUDE_DIR
        NAMES TcAdsApi.h TcAdsDef.h
        PATHS "${_TcAdsDll_PATH}"
        PATH_SUFFIXES "Include" "include"
)
# Find all related files base on the include files location. This is done
# assuming that the files are ordered as install by TwinCat. If they are
# in some other configuration this will not work.
if (WIN32)
    cmake_path(GET TcAdsDll_INCLUDE_DIR PARENT_PATH TcAdsDll_ROOT_DIR)
    if (NOT TcAdsDll_FIND_QUIETLY)
        message(STATUS "Looking for TcAdsDll in ROOT ${TcAdsDll_ROOT_DIR}")
    endif ()
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(TcAdsDll_IMPLIB_DIR "${TcAdsDll_ROOT_DIR}/x64/lib")
        set(TcAdsDll_DLL_DIR "${TcAdsDll_ROOT_DIR}/x64")
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(TcAdsDll_IMPLIB_DIR "${TcAdsDll_ROOT_DIR}/Lib")
        set(TcAdsDll_DLL_DIR "${TcAdsDll_ROOT_DIR}")
    endif ()
    # Use NO_DEFAULT_PATH so that we only look in the provided location. If not set
    # the find_library will find the 32 bit version first regardless of the config
    # which leads to errors.
    find_library(TcAdsDll_IMPLIB
            NAMES TcAdsDll
            PATHS "${TcAdsDll_IMPLIB_DIR}"
            NO_DEFAULT_PATH
    )
    find_file(TcAdsDll_LIBRARY
            NAMES TcAdsDll.dll
            PATHS "${TcAdsDll_DLL_DIR}"
            NO_DEFAULT_PATH
    )
else ()
    # TODO: Linux not tested. We just try to look for the library by name.
    find_library(TcAdsDll_LIBRARY
            NAMES TcAdsDll
    )
endif ()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TcAdsDll
        REQUIRED_VARS TcAdsDll_INCLUDE_DIR TcAdsDll_LIBRARY
        VERSION_VAR TcAdsDll_VERSION
)

if (NOT TcAdsDll_FOUND)
    if (NOT TcAdsDll_FIND_QUIETLY)
        message(WARNING "Could NOT find TcAdsDll. Try setting TcAdsDll_ROOT to the installation directory.")
    endif ()
    return()
endif ()

set(TcAdsDll_INCLUDE_DIRS "${TcAdsDll_INCLUDE_DIR}")
set(TcAdsDll_LIBRARIES "${TcAdsDll_LIBRARY}")
if (WIN32)
    list(APPEND TcAdsDll_LIBRARIES "${TcAdsDll_IMPLIB}" )
endif ()
set(TcAdsDll_FOUND "${TcAdsDll_FOUND}")

mark_as_advanced(TcAdsDll_INCLUDE_DIR TcAdsDll_LIBRARY)

# ---- Create imported target -------------------------------------------------

if (NOT TARGET TcAdsDll::TcAdsDll)
    add_library(TcAdsDll::TcAdsDll SHARED IMPORTED)
    if (WIN32)
        set_target_properties(TcAdsDll::TcAdsDll PROPERTIES
                IMPORTED_LOCATION "${TcAdsDll_LIBRARY}"
                IMPORTED_IMPLIB "${TcAdsDll_IMPLIB}"
                INTERFACE_INCLUDE_DIRECTORIES "${TcAdsDll_INCLUDE_DIR}"
        )
    else ()
        set_target_properties(TcAdsDll::TcAdsDll PROPERTIES
                IMPORTED_LOCATION "${TcAdsDll_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${TcAdsDll_INCLUDE_DIR}"
        )
    endif ()
endif ()

if (NOT TcAdsDll_FIND_QUIETLY)
    message(STATUS "Found TcAdsDll (lib): ${TcAdsDll_LIBRARY}")
    if (WIN32)
        message(STATUS "Found TcAdsDll (implib): ${TcAdsDll_IMPLIB}")
    endif ()
    message(STATUS "Found TcAdsDll (include): ${TcAdsDll_INCLUDE_DIR}")
endif ()

