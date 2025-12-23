# CompilerFlags.cmake - Cross-platform compiler settings for audioBridge

if(MSVC)
    # Microsoft Visual C++
    add_compile_options(/W4)  # High warning level
    add_compile_options(/WX)  # Treat warnings as errors
    add_compile_options(/MP)  # Multi-processor compilation

    # C++17 specific flags
    add_compile_options(/permissive-)  # Disable non-standard extensions

    # Optimization flags for Release builds
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")

    # Debug information
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")

    # Disable specific warnings that are not relevant
    add_compile_options(/wd4996)  # Disable deprecated POSIX function warnings

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # GCC
    add_compile_options(-Wall -Wextra -Wpedantic)

    # Treat warnings as errors for our code only (not for dependencies)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Werror>)

    # C++17 specific flags (C++ only)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-noexcept-type>)

    # Optimization flags
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

    # Debug symbols
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb")

    # Position independent code for shared libraries
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # Linker flags
    add_link_options(-pthread)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang (including Apple Clang)
    add_compile_options(-Wall -Wextra -Wpedantic)

    # Treat warnings as errors for our code only (not for dependencies)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Werror>)

    # C++17 specific flags (C++ only)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-gnu-anonymous-struct>)

    # Optimization flags
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

    # Debug symbols
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

    # Position independent code
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # Linker flags
    add_link_options(-pthread)

    # macOS specific
    if(APPLE)
        # Version 10.14 (Mojave) minimum for CoreAudio features
        add_compile_options(-mmacosx-version-min=10.14)
        add_link_options(-mmacosx-version-min=10.14)
        # Link frameworks needed for audio
        find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
        find_library(AUDIOUNIT_LIBRARY AudioUnit REQUIRED)
        find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox REQUIRED)
        link_libraries(${COREAUDIO_LIBRARY} ${AUDIOUNIT_LIBRARY} ${AUDIOTOOLBOX_LIBRARY})
    endif()
endif()

# Common defines for all compilers
add_definitions(-DAUDIOBRIDGE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_definitions(-DAUDIOBRIDGE_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_definitions(-DAUDIOBRIDGE_VERSION_PATCH=${PROJECT_VERSION_PATCH})

# RT-safe defines
add_definitions(-DRT_SAFE)
