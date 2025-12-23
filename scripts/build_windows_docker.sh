#!/bin/bash
# build_windows_docker.sh - Build Windows executables using Docker
#
# This script uses Docker to create a complete MinGW-w64 build environment
# with all necessary dependencies for cross-compiling audioBridge.

set -e  # Exit on error

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build-windows"
DOCKER_IMAGE="audiobridge-mingw:latest"

echo "========================================"
echo "  Docker-based Windows Cross-Compilation"
echo "========================================"
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo "❌ ERROR: Docker not found!"
    echo ""
    echo "Install Docker:"
    echo "  curl -fsSL https://get.docker.com -o get-docker.sh"
    echo "  sudo sh get-docker.sh"
    echo "  sudo usermod -aG docker \$USER"
    exit 1
fi

# Check if Docker is running
if ! docker info &> /dev/null; then
    echo "❌ ERROR: Docker is not running!"
    echo "Please start Docker and try again."
    exit 1
fi

echo "✅ Docker is installed and running"
docker --version
echo ""

# Step 1: Build Docker image
echo "📦 Building Docker image with MinGW-w64..."
echo "This may take a few minutes on first run..."
echo ""

if ! docker image inspect "$DOCKER_IMAGE" &> /dev/null; then
    docker build -t "$DOCKER_IMAGE" -f "$PROJECT_DIR/docker/Dockerfile.mingw" "$PROJECT_DIR"
    echo ""
    echo "✅ Docker image built successfully"
else
    echo "✅ Docker image already exists (skipping build)"
    echo "  To rebuild: docker build -t $DOCKER_IMAGE -f docker/Dockerfile.mingw ."
fi

echo ""

# Step 2: Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "🧹 Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Step 3: Run build in Docker
echo "🔨 Building audioBridge for Windows in Docker..."
echo ""

docker run --rm \
    -v "$PROJECT_DIR:/project" \
    -w /project \
    "$DOCKER_IMAGE" \
    bash -c "
        echo '========================================'
        echo '  Build Environment Information'
        echo '========================================'
        echo ''
        echo 'Compiler:'
        x86_64-w64-mingw32-g++ --version | head -1
        echo ''
        echo 'CMake:'
        cmake --version | head -1
        echo ''
        echo '========================================'
        echo ''

        # Create build directory
        mkdir -p build-windows
        cd build-windows

        # Configure CMake
        echo 'Configuring CMake...'
        cmake .. \
            -DCMAKE_TOOLCHAIN_FILE=../cmake/Mingw-w64-x86_64.cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=OFF \
            -G Ninja

        echo ''
        echo 'Building...'
        cmake --build . --config Release --parallel \$(nproc)

        echo ''
        echo '========================================'
        echo '  Build Complete!'
        echo '========================================'
    "

echo ""
echo "========================================"
echo "  ✅ Docker Build Complete!"
echo "========================================"
echo ""

# List generated executables
if [ -d "$BUILD_DIR" ]; then
    echo "Generated Windows executables:"
    echo "--------------------------------"
    find "$BUILD_DIR" -name "*.exe" -type f | while read exe; do
        SIZE=$(ls -lh "$exe" | awk '{print $5}')
        echo "  ✓ $(basename $exe) ($SIZE)"
    done
    echo ""

    # Verify file type
    if [ -f "$BUILD_DIR/audioBridge.exe" ]; then
        echo "File type verification:"
        echo "----------------------"
        file "$BUILD_DIR/audioBridge.exe"
        echo ""
    fi
else
    echo "⚠️  Build directory not found. Check for errors above."
fi

echo "========================================"
echo "  Usage Instructions"
echo "========================================"
echo ""
echo "1. Copy to Windows:"
echo "   scp $BUILD_DIR/audioBridge.exe user@windows-machine:/C/Users/user/"
echo ""
echo "2. Or create ZIP archive:"
echo "   cd $BUILD_DIR"
echo "   zip -r audioBridge-windows-x64.zip *.exe"
echo ""
echo "3. On Windows, simply run:"
echo "   audioBridge.exe --list"
echo "   audioBridge.exe --input 0 --output 1"
echo ""
echo "Note: Executables are statically linked and require no DLLs."
echo ""
