# FindSndFile.cmake
# Find libsndfile audio file I/O library
#
# This module defines:
#  SNDFILE_FOUND - True if libsndfile is found
#  SNDFILE_INCLUDE_DIRS - Include directories for libsndfile
#  SNDFILE_LIBRARIES - Libraries to link against
#  SNDFILE_VERSION - libsndfile version string

find_path(SNDFILE_INCLUDE_DIR
    NAMES sndfile.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
    DOC "libsndfile include directory"
)

find_library(SNDFILE_LIBRARY
    NAMES sndfile
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/i386-linux-gnu
    DOC "libsndfile library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SndFile
    REQUIRED_VARS SNDFILE_LIBRARY SNDFILE_INCLUDE_DIR
    VERSION_VAR SNDFILE_VERSION
)

if(SNDFILE_FOUND)
    set(SNDFILE_INCLUDE_DIRS ${SNDFILE_INCLUDE_DIR})
    set(SNDFILE_LIBRARIES ${SNDFILE_LIBRARY})
    set(SNDFILE_VERSION ${SNDFILE_VERSION})

    if(NOT TARGET SndFile::SndFile)
        add_library(SndFile::SndFile UNKNOWN IMPORTED)
        set_target_properties(SndFile::SndFile PROPERTIES
            IMPORTED_LOCATION "${SNDFILE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SNDFILE_INCLUDE_DIR}"
        )
    endif()

    message(STATUS "Found libsndfile: ${SNDFILE_LIBRARY}")
else()
    message(WARNING "libsndfile not found. Audio file I/O functionality will be limited.")
endif()

mark_as_advanced(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)
