# FindGist.cmake
# Find Gist audio analysis library (header-only)
#
# This module defines:
#  GIST_FOUND - True if Gist is found
#  GIST_INCLUDE_DIRS - Include directories for Gist
#  GIST_VERSION - Gist version string

find_path(GIST_INCLUDE_DIR
    NAMES Gist.h
    PATHS
        ${CMAKE_SOURCE_DIR}/third_party/gist
        /usr/include
        /usr/local/include
        /opt/local/include
    DOC "Gist include directory"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gist
    REQUIRED_VARS GIST_INCLUDE_DIR
    VERSION_VAR GIST_VERSION
)

if(GIST_FOUND)
    set(GIST_INCLUDE_DIRS ${GIST_INCLUDE_DIR})
    set(GIST_VERSION ${GIST_VERSION})

    if(NOT TARGET Gist::Gist)
        add_library(Gist::Gist INTERFACE IMPORTED)
        set_target_properties(Gist::Gist PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GIST_INCLUDE_DIR}"
        )
    endif()

    message(STATUS "Found Gist (header-only): ${GIST_INCLUDE_DIR}")
else()
    message(WARNING "Gist not found. Audio validation features will be limited.")
endif()

mark_as_advanced(GIST_INCLUDE_DIR)
