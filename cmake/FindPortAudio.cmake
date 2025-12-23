# FindPortAudio.cmake - Find PortAudio installation
#
# This module defines:
#  PortAudio_FOUND - True if PortAudio was found
#  PortAudio_INCLUDE_DIRS - Include directories for PortAudio
#  PortAudio_LIBRARIES - Libraries to link against
#  PortAudio_VERSION - Version of PortAudio

find_path(PortAudio_INCLUDE_DIR
    NAMES portaudio.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        C:/portaudio/include
        $ENV{PORTAUDIO_ROOT}/include
    DOC "PortAudio include directory"
)

find_library(PortAudio_LIBRARY
    NAMES portaudio
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        C:/portaudio/lib
        $ENV{PORTAUDIO_ROOT}/lib
    DOC "PortAudio library"
)

# Handle Windows specifics
if(WIN32)
    find_library(PortAudio_LIBRARY
        NAMES portaudio_x64 portaudio
        PATHS
            C:/portaudio/build/msvc/Win32/Release
            C:/portaudio/build/msvc/x64/Release
            $ENV{PORTAUDIO_ROOT}/lib
    )
endif()

# Handle macOS specifics
if(APPLE)
    find_library(PortAudio_LIBRARY
        NAMES portaudio
        PATHS
            /usr/local/lib
            /opt/local/lib
            $ENV{PORTAUDIO_ROOT}/lib
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PortAudio
    REQUIRED_VARS PortAudio_LIBRARY PortAudio_INCLUDE_DIR
)

if(PortAudio_FOUND)
    set(PortAudio_INCLUDE_DIRS ${PortAudio_INCLUDE_DIR})
    set(PortAudio_LIBRARIES ${PortAudio_LIBRARY})

    # Try to get version from portaudio.h
    if(EXISTS "${PortAudio_INCLUDE_DIR}/portaudio.h")
        file(READ "${PortAudio_INCLUDE_DIR}/portaudio.h" _pa_version_header)
        string(REGEX MATCH "define PA_MAJOR_VERSION[ \t]+([0-9]+)" _ ${_pa_version_header})
        set(PA_MAJOR ${CMAKE_MATCH_1})
        string(REGEX MATCH "define PA_MINOR_VERSION[ \t]+([0-9]+)" _ ${_pa_version_header})
        set(PA_MINOR ${CMAKE_MATCH_1})
        string(REGEX MATCH "define PA_MICRO_VERSION[ \t]+([0-9]+)" _ ${_pa_version_header})
        set(PA_MICRO ${CMAKE_MATCH_1})

        if(DEFINED PA_MAJOR AND DEFINED PA_MINOR AND DEFINED PA_MICRO)
            set(PortAudio_VERSION "${PA_MAJOR}.${PA_MINOR}.${PA_MICRO}")
        endif()
    endif()

    if(NOT TARGET PortAudio::PortAudio)
        add_library(PortAudio::PortAudio UNKNOWN IMPORTED)
        set_target_properties(PortAudio::PortAudio PROPERTIES
            IMPORTED_LOCATION "${PortAudio_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${PortAudio_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(PortAudio_INCLUDE_DIR PortAudio_LIBRARY)
