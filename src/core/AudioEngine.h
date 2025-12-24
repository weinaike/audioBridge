#ifndef AUDIOBRIDGE_AUDIOENGINE_H
#define AUDIOBRIDGE_AUDIOENGINE_H

#include "adapters/IAudioEngine.h"
#include "core/AudioPipeline.h"
#include "core/Types.h"
#include "utils/DeviceEnumerator.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

namespace audiobridge {

// Forward declarations
class IAudioInput;
class IAudioOutput;

/// Audio engine implementation
/// Manages audio devices and pass-through functionality
class AudioEngine : public IAudioEngine {
public:
    AudioEngine();
    ~AudioEngine() override;

    // Disable copy and move
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;

    //--- Device Management ---

    IDeviceEnumerator& GetDeviceEnumerator() override;
    bool SelectInputDevice(int deviceId) override;
    bool SelectOutputDevice(int deviceId) override;

    //--- Engine Control ---

    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;

    //--- Pass-through Mode ---

    void SetPassThroughEnabled(bool enabled) override;
    bool IsPassThroughEnabled() const override;

    //--- Monitoring ---

    void SetLevelCallback(LevelCallback callback) override;
    double GetCurrentLatency() const override;

private:
    /// Processing thread function
    void ProcessingThread();

    /// Update levels from input/output
    void UpdateLevels();

    /// Audio configuration
    AudioStreamConfig config_;

    /// Device enumerator
    std::unique_ptr<DeviceEnumerator> enumerator_;

    /// Audio input/output
    std::unique_ptr<IAudioInput> input_;
    std::unique_ptr<IAudioOutput> output_;

    /// Audio pipeline
    std::unique_ptr<AudioPipeline> pipeline_;

    /// State
    std::atomic<bool> running_;
    std::atomic<bool> passThroughEnabled_;
    std::atomic<int> selectedInputDevice_;
    std::atomic<int> selectedOutputDevice_;

    /// Level monitoring
    LevelCallback levelCallback_;
    std::mutex levelCallbackMutex_;
    AudioLevels currentLevels_;

    /// Processing thread
    std::thread processingThread_;
    std::atomic<bool> processingThreadActive_;

    /// Latency tracking
    mutable std::mutex latencyMutex_;
    double currentLatency_;
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_AUDIOENGINE_H
