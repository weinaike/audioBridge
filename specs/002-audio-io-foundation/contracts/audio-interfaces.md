# Audio Interfaces Contract

**Feature**: 002-audio-io-foundation
**Date**: 2025-12-22

## 概述

本文档定义 Audio I/O Foundation 的 C++ 接口契约。所有接口遵循宪法中的适配器抽象原则。

---

## 1. 设备枚举接口

### IDeviceEnumerator

```cpp
namespace audiobridge {

struct AudioDeviceInfo {
    int deviceId;
    std::string name;
    std::string hostApi;
    int maxInputChannels;
    int maxOutputChannels;
    double defaultSampleRate;
    bool isDefaultInput;
    bool isDefaultOutput;
};

class IDeviceEnumerator {
public:
    virtual ~IDeviceEnumerator() = default;

    /// 获取所有可用设备列表
    /// @return 设备信息列表
    virtual std::vector<AudioDeviceInfo> GetAllDevices() = 0;

    /// 获取所有输入设备
    /// @return 仅包含支持输入的设备
    virtual std::vector<AudioDeviceInfo> GetInputDevices() = 0;

    /// 获取所有输出设备
    /// @return 仅包含支持输出的设备
    virtual std::vector<AudioDeviceInfo> GetOutputDevices() = 0;

    /// 刷新设备列表
    virtual void Refresh() = 0;

    /// 获取系统默认输入设备
    /// @return 设备 ID，-1 表示无可用设备
    virtual int GetDefaultInputDevice() = 0;

    /// 获取系统默认输出设备
    /// @return 设备 ID，-1 表示无可用设备
    virtual int GetDefaultOutputDevice() = 0;
};

} // namespace audiobridge
```

---

## 2. 音频流配置

### AudioStreamConfig

```cpp
namespace audiobridge {

enum class SampleFormat {
    Float32  // 当前仅支持 float32
};

struct AudioStreamConfig {
    double sampleRate = 48000.0;
    int framesPerBuffer = 128;
    int channelCount = 2;
    SampleFormat format = SampleFormat::Float32;

    /// 验证配置有效性
    bool IsValid() const {
        return sampleRate == 48000.0 &&
               framesPerBuffer == 128 &&
               (channelCount == 1 || channelCount == 2) &&
               format == SampleFormat::Float32;
    }
};

} // namespace audiobridge
```

---

## 3. 音频缓冲区

### AudioBuffer

```cpp
namespace audiobridge {

struct AudioBuffer {
    float* data;           // 交错格式音频数据
    size_t frameCount;     // 帧数
    int channelCount;      // 通道数
    uint64_t timestamp;    // 可选时间戳 (微秒)

    /// 获取总采样数
    size_t GetSampleCount() const {
        return frameCount * channelCount;
    }

    /// 获取数据大小 (字节)
    size_t GetByteSize() const {
        return GetSampleCount() * sizeof(float);
    }
};

} // namespace audiobridge
```

---

## 4. 流状态

### StreamState

```cpp
namespace audiobridge {

enum class StreamState {
    Stopped,    // 流已停止
    Starting,   // 流正在启动
    Active,     // 流正在运行
    Stopping,   // 流正在停止
    Error       // 流发生错误
};

/// 流状态变更回调
using StreamStateCallback = std::function<void(StreamState newState)>;

} // namespace audiobridge
```

---

## 5. 音频输入接口

### IAudioInput

```cpp
namespace audiobridge {

class IAudioInput {
public:
    virtual ~IAudioInput() = default;

    //--- 生命周期管理 ---

    /// 启动音频输入流
    /// @param deviceId 设备 ID
    /// @param config 流配置
    /// @return 成功返回 true
    virtual bool Open(int deviceId, const AudioStreamConfig& config) = 0;

    /// 启动流（Open 后调用）
    /// @return 成功返回 true
    virtual bool Start() = 0;

    /// 停止流
    virtual void Stop() = 0;

    /// 关闭流（释放资源）
    virtual void Close() = 0;

    //--- 状态查询 ---

    /// 获取当前流状态
    virtual StreamState GetState() const = 0;

    /// 检查流是否活动
    virtual bool IsActive() const = 0;

    /// 获取设备信息
    virtual AudioDeviceInfo GetDeviceInfo() const = 0;

    //--- 数据访问 ---

    /// 获取可读取的帧数
    /// @note RT-safe，可在音频线程调用
    virtual size_t AvailableFrames() const = 0;

    /// 读取音频数据
    /// @param buffer 目标缓冲区
    /// @return 实际读取的帧数
    /// @note RT-safe，可在音频线程调用
    virtual size_t Read(AudioBuffer& buffer) = 0;

    //--- 回调注册 ---

    /// 设置状态变更回调
    virtual void SetStateCallback(StreamStateCallback callback) = 0;
};

} // namespace audiobridge
```

---

## 6. 音频输出接口

### IAudioOutput

```cpp
namespace audiobridge {

class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    //--- 生命周期管理 ---

    /// 打开音频输出流
    /// @param deviceId 设备 ID
    /// @param config 流配置
    /// @return 成功返回 true
    virtual bool Open(int deviceId, const AudioStreamConfig& config) = 0;

    /// 启动流
    /// @return 成功返回 true
    virtual bool Start() = 0;

    /// 停止流
    virtual void Stop() = 0;

    /// 关闭流
    virtual void Close() = 0;

    //--- 状态查询 ---

    /// 获取当前流状态
    virtual StreamState GetState() const = 0;

    /// 检查流是否活动
    virtual bool IsActive() const = 0;

    /// 获取设备信息
    virtual AudioDeviceInfo GetDeviceInfo() const = 0;

    //--- 数据访问 ---

    /// 获取可写入的帧数
    /// @note RT-safe
    virtual size_t AvailableSpace() const = 0;

    /// 写入音频数据
    /// @param buffer 源缓冲区
    /// @return 实际写入的帧数
    /// @note RT-safe
    virtual size_t Write(const AudioBuffer& buffer) = 0;

    //--- 回调注册 ---

    /// 设置状态变更回调
    virtual void SetStateCallback(StreamStateCallback callback) = 0;
};

} // namespace audiobridge
```

---

## 7. 音频引擎接口

### IAudioEngine

```cpp
namespace audiobridge {

/// 音频电平信息
struct AudioLevels {
    float inputPeakL;   // 左声道输入峰值 [0.0, 1.0]
    float inputPeakR;   // 右声道输入峰值
    float outputPeakL;  // 左声道输出峰值
    float outputPeakR;  // 右声道输出峰值
};

/// 电平回调
using LevelCallback = std::function<void(const AudioLevels& levels)>;

class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    //--- 设备管理 ---

    /// 获取设备枚举器
    virtual IDeviceEnumerator& GetDeviceEnumerator() = 0;

    /// 选择输入设备
    /// @return 成功返回 true
    virtual bool SelectInputDevice(int deviceId) = 0;

    /// 选择输出设备
    /// @return 成功返回 true
    virtual bool SelectOutputDevice(int deviceId) = 0;

    //--- 引擎控制 ---

    /// 启动音频引擎
    /// @return 成功返回 true
    virtual bool Start() = 0;

    /// 停止音频引擎
    virtual void Stop() = 0;

    /// 检查引擎是否运行
    virtual bool IsRunning() const = 0;

    //--- Pass-through 模式 ---

    /// 启用/禁用直通模式
    virtual void SetPassThroughEnabled(bool enabled) = 0;

    /// 检查直通模式状态
    virtual bool IsPassThroughEnabled() const = 0;

    //--- 监控 ---

    /// 设置电平回调
    virtual void SetLevelCallback(LevelCallback callback) = 0;

    /// 获取当前延迟 (毫秒)
    virtual double GetCurrentLatency() const = 0;
};

} // namespace audiobridge
```

---

## 8. 工厂函数

```cpp
namespace audiobridge {

/// 创建 PortAudio 设备枚举器
std::unique_ptr<IDeviceEnumerator> CreatePortAudioEnumerator();

/// 创建 PortAudio 输入适配器
std::unique_ptr<IAudioInput> CreatePortAudioInput();

/// 创建 PortAudio 输出适配器
std::unique_ptr<IAudioOutput> CreatePortAudioOutput();

/// 创建音频引擎
std::unique_ptr<IAudioEngine> CreateAudioEngine();

} // namespace audiobridge
```

---

## 使用示例

```cpp
#include "audiobridge/AudioEngine.h"

int main() {
    auto engine = audiobridge::CreateAudioEngine();

    // 枚举设备
    auto& enumerator = engine->GetDeviceEnumerator();
    auto inputs = enumerator.GetInputDevices();
    auto outputs = enumerator.GetOutputDevices();

    // 选择设备
    if (!inputs.empty() && !outputs.empty()) {
        engine->SelectInputDevice(inputs[0].deviceId);
        engine->SelectOutputDevice(outputs[0].deviceId);
    }

    // 设置电平回调
    engine->SetLevelCallback([](const audiobridge::AudioLevels& levels) {
        printf("Input: L=%.2f R=%.2f\n", levels.inputPeakL, levels.inputPeakR);
    });

    // 启动直通模式
    engine->SetPassThroughEnabled(true);
    engine->Start();

    // ... 运行 ...

    engine->Stop();
    return 0;
}
```
