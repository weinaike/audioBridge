# Data Model: Audio I/O Foundation

**Feature**: 002-audio-io-foundation
**Date**: 2025-12-22

## 核心实体

### AudioDevice

表示系统中的物理或虚拟音频设备。

| 字段 | 类型 | 描述 |
| ---- | ---- | ---- |
| deviceId | int | PortAudio 设备索引 |
| name | string | 设备显示名称 |
| hostApi | string | 主机 API 名称 (WASAPI/CoreAudio/ALSA) |
| maxInputChannels | int | 最大输入通道数 |
| maxOutputChannels | int | 最大输出通道数 |
| defaultSampleRate | double | 默认采样率 |
| isDefaultInput | bool | 是否为系统默认输入设备 |
| isDefaultOutput | bool | 是否为系统默认输出设备 |

**验证规则**:
- deviceId >= 0
- maxInputChannels >= 0 或 maxOutputChannels >= 0
- defaultSampleRate > 0

---

### AudioStreamConfig

音频流配置参数。

| 字段 | 类型 | 描述 | 默认值 |
| ---- | ---- | ---- | ------ |
| sampleRate | double | 采样率 (Hz) | 48000.0 |
| framesPerBuffer | int | 每缓冲区帧数 | 128 |
| channelCount | int | 通道数 | 2 (立体声) |
| sampleFormat | enum | 采样格式 | Float32 |

**验证规则**:
- sampleRate == 48000.0 (当前阶段固定)
- framesPerBuffer == 128 (当前阶段固定)
- channelCount ∈ {1, 2}
- sampleFormat == Float32

---

### AudioBuffer

音频数据缓冲区。

| 字段 | 类型 | 描述 |
| ---- | ---- | ---- |
| data | float* | PCM float32 采样数据指针 |
| frameCount | size_t | 帧数 (每帧 = channelCount 个采样) |
| channelCount | int | 通道数 |
| timestamp | uint64_t | 时间戳 (可选，用于延迟测量) |

**内存布局**: 交错格式 (Interleaved)
- 立体声: [L0, R0, L1, R1, L2, R2, ...]
- 单声道: [S0, S1, S2, ...]

**验证规则**:
- data != nullptr
- frameCount > 0
- channelCount ∈ {1, 2}

---

### StreamState

音频流状态枚举。

| 值 | 描述 |
| -- | ---- |
| Stopped | 流已停止 |
| Starting | 流正在启动 |
| Active | 流正在运行 |
| Stopping | 流正在停止 |
| Error | 流发生错误 |

**状态转换**:
```
Stopped → Starting → Active → Stopping → Stopped
                ↓
              Error → Stopped
```

---

### DeviceConfiguration

用户设备配置。

| 字段 | 类型 | 描述 |
| ---- | ---- | ---- |
| inputDeviceId | int | 选定的输入设备 ID (-1 表示未选择) |
| outputDeviceId | int | 选定的输出设备 ID (-1 表示未选择) |
| passThroughEnabled | bool | 是否启用直通模式 |

**验证规则**:
- inputDeviceId != outputDeviceId (防止反馈回路)
- inputDeviceId == -1 或为有效输入设备
- outputDeviceId == -1 或为有效输出设备

---

## 核心接口

### IAudioInput

音频输入抽象接口。

```cpp
class IAudioInput {
public:
    virtual ~IAudioInput() = default;

    // 生命周期
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual StreamState GetState() const = 0;

    // 设备信息
    virtual AudioDevice GetDeviceInfo() const = 0;

    // 数据访问 (由引擎内部调用)
    virtual size_t AvailableFrames() const = 0;
    virtual size_t Read(AudioBuffer& buffer) = 0;
};
```

### IAudioOutput

音频输出抽象接口。

```cpp
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    // 生命周期
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual StreamState GetState() const = 0;

    // 设备信息
    virtual AudioDevice GetDeviceInfo() const = 0;

    // 数据访问 (由引擎内部调用)
    virtual size_t AvailableSpace() const = 0;
    virtual size_t Write(const AudioBuffer& buffer) = 0;
};
```

---

## 实体关系

```
DeviceConfiguration
    ├── inputDeviceId → AudioDevice (input)
    └── outputDeviceId → AudioDevice (output)

AudioEngine
    ├── IAudioInput (via inputDeviceId)
    ├── IAudioOutput (via outputDeviceId)
    ├── RingBuffer (input → processing)
    └── RingBuffer (processing → output)

RingBuffer
    └── AudioBuffer[] (预分配帧)
```

---

## 数据流

```
[Input Device]
    → PortAudio Callback (RT-safe)
    → RingBuffer.Write()
    → [Processing Thread]
    → RingBuffer.Read() / Write()
    → PortAudio Callback (RT-safe)
    → [Output Device]
```

**关键约束**:
1. PortAudio 回调中禁止内存分配
2. RingBuffer 操作必须 lock-free
3. 所有缓冲区在启动时预分配
