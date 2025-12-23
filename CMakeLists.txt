cmake_minimum_required(VERSION 3.14)
project(audioBridge VERSION 0.1.0 LANGUAGES C CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include compiler flags
include(cmake/CompilerFlags.cmake)

# Find dependencies using FetchContent
include(FetchContent)

# spdlog - Logging library
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Fix for Windows cross-compilation with MinGW-w64
# Use header-only mode to avoid Windows-specific threading issues
if(WIN32 AND CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling for Windows: using header-only spdlog")
    target_compile_definitions(spdlog INTERFACE
        SPDLOG_HEADER_ONLY
        SPDLOG_COMPILED_LIB
        SPDLOG_WCHAR_TO_UTF8_SUPPORT
    )
    # Disable Windows-specific features that cause issues with MinGW
    target_compile_definitions(spdlog INTERFACE
        SPDLOG_NO_EXCEPTIONS
    )
endif()

# GoogleTest - Testing framework
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Google Benchmark - Performance testing
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

# PortAudio - Cross-platform audio I/O
# For cross-compilation, we always use FetchContent to ensure proper configuration
if(WIN32)
    # Windows: Always use FetchContent for cross-compilation
    message(STATUS "Cross-compiling for Windows, using FetchContent for PortAudio")
    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG v19.7.0
    )
    FetchContent_MakeAvailable(portaudio)
    set(PORTAUDIO_TARGETS portaudio_static)
else()
    # Linux: Try to find system PortAudio first, if not found, use FetchContent
    find_package(PortAudio QUIET)

    if(NOT PortAudio_FOUND)
        message(STATUS "PortAudio not found on system, using FetchContent")
        FetchContent_Declare(
            portaudio
            GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
            GIT_TAG v19.7.0
        )
        FetchContent_MakeAvailable(portaudio)
        set(PORTAUDIO_TARGETS portaudio_static)
    else()
        message(STATUS "Found system PortAudio: ${PortAudio_LIBRARIES}")
        set(PORTAUDIO_TARGETS ${PortAudio_LIBRARIES})
    endif()
endif()

# Source files
set(CORE_SOURCES
    src/core/RingBuffer.h
    src/core/AudioPipeline.h
    src/core/AudioPipeline.cpp
    src/core/AudioEngine.h
    src/core/AudioEngine.cpp
    src/core/Factory.h
    src/core/Factory.cpp
    src/core/AudioDeviceInfo.h
    src/core/AudioLevels.h
    src/core/Types.h
)

set(ADAPTER_SOURCES
    src/adapters/IAudioInput.h
    src/adapters/IAudioOutput.h
    src/adapters/IDeviceEnumerator.h
    src/adapters/IAudioEngine.h
    src/adapters/PortAudioInput.h
    src/adapters/PortAudioInput.cpp
    src/adapters/PortAudioOutput.h
    src/adapters/PortAudioOutput.cpp
)

set(UTILS_SOURCES
    src/utils/Logger.h
    src/utils/Logger.cpp
    src/utils/DeviceEnumerator.h
    src/utils/DeviceEnumerator.cpp
    src/utils/PortAudioHelper.h
    src/utils/PortAudioHelper.cpp
)

set(PROCESSING_SOURCES
    src/processing/DummyEngine.h
    src/processing/DummyEngine.cpp
)

# Create audioBridge library
add_library(audioBridge_lib STATIC
    ${CORE_SOURCES}
    ${ADAPTER_SOURCES}
    ${UTILS_SOURCES}
    ${PROCESSING_SOURCES}
)

target_include_directories(audioBridge_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# Treat warnings as errors for our code only (GCC/Clang)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(audioBridge_lib PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
endif()

target_link_libraries(audioBridge_lib
    PUBLIC
        spdlog::spdlog
        ${PORTAUDIO_TARGETS}
)

# Demo application
add_executable(audioBridge
    src/main.cpp
)

# Treat warnings as errors for our code only (GCC/Clang)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(audioBridge PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
endif()

target_link_libraries(audioBridge
    PRIVATE
        audioBridge_lib
)

# Enable testing
enable_testing()

# GoogleTest unit tests
add_executable(unit_tests
    tests/unit/test_ring_buffer.cpp
    tests/unit/test_portaudio_input.cpp
    tests/unit/test_portaudio_output.cpp
    tests/unit/test_device_enumerator.cpp
    tests/unit/test_mock_audio.cpp
    tests/unit/test_virtual_audio.cpp
)

# Treat warnings as errors for our code only (GCC/Clang)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unit_tests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
endif()

target_link_libraries(unit_tests
    PRIVATE
        audioBridge_lib
        gtest_main
        gmock_main
)

# Add tests directory to include path for mocks
target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

include(GoogleTest)
gtest_discover_tests(unit_tests)

# Integration tests
add_executable(integration_tests
    tests/integration/test_audio_passthrough.cpp
    tests/integration/test_audio_capture.cpp
    tests/integration/test_audio_playback.cpp
)

# Treat warnings as errors for our code only (GCC/Clang)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(integration_tests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
endif()

target_link_libraries(integration_tests
    PRIVATE
        audioBridge_lib
        gtest_main
        gmock_main
)

gtest_discover_tests(integration_tests)

# Performance benchmarks
add_executable(latency_benchmark
    tests/performance/benchmark_latency.cpp
)

# Treat warnings as errors for our code only (GCC/Clang)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(latency_benchmark PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
endif()

target_link_libraries(latency_benchmark
    PRIVATE
        audioBridge_lib
        benchmark::benchmark
        benchmark::benchmark_main
)

# Installation
install(DIRECTORY src/ DESTINATION include/audioBridge)
install(TARGETS audioBridge audioBridge_lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "================== AudioBridge Configuration ==================")
message(STATUS "  Version:             ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:        ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:            ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Install Prefix:      ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  PortAudio:           ${PortAudio_LIBRARIES}")
message(STATUS "  spdlog:              spdlog::spdlog")
message(STATUS "  GoogleTest:          gtest_main")
message(STATUS "  Google Benchmark:    benchmark::benchmark")
message(STATUS "=============================================================")
message(STATUS "")
