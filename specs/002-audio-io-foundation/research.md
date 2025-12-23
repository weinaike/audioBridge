# Research: Audio I/O Foundation

**Feature**: 002-audio-io-foundation
**Date**: 2025-12-22
**Status**: Complete

## 技术决策汇总

本文档记录基于 `docs/requirement.md` 和 `docs/架构设计文档.md` 的技术决策。

---

## 1. 音频 I/O 库选择

### Decision: PortAudio

**Rationale**:
- 成熟的跨平台音频库，支持 Windows (WASAPI)、macOS (CoreAudio)、Linux (ALSA/PulseAudio/JACK)
- 提供统一的 API，业务代码无需处理平台差异
- RT-safe 设计，适合实时音频处理
- 开源且广泛使用，社区支持良好

**Alternatives Considered**:
- 直接使用平台原生 API：复杂度高，需要维护三套代码
- RtAudio：功能类似但社区较小
- JUCE：过于重量级，包含 GUI 等不需要的功能

---

## 2. 音频格式标准化

### Decision: PCM float32 @ 48kHz / 128帧

**Rationale**:
- **PCM float32**: 处理范围 [-1.0, 1.0]，避免整数溢出，便于 DSP 处理
- **48kHz**: 行业标准采样率，兼容大多数音频设备
- **128帧**: 约 2.67ms 延迟，平衡低延迟与 CPU 效率

**Alternatives Considered**:
- 16-bit/24-bit PCM：需要额外转换，精度损失
- 44.1kHz：兼容性略差，常见于 CD 音频
- 256/512帧：延迟增加，不满足实时要求

---

## 3. 线程安全模型

### Decision: Lock-free RingBuffer + 多线程

**Rationale**:
- **输入线程 (RT-safe)**: 从 PortAudio 回调接收音频，写入 RingBuffer
- **处理线程**: 从 RingBuffer 读取，处理后写入输出 RingBuffer
- **输出线程 (RT-safe)**: 从 RingBuffer 读取，发送到 PortAudio 回调
- 所有线程间通信使用 Lock-free RingBuffer，避免优先级反转

**Alternatives Considered**:
- 单线程处理：无法满足实时要求
- Mutex 同步：可能导致音频线程阻塞，违反 RT-safe 原则

---

## 4. 日志框架

### Decision: spdlog (异步模式)

**Rationale**:
- 高性能 C++ 日志库，支持异步日志
- 音频线程使用异步日志，避免阻塞
- 支持多种输出目标（控制台、文件）
- Header-only 或静态链接，易于集成

**Alternatives Considered**:
- printf/std::cout：阻塞 I/O，不适合音频线程
- log4cxx：依赖较重

---

## 5. 构建系统

### Decision: CMake

**Rationale**:
- 跨平台构建系统，支持 Windows/macOS/Linux
- 良好的 IDE 集成（Visual Studio、CLion、VS Code）
- 支持 FetchContent 管理依赖

**Alternatives Considered**:
- Meson：生态系统较小
- Bazel：学习曲线陡峭

---

## 6. 测试框架

### Decision: Google Test + Google Benchmark

**Rationale**:
- **Google Test**: 行业标准 C++ 测试框架，CMake 集成良好
- **Google Benchmark**: 精确的纳秒级性能测量，适合延迟测试

**Alternatives Considered**:
- Catch2：编译时间较长
- doctest：生态系统较小

---

## 7. 虚拟声卡策略

### Decision: 当前使用第三方，未来自研

**Current Phase**:
- Windows: VB-Cable
- macOS: BlackHole
- Linux: PulseAudio/JACK 路由

**Future Phase**:
- 自研虚拟声卡驱动
- 通过 IAudioInput/IAudioOutput 接口无缝切换
- 业务层无需修改

---

## 8. 适配器抽象设计

### Decision: IAudioInput / IAudioOutput 接口

**Rationale**:
- 业务逻辑仅依赖抽象接口
- PortAudioInput/PortAudioOutput 实现当前功能
- 未来 VirtualAudioAdapter 可替换实现
- 符合依赖倒置原则和宪法要求

**Interface Design**:
```
IAudioInput
├── Start() / Stop()
├── IsActive()
└── GetDeviceInfo()

IAudioOutput
├── Start() / Stop()
├── IsActive()
└── GetDeviceInfo()
```

---

## 待解决问题

无。所有技术决策均已基于现有文档确定。
