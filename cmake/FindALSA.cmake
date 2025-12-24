# FindALSA.cmake
# Find ALSA (Advanced Linux Sound Architecture) library
#
# This module defines:
#  ALSA_FOUND - True if ALSA is found
#  ALSA_INCLUDE_DIRS - Include directories for ALSA
#  ALSA_LIBRARIES - Libraries to link against
#  ALSA_VERSION - ALSA version string

find_path(ALSA_INCLUDE_DIR
    NAMES alsa/asoundlib.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
    DOC "ALSA include directory"
)

find_library(ALSA_LIBRARY
    NAMES asound
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/i386-linux-gnu
    DOC "ALSA library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ALSA
    REQUIRED_VARS ALSA_LIBRARY ALSA_INCLUDE_DIR
    VERSION_VAR ALSA_VERSION
)

if(ALSA_FOUND)
    set(ALSA_INCLUDE_DIRS ${ALSA_INCLUDE_DIR})
    set(ALSA_LIBRARIES ${ALSA_LIBRARY})
    set(ALSA_VERSION ${ALSA_VERSION})

    if(NOT TARGET ALSA::ALSA)
        add_library(ALSA::ALSA UNKNOWN IMPORTED)
        set_target_properties(ALSA::ALSA PROPERTIES
            IMPORTED_LOCATION "${ALSA_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ALSA_INCLUDE_DIR}"
        )
    endif()

    message(STATUS "Found ALSA: ${ALSA_LIBRARY}")
else()
    message(WARNING "ALSA not found. Virtual audio testing on Linux requires ALSA.")
endif()

mark_as_advanced(ALSA_INCLUDE_DIR ALSA_LIBRARY)
