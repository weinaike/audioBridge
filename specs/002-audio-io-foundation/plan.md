# Implementation Plan: Audio I/O Foundation

**Branch**: `002-audio-io-foundation` | **Date**: 2025-12-22 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/002-audio-io-foundation/spec.md`

## Summary

构建跨平台实时音频处理引擎的基础 I/O 层，实现音频设备枚举、选择、拾音与播放功能。当前阶段使用 PortAudio + 第三方虚拟声卡（VB-Cable / BlackHole），为未来自研虚拟声卡驱动预留抽象接口。

## Technical Context

**Language/Version**: C++17
**Primary Dependencies**: PortAudio (音频I/O), spdlog (日志), CMake (构建系统)
**Storage**: N/A (实时音频处理，无持久化存储)
**Testing**: Google Test (单元测试), Google Benchmark (性能测试)
**Target Platform**: Windows (WASAPI), macOS (CoreAudio), Linux (ALSA/PulseAudio)
**Project Type**: single (C++ 音频引擎库 + 演示应用)
**Performance Goals**: <200ms 端到端延迟, 48kHz 采样率, 128 帧缓冲区, PCM float32 格式
**Constraints**: RT-safe (音频线程无动态内存分配、无阻塞I/O), Lock-free 数据结构, 预分配内存
**Scale/Scope**: 单应用程序，通过 IAudioInput/IAudioOutput 抽象层支持多种虚拟声卡驱动

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Real-Time Audio Safety ✅
- [x] No dynamic memory allocation in audio callback paths - 使用预分配 RingBuffer
- [x] No blocking I/O operations in real-time threads - 仅 lock-free 操作
- [x] All audio thread operations are lock-free and predictable - 使用 Lock-free RingBuffer

### Adapter Abstraction Layer ✅
- [x] All audio I/O uses IAudioInput/IAudioOutput interfaces - 架构设计已定义
- [x] No direct dependencies on PortAudio, VB-Cable, or specific drivers in business logic - 通过 Adapter 隔离
- [x] Clear separation between adapter layer and core engine - 三层架构已确定

### Cross-Platform Compatibility ✅
- [x] Core audio engine compiles on Windows, macOS, Linux without platform-specific code - PortAudio 统一 API
- [x] Platform-specific implementations isolated in adapter layer only - Adapter 层封装
- [x] Use of CMake for cross-platform build system - 已选定

### Low Latency Requirements ✅
- [x] Design demonstrates <200ms end-to-end latency capability - 48kHz/128帧 ≈ 2.67ms/buffer
- [x] Buffer sizes and processing paths optimized for minimal delay - 固定配置
- [x] Real-time monitoring of latency metrics included - spdlog 日志

### Test-First Audio Development ✅
- [x] Unit test strategy for all audio components - Google Test
- [x] Integration tests for complete audio pipeline - 端到端管线测试
- [x] Performance tests for real-time constraints and latency verification - Google Benchmark

## Project Structure

### Documentation (this feature)

```text
specs/002-audio-io-foundation/
├── spec.md              # 功能规格书
├── plan.md              # 本文件 - 实现计划
├── research.md          # 技术决策记录
├── data-model.md        # 数据模型定义
├── quickstart.md        # 快速启动指南
├── contracts/           # 接口契约定义
│   └── audio-interfaces.md
└── checklists/
    └── requirements.md  # 需求检查清单
```

### Source Code (repository root)

```text
# AudioBridge 项目结构 (基于架构设计文档)
src/
├── core/                    # Audio Engine Core - 音频引擎核心
│   ├── AudioPipeline.h/cpp  # 音频管线管理
│   ├── RingBuffer.h         # Lock-free 环形缓冲区
│   └── AudioEngine.h/cpp    # 引擎主控制器
├── adapters/                # Audio I/O Adapter Layer - 适配器抽象层
│   ├── IAudioInput.h        # 输入接口抽象
│   ├── IAudioOutput.h       # 输出接口抽象
│   ├── PortAudioInput.h/cpp # PortAudio 输入实现
│   └── PortAudioOutput.h/cpp # PortAudio 输出实现
├── processing/              # DSP/AI Processing - 处理模块
│   └── DummyEngine.h/cpp    # 当前阶段占位（直通）
├── utils/                   # Utilities - 工具类
│   ├── Logger.h/cpp         # spdlog 封装
│   └── DeviceEnumerator.h/cpp # 设备枚举工具
└── main.cpp                 # 演示应用入口

tests/
├── unit/                    # 单元测试
│   ├── test_ring_buffer.cpp
│   ├── test_audio_pipeline.cpp
│   └── test_device_enumerator.cpp
├── integration/             # 集成测试
│   └── test_audio_passthrough.cpp
└── performance/             # 性能测试
    └── benchmark_latency.cpp

cmake/                       # CMake 构建配置
├── FindPortAudio.cmake
└── CompilerFlags.cmake
```

**Structure Decision**: 基于架构设计文档的三层架构：应用层、Audio Engine Core、Audio I/O Adapter。符合宪法的适配器抽象原则，实现跨平台兼容性。

## Complexity Tracking

> 宪法检查全部通过，无需违规说明。

| 设计决策 | 理由 | 备选方案 |
|---------|------|---------|
| PortAudio 作为当前适配器 | 成熟的跨平台库，支持 WASAPI/CoreAudio/ALSA | 直接使用平台 API（复杂度高） |
| Lock-free RingBuffer | RT-safe 要求，避免音频线程阻塞 | std::queue + mutex（不满足 RT-safe） |
| 48kHz / 128帧 固定配置 | 简化实现，满足 <200ms 延迟要求 | 动态配置（增加复杂度） |
