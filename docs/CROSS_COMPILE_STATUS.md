# 交叉编译环境设置完成报告

## ✅ 已完成的设置

### 1. 验证 MinGW-w64 安装 ✅
```bash
x86_64-w64-mingw32-g++ (GCC) 13-win32
```

### 2. 创建的文件

#### ✅ CMake 工具链文件
- **文件**: `cmake/Mingw-w64-x86_64.cmake`
- **功能**: 定义 Windows 交叉编译配置
- **内容**:
  - 编译器设置 (x86_64-w64-mingw32-gcc/g++)
  - 静态链接配置（无需 DLL）
  - Windows 特定宏定义

#### ✅ Windows 构建脚本
- **文件**: `scripts/build_windows.sh`
- **功能**: 自动化 Windows 交叉编译
- **使用**: `./scripts/build_windows.sh`

#### ✅ CMakeLists.txt 修改
- **修改**: PortAudio 检测逻辑
- **改进**: Windows 交叉编译时自动使用 FetchContent

### 3. 当前状态

**环境配置**: ✅ 完成
**编译脚本**: ✅ 完成
**首次编译**: ⚠️ PortAudio 依赖问题

---

## ⚠️ 遇到的问题

### PortAudio Windows 依赖问题

**错误**:
```
error: 'MMSYSERR_NOERROR' undeclared
error: 'TIMERR_NOERROR' undeclared
```

**原因**: MinGW-w64 缺少完整的 Windows SDK 头文件

---

## 🔄 解决方案

### 方案 1: 使用预编译的 PortAudio（推荐）

对于快速测试和开发，可以暂时禁用 PortAudio：

#### 1.1 创建简化版本（无 PortAudio）

创建 `cmake/Mingw-w64-x86_64-simple.cmake`:

```cmake
# 简化版工具链文件 - 用于测试交叉编译（不含 PortAudio）

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_EXECUTABLE_SUFFIX .exe)
set(CMAKE_STATIC_LIBRARY_SUFFIX .lib)
set(CMAKE_SHARED_LIBRARY_SUFFIX .dll)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
```

#### 1.2 创建测试程序验证交叉编译

创建 `src/test_cross_compile.cpp`:

```cpp
#include <iostream>

int main() {
    std::cout << "Hello from Windows (cross-compiled from Linux)!" << std::endl;
    std::cout << "Compiler: ";
    #ifdef __MINGW64__
    std::cout << "MinGW-w64" << std::endl;
    #endif
    return 0;
}
```

#### 1.3 测试编译

```bash
mkdir build-windows-test && cd build-windows-test
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Mingw-w64-x86_64-simple.cmake
x86_64-w64-mingw32-g++ ../src/test_cross_compile.cpp -o test.exe
file test.exe
# 输出应该是: test.exe: PE32+ executable (console) x86-64
```

### 方案 2: 安装完整的 MinGW-w64 SDK

```bash
# Ubuntu/Debian
sudo apt-get install mingw-w64 mingw-w64-tools mingw-w64-x86-64-dev

# 或者从源码编译 MinGW-w64
git clone https://github.com/mirror/mingw-w64.git
cd mingw-w64
mkdir build && cd build
../mingw-w64/mingw-w64-cmake/configure --prefix=/usr \
    --enable-sdk=all \
    --enable-threads=win32
make -j$(nproc)
sudo make install
```

### 方案 3: 使用 Docker 完整构建环境

创建 `docker/Dockerfile.mingw-full`:

```dockerfile
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    mingw-w64 \
    mingw-w64-tools \
    mingw-w64-x86-64-dev \
    wine64 \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /project
```

使用 Docker 构建:

```bash
docker build -t audiobridge-mingw-full -f docker/Dockerfile.mingw-full .

docker run --rm -v $(pwd):/project audiobridge-mingw-full bash -c "
  mkdir build && cd build &&
  cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Mingw-w64-x86_64.cmake &&
  make -j\$(nproc)
"
```

### 方案 4: 使用 vcpkg 管理依赖

```bash
# 安装 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# 添加 MinGW 三元组
./vcpkg integrate install

# 安装 PortAudio for Windows
./vcpkg install portaudio:x64-mingw-static
```

---

## 📋 当前环境总结

### ✅ 已安装
- MinGW-w64 GCC 13.0.0
- CMake 3.30+
- 所有必要的脚本和配置文件

### ⏳ 需要额外设置
- PortAudio Windows 头文件（可选）
- 或使用预编译 PortAudio 库
- 或使用 Docker 完整环境

### 📝 建议的下一步

1. **立即测试**: 使用方案 1 测试基础交叉编译功能
2. **短期解决**: 使用 Docker 环境（方案 3）获得完整依赖
3. **长期方案**: 配置 vcpkg（方案 4）管理 Windows 依赖

---

## 🎯 快速验证命令

### 验证 MinGW-w64 可用

```bash
x86_64-w64-mingw32-g++ --version
```

### 测试简单程序

```bash
cd /home/wnk/code/audioBridge
cat > test_simple.cpp << 'EOF'
#include <iostream>
int main() {
    std::cout << "Cross-compile test!" << std::endl;
    return 0;
}
EOF

x86_64-w64-mingw32-g++ test_simple.cpp -o test_simple.exe
file test_simple.exe
# 应该显示: PE32+ executable (console) x86-64
```

### 使用 Wine 测试（可选）

```bash
sudo apt-get install wine64
wine test_simple.exe
```

---

## 📚 参考文档

详细交叉编译指南请参考:
- `docs/CROSS_COMPILATION_WINDOWS.md` - 完整交叉编译文档
- `cmake/Mingw-w64-x86_64.cmake` - CMake 工具链配置
- `scripts/build_windows.sh` - 自动化构建脚本

---

## 总结

✅ **交叉编译环境基础设置完成**

**状态**:
- ✅ MinGW-w64 编译器已安装
- ✅ CMake 工具链文件已创建
- ✅ 构建脚本已创建
- ⚠️ PortAudio 依赖需要额外处理

**推荐行动**:
1. 先用简单程序验证交叉编译功能
2. 使用 Docker 获得完整构建环境
3. 或配置 vcpkg 管理依赖

交叉编译环境的**基础设施已经完成**，剩下的是处理 PortAudio 依赖的问题。这可以通过多种方式解决。
