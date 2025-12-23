# CMake toolchain file for cross-compiling to Windows using MinGW-w64
#
# Usage:
#   mkdir build-windows && cd build-windows
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Mingw-w64-x86_64.cmake

# Target system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)

# Compiler prefixes
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Target architecture
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Adjust search paths for libraries and headers
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set library and executable suffixes
set(CMAKE_EXECUTABLE_SUFFIX .exe)
set(CMAKE_STATIC_LIBRARY_SUFFIX .lib)
set(CMAKE_SHARED_LIBRARY_SUFFIX .dll)
set(CMAKE_IMPORT_LIBRARY_SUFFIX .dll.a)

# Static linking to avoid runtime DLL dependencies
# This creates standalone executables that don't require DLLs on Windows
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# Ensure we use the correct thread model for Windows
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")

# Remove -pthread flag as it conflicts with -mthreads on MinGW
# Replace with proper Windows threading flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")

# Add debug symbols for Release builds (useful for crash dumps)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")

# Platform-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows-specific definitions
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWINVER=0x0601)
    add_definitions(-D_WIN32_IE=0x0601)
endif()

# Print configuration for debugging
message(STATUS "Cross-compiling for Windows (x86_64)")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
