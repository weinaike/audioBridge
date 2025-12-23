#!/bin/bash
# build_windows.sh - Cross-compile audioBridge for Windows on Linux
#
# This script compiles audioBridge for Windows using MinGW-w64 on Linux.
# The resulting executables can run directly on Windows without any DLLs.

set -e  # Exit on error

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build-windows"

echo "========================================"
echo "  Cross-compiling for Windows (MinGW)"
echo "========================================"
echo ""

# Check if MinGW-w64 is installed
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "❌ ERROR: MinGW-w64 not found!"
    echo ""
    echo "Install on Ubuntu/Debian:"
    echo "  sudo apt-get update"
    echo "  sudo apt-get install mingw-w64"
    echo ""
    echo "Install on Arch Linux:"
    echo "  sudo pacman -S mingw-w64-gcc"
    echo ""
    exit 1
fi

# Show compiler version
echo "✅ MinGW-w64 found:"
x86_64-w64-mingw32-g++ --version | head -1
echo ""

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "🧹 Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "📦 Configuring CMake for Windows..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/Mingw-w64-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF

echo ""
echo "🔨 Building for Windows..."
# Use number of CPU cores for parallel build
NPROC=$(nproc)
cmake --build . --config Release --parallel "$NPROC"

echo ""
echo "========================================"
echo "  ✅ Build Complete!"
echo "========================================"
echo ""
echo "Build directory: $BUILD_DIR"
echo ""

# List generated executables
echo "Generated Windows executables:"
echo "--------------------------------"
find . -name "*.exe" -type f | while read exe; do
    SIZE=$(ls -lh "$exe" | awk '{print $5}')
    echo "  ✓ $(basename $exe) ($SIZE)"
done

echo ""
echo "File type verification:"
echo "----------------------"
file build/audioBridge.exe 2>/dev/null || echo "  (audioBridge.exe not found)"

echo ""
echo "========================================"
echo "  Usage Instructions"
echo "========================================"
echo ""
echo "1. Copy to Windows:"
echo "   scp $BUILD_DIR/audioBridge.exe user@windows-machine:/C/Users/user/"
echo ""
echo "2. Or test with Wine on Linux:"
echo "   wine $BUILD_DIR/audioBridge.exe --help"
echo ""
echo "3. On Windows, simply run:"
echo "   audioBridge.exe --list"
echo "   audioBridge.exe --input 0 --output 1"
echo ""
echo "Note: The executables are statically linked and do not require any DLLs."
echo ""
