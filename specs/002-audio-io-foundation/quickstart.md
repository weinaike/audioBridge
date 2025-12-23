# Quick Start: Audio I/O Foundation

**Feature**: 002-audio-io-foundation
**Date**: 2025-12-22

## 前置条件

### 系统要求

| 平台 | 要求 |
| ---- | ---- |
| Windows | Windows 10+, WASAPI 支持 |
| macOS | macOS 10.14+, CoreAudio |
| Linux | ALSA 或 PulseAudio |

### 虚拟声卡安装

| 平台 | 软件 | 安装方式 |
| ---- | ---- | -------- |
| Windows | VB-Cable | https://vb-audio.com/Cable/ |
| macOS | BlackHole | `brew install blackhole-2ch` |
| Linux | N/A | 使用 PulseAudio 虚拟设备 |

### 开发环境

- C++17 兼容编译器 (GCC 8+, Clang 10+, MSVC 2019+)
- CMake 3.14+
- Git

---

## 快速构建

### 1. 克隆仓库

```bash
git clone <repository-url>
cd audioBridge
```

### 2. 安装依赖

**Ubuntu/Debian:**
```bash
sudo apt-get install libportaudio2 libportaudio-dev
```

**macOS:**
```bash
brew install portaudio
```

**Windows:**
- CMake 会通过 FetchContent 自动下载 PortAudio

### 3. 构建

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### 4. 运行测试

```bash
ctest --output-on-failure
```

---

## 基本用法

### 列出音频设备

```cpp
#include "audiobridge/AudioEngine.h"
#include <iostream>

int main() {
    auto engine = audiobridge::CreateAudioEngine();
    auto& enumerator = engine->GetDeviceEnumerator();

    std::cout << "=== Input Devices ===" << std::endl;
    for (const auto& device : enumerator.GetInputDevices()) {
        std::cout << "[" << device.deviceId << "] " << device.name;
        if (device.isDefaultInput) std::cout << " (default)";
        std::cout << std::endl;
    }

    std::cout << "\n=== Output Devices ===" << std::endl;
    for (const auto& device : enumerator.GetOutputDevices()) {
        std::cout << "[" << device.deviceId << "] " << device.name;
        if (device.isDefaultOutput) std::cout << " (default)";
        std::cout << std::endl;
    }

    return 0;
}
```

### 音频直通 (Pass-through)

```cpp
#include "audiobridge/AudioEngine.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    auto engine = audiobridge::CreateAudioEngine();
    auto& enumerator = engine->GetDeviceEnumerator();

    // 选择默认设备
    engine->SelectInputDevice(enumerator.GetDefaultInputDevice());
    engine->SelectOutputDevice(enumerator.GetDefaultOutputDevice());

    // 设置电平监控
    engine->SetLevelCallback([](const audiobridge::AudioLevels& levels) {
        std::cout << "\rInput: " << levels.inputPeakL
                  << " | Output: " << levels.outputPeakL << std::flush;
    });

    // 启动直通模式
    engine->SetPassThroughEnabled(true);

    if (!engine->Start()) {
        std::cerr << "Failed to start audio engine" << std::endl;
        return 1;
    }

    std::cout << "Audio pass-through active. Press Enter to stop..." << std::endl;
    std::cin.get();

    engine->Stop();
    return 0;
}
```

---

## 项目结构

```
audioBridge/
├── src/
│   ├── core/           # 音频引擎核心
│   ├── adapters/       # I/O 适配器
│   ├── processing/     # 处理模块
│   └── utils/          # 工具类
├── tests/
│   ├── unit/           # 单元测试
│   ├── integration/    # 集成测试
│   └── performance/    # 性能测试
├── cmake/              # CMake 模块
├── docs/               # 文档
└── specs/              # 功能规格
```

---

## 配置说明

### 音频参数 (固定)

| 参数 | 值 | 说明 |
| ---- | -- | ---- |
| 采样率 | 48000 Hz | 行业标准 |
| 缓冲区大小 | 128 帧 | 约 2.67ms 延迟 |
| 格式 | Float32 | PCM 浮点格式 |
| 通道 | 1-2 | 单声道或立体声 |

### 延迟预期

| 场景 | 预期延迟 |
| ---- | -------- |
| 单缓冲区 | ~2.67ms |
| 直通模式 (典型) | ~20-50ms |
| 端到端最大 | <200ms |

---

## 故障排除

### 无法检测到虚拟声卡

1. 确认虚拟声卡软件已安装并启动
2. Windows: 检查 VB-Cable 驱动状态
3. macOS: 运行 `brew services restart blackhole-2ch`

### 音频断续或卡顿

1. 检查 CPU 占用率
2. 关闭其他音频应用
3. 检查日志中的 buffer underrun 警告

### 延迟过高

1. 确认使用低延迟音频 API (WASAPI Exclusive, CoreAudio)
2. 减小其他应用的缓冲区设置
3. 检查系统音频设置

---

## 下一步

- 运行 `/speckit.tasks` 生成任务列表
- 阅读 `contracts/audio-interfaces.md` 了解接口详情
- 查看 `data-model.md` 了解数据结构
