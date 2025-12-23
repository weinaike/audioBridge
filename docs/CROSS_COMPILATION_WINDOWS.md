# 在 Linux 上交叉编译 Windows 程序

## 概述

在 Linux 上编译 Windows 程序称为**交叉编译**（Cross-compilation）。对于 audioBridge 项目，有几种可行的方法。

---

## 方法 1: MinGW-w64（推荐）

MinGW-w64 是最流行的 Windows 交叉编译工具链，可以在 Linux 上编译 Windows 程序。

### 安装 MinGW-w64

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y mingw-w64
```

#### Arch Linux
```bash
sudo pacman -S mingw-w64-gcc
```

#### Fedora/RHEL
```bash
sudo dnf install mingw64-gcc
```

### 验证安装

```bash
# 检查 64 位 Windows 编译器
x86_64-w64-mingw32-g++ --version

# 检查 32 位 Windows 编译器（可选）
i686-w64-mingw32-g++ --version
```

**预期输出**:
```
x86_64-w64-mingw32-g++ (GCC) 11.4.0
Copyright (C) 2021 Free Software Foundation, Inc.
...
```

---

## 方法 2: 使用 CMake 交叉编译

### 为 audioBridge 项目创建 Windows 构建配置

#### 1. 创建工具链文件

创建文件 `cmake/Mingw-w64-x86_64.cmake`:

```cmake
# CMake toolchain file for cross-compiling to Windows using MinGW-w64

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)

# 指定编译器前缀
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# 目标架构
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 调整查找路径
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 设置库和可执行文件后缀
set(CMAKE_EXECUTABLE_SUFFIX .exe)
set(CMAKE_STATIC_LIBRARY_SUFFIX .lib)
set(CMAKE_SHARED_LIBRARY_SUFFIX .dll)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
```

#### 2. 创建 Windows 构建脚本

创建 `scripts/build_windows.sh`:

```bash
#!/bin/bash
# build_windows.sh - Cross-compile audioBridge for Windows on Linux

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build-windows"

echo "========================================"
echo "  Cross-compiling for Windows (MinGW)"
echo "========================================"
echo ""

# 检查 MinGW-w64 是否安装
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "❌ ERROR: MinGW-w64 not found!"
    echo ""
    echo "Install with:"
    echo "  sudo apt-get install mingw-w64"
    exit 1
fi

echo "✅ MinGW-w64 found:"
x86_64-w64-mingw32-g++ --version | head -1
echo ""

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置 CMake（使用工具链文件）
echo "📦 Configuring CMake..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/Mingw-w64-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF

# 编译
echo "🔨 Building..."
cmake --build . --config Release --parallel $(nproc)

echo ""
echo "========================================"
echo "  ✅ Build Complete!"
echo "========================================"
echo ""
echo "Output directory: $BUILD_DIR"
echo ""

# 列出生成的文件
echo "Generated executables:"
find . -name "*.exe" -type f -exec ls -lh {} \;

echo ""
echo "Dependencies:"
echo "  - audioBridge.exe (standalone, no DLLs needed)"
echo "  - unit_tests.exe (test suite)"
echo ""
echo "Run on Windows:"
echo "  Simply copy .exe files to Windows and run!"
```

赋予执行权限:
```bash
chmod +x scripts/build_windows.sh
```

#### 3. 执行 Windows 构建

```bash
cd /home/wnk/code/audioBridge
./scripts/build_windows.sh
```

**预期输出**:
```
========================================
  Cross-compiling for Windows (MinGW)
========================================

✅ MinGW-w64 found:
x86_64-w64-mingw32-g++ (GCC) 11.4.0

📦 Configuring CMake...
🔨 Building...

========================================
  ✅ Build Complete!
========================================

Generated executables:
-rwxr-xr-x 1 user user 1.2M ... audioBridge.exe
-rwxr-xr-x 1 user user 3.5M ... unit_tests.exe
```

---

## 方法 3: 使用 Docker 隔离构建环境

### 创建 Dockerfile

创建 `docker/Dockerfile.mingw`:

```dockerfile
FROM ubuntu:22.04

# 避免交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 安装依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    mingw-w64 \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /project

# 默认命令
CMD ["bash"]
```

### 构建和运行 Docker 容器

```bash
# 构建镜像
docker build -t audiobridge-mingw -f docker/Dockerfile.mingw .

# 运行容器并编译
docker run --rm -v $(pwd):/project audiobridge-mingw \
    bash -c "
    mkdir build && cd build &&
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Mingw-w64-x86_64.cmake &&
    make -j\$(nproc)
    "
```

---

## 方法 4: 使用 GitHub Actions 自动构建

### 创建 GitHub Actions 工作流

创建 `.github/workflows/build-windows.yml`:

```yaml
name: Build Windows (Cross-Compile)

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]
  workflow_dispatch:

jobs:
  build-windows:
    runs-on: ubuntu-latest

    steps:
    - name: Checkout code
      uses: actions/checkout@v3

    - name: Install MinGW-w64
      run: |
        sudo apt-get update
        sudo apt-get install -y mingw-w64

    - name: Configure CMake
      run: |
        mkdir build
        cd build
        cmake .. \
          -DCMAKE_TOOLCHAIN_FILE=cmake/Mingw-w64-x86_64.cmake \
          -DCMAKE_BUILD_TYPE=Release

    - name: Build
      run: |
        cd build
        cmake --build . --config Release -j$(nproc)

    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: audioBridge-windows-x64
        path: |
          build/audioBridge.exe
          build/unit_tests.exe

    - name: Release (if tagged)
      if: startsWith(github.ref, 'refs/tags/')
      uses: softprops/action-gh-release@v1
      with:
        files: |
          build/audioBridge.exe
          build/unit_tests.exe
        draft: false
        prerelease: true
      env:
        GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

---

## 特殊注意事项

### PortAudio 在 Windows 上的支持

PortAudio 支持 Windows，但交叉编译时需要注意：

#### 选项 1: 使用预编译的 PortAudio

下载预编译的 PortAudio 库：
```bash
# 创建目录
mkdir -p build-windows/external/portaudio

# 下载预编译库（从 PortAudio 官网）
# 或者从 vcpkg 获取
```

#### 选项 2: 交叉编译 PortAudio

创建脚本 `scripts/build_portaudio_windows.sh`:

```bash
#!/bin/bash
# 交叉编译 PortAudio for Windows

set -e

PORTAUDIO_VERSION="v19.7.0"
BUILD_DIR="$PWD/build-portaudio"
INSTALL_DIR="$PWD/build-windows/external"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 下载 PortAudio
if [ ! -d "portaudio" ]; then
    git clone --depth 1 --branch $PORTAUDIO_VERSION https://github.com/PortAudio/portaudio.git
fi

cd portaudio

# 使用 MinGW 编译
cmake -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE=../../cmake/Mingw-w64-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DPA_BUILD_SHARED=OFF \
    -DPA_BUILD_STATIC=ON \
    -DPA_USE_WMME=ON \
    -DPA_USE_DS=OFF \
    -DPA_USE_ASIO=OFF \
    -DPA_USE_WASAPI=ON \
    -DPA_USE_WDMKS=OFF \
    BUILDDIR=build

cmake --build build --parallel $(nproc)
cmake --install build

echo "✅ PortAudio installed to: $INSTALL_DIR"
```

### 修改 CMakeLists.txt 以支持交叉编译

更新 `CMakeLists.txt`:

```cmake
# PortAudio - Cross-platform audio I/O
if(WIN32)
    # Windows: 使用预编译或交叉编译的 PortAudio
    find_package(PortAudio REQUIRED HINTS
        ${CMAKE_SOURCE_DIR}/external/portaudio
        $ENV{PORTAUDIO_DIR}
    )
    set(PORTAUDIO_TARGETS PortAudio::PortAudioStatic)
else()
    # Linux: 使用系统 PortAudio 或 FetchContent
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
```

---

## 测试 Windows 构建

### 在 Linux 上验证

```bash
# 检查可执行文件类型
file build-windows/audioBridge.exe

# 预期输出:
# audioBridge.exe: PE32+ executable (console) x86-64, for MS Windows
```

### 在 Windows 上运行

1. **复制文件到 Windows**:
   ```bash
   # 通过网络共享、USB 或云存储
   scp build-windows/audioBridge.exe user@windows-machine:/c/Users/user/
   ```

2. **或使用 Wine 测试**:
   ```bash
   # 安装 Wine
   sudo apt-get install wine64 wine32

   # 运行 Windows 可执行文件
   wine build-windows/audioBridge.exe --help
   ```

---

## 限制和注意事项

### 1. 音频驱动程序支持

- **Linux → Windows**: PortAudio 支持不同后端
  - Linux: ALSA, JACK, PulseAudio
  - Windows: MME, DirectSound, WASAPI, ASIO
- **交叉编译限制**: 某些特定功能可能无法测试

### 2. 依赖库

需要确保所有依赖库都支持 Windows:
- ✅ PortAudio: 支持
- ✅ spdlog: 支持（仅头文件）
- ✅ Google Test: 支持
- ⚠️ 其他 C++ 库: 需要验证 Windows 支持

### 3. 静态链接 vs 动态链接

**推荐**: 静态链接以避免 DLL 依赖

```cmake
# 在工具链文件中
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")
```

### 4. 调试和测试

- **单元测试**: 可以在 Windows 上运行
- **集成测试**: 需要真实 Windows 环境
- **音频测试**: 需要音频硬件

---

## 完整示例：构建和打包

### 创建发布脚本

创建 `scripts/package_windows.sh`:

```bash
#!/bin/bash
# package_windows.sh - Build and package Windows release

set -e

PROJECT_DIR="$(dirname "$(dirname "$0")")"
cd "$PROJECT_DIR"

# 1. 构建 Windows 版本
./scripts/build_windows.sh

# 2. 创建发布包
RELEASE_DIR="release-windows"
VERSION="0.1.0"

mkdir -p "$RELEASE_DIR"

# 复制可执行文件
cp build-windows/audioBridge.exe "$RELEASE_DIR/"
cp build-windows/unit_tests.exe "$RELEASE_DIR/"

# 创建 README
cat > "$RELEASE_DIR/README.txt" << 'EOF'
audioBridge for Windows
======================

Version: 0.1.0
Build Date: $(date)

Files:
------
- audioBridge.exe    : Main application
- unit_tests.exe     : Test suite

Usage:
------
List devices:
  audioBridge.exe --list

Run pass-through:
  audioBridge.exe --input 0 --output 1

Run tests:
  unit_tests.exe

Requirements:
------------
- Windows 7 or later
- Audio device (sound card)

Notes:
------
- This is a cross-compiled build from Linux
- No additional DLLs required (statically linked)
- For issues, please report to: https://github.com/yourusername/audioBridge
EOF

# 创建 ZIP 包
cd "$RELEASE_DIR"
zip -r "../audioBridge-windows-x64-${VERSION}.zip" .

echo ""
echo "✅ Release package created: audioBridge-windows-x64-${VERSION}.zip"
echo ""
ls -lh "../audioBridge-windows-x64-${VERSION}.zip"
```

---

## 故障排除

### 问题 1: 找不到 MinGW 编译器

**错误**:
```
CMake Error: Could not find compiler
```

**解决方案**:
```bash
sudo apt-get install mingw-w64
```

### 问题 2: PortAudio 编译失败

**错误**:
```
PortAudio configuration failed
```

**解决方案**:
- 使用预编译的 PortAudio
- 或使用 vcpkg 管理依赖

### 问题 3: Windows 上运行时缺少 DLL

**解决方案**:
```cmake
# 静态链接所有内容
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
```

---

## 推荐工作流

### 开发阶段
1. ✅ 在 Linux 上开发和测试（原生）
2. ✅ 使用 Mock 测试验证逻辑
3. ✅ 定期交叉编译验证

### 发布阶段
1. ✅ 交叉编译 Windows 版本
2. ✅ 在 Wine 中快速验证
3. ✅ 在真实 Windows 上测试
4. ✅ 打包发布

---

## 总结

**可行性**: ✅ 完全可行

**推荐方法**:
1. **简单快速**: MinGW-w64 + CMake 工具链文件
2. **隔离环境**: Docker + MinGW-w64
3. **自动化**: GitHub Actions

**优势**:
- ✅ 无需 Windows 许可证
- ✅ Linux 开发环境
- ✅ 自动化 CI/CD
- ✅ 一致的可重复构建

**限制**:
- ⚠️ 需要在真实 Windows 上最终测试
- ⚠️ 音频 API 差异需要适配
- ⚠️ 某些调试困难

对于 audioBridge 项目，**强烈推荐使用 MinGW-w64 交叉编译**，因为它成熟、稳定且文档完善。
