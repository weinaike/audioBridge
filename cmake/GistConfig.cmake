# GistConfig.cmake
# Configuration for Gist audio analysis library (header-only)
#
# This is a header-only library, so we just need to find the header directory

# Try to find Gist in common locations
find_path(GIST_INCLUDE_DIR
    NAMES gist.h
    PATHS
        /usr/local/include
        /usr/include
        ${CMAKE_SOURCE_DIR}/third_party/gist
        ${CMAKE_SOURCE_DIR}/third_party/Gist/src
        ${CMAKE_SOURCE_DIR}/external/Gist/src
    DOC "Gist include directory"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gist
    REQUIRED_VARS GIST_INCLUDE_DIR
)

if(GIST_FOUND)
    # Gist is header-only, so we just set the include directory
    set(GIST_INCLUDE_DIRS ${GIST_INCLUDE_DIR})
    message(STATUS "Found Gist: ${GIST_INCLUDE_DIR}")

    # Create an imported target for Gist (interface library since it's header-only)
    if(NOT TARGET Gist::Gist)
        add_library(Gist::Gist INTERFACE IMPORTED)
        set_target_properties(Gist::Gist PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GIST_INCLUDE_DIR}"
        )
    endif()
else()
    # For now, we'll stub it out for testing purposes
    # In production, Gist should be properly installed
    message(WARNING "Gist library not found - some features may be disabled")

    # Create a stub target
    if(NOT TARGET Gist::Gist)
        add_library(Gist::Gist INTERFACE IMPORTED)
        set_target_properties(Gist::Gist PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/tests/tools/stubs"
        )
    endif()
endif()
