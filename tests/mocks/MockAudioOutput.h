#ifndef AUDIOBRIDGE_MOCK_AUDIO_OUTPUT_H
#define AUDIOBRIDGE_MOCK_AUDIO_OUTPUT_H

#include "adapters/IAudioOutput.h"
#include "core/RingBuffer.h"
#include <atomic>
#include <queue>

namespace audiobridge {

/// Mock audio output adapter for testing
/// Simulates audio output without requiring actual audio hardware
class MockAudioOutput : public IAudioOutput {
public:
    MockAudioOutput()
        : stream_(nullptr)
        , config_{}
        , state_()
        , ringBuffer_(nullptr)
        , peakLevelL_(0.0f)
        , peakLevelR_(0.0f)
        , stateCallback_(nullptr)
        , playbackThread_(nullptr)
        , playbackActive_(false)
        , totalFramesPlayed_(0)
    {
        state_.isOpen = false;
        state_.isStarted = false;
        state_.currentState = StreamState::Stopped;
    }

    ~MockAudioOutput() override {
        Stop();
        Close();
        delete ringBuffer_;
    }

    bool Open(int deviceId, const AudioStreamConfig& config) override {
        if (state_.isOpen.load()) {
            return false;  // Already open
        }

        // Validate device ID (accept any for mock)
        if (deviceId < 0) {
            return false;
        }

        // Validate config
        if (config.sampleRate <= 0 || config.channelCount <= 0 || config.framesPerBuffer <= 0) {
            return false;
        }

        config_ = config;
        state_.isOpen.store(true);
        UpdateState(StreamState::Stopped);
        return true;
    }

    bool Start() override {
        if (!state_.isOpen.load() || state_.isStarted.load()) {
            return false;
        }

        // Create ring buffer if not exists
        if (!ringBuffer_) {
            ringBuffer_ = new RingBuffer<float, 16384>();  // Fixed size for simplicity
        }

        state_.isStarted.store(true);
        UpdateState(StreamState::Active);

        // Start playback thread
        playbackActive_.store(true);
        playbackThread_ = new std::thread(&MockAudioOutput::PlaybackLoop, this);

        return true;
    }

    void Stop() override {
        if (!state_.isStarted.load()) {
            return;
        }

        playbackActive_.store(false);
        if (playbackThread_ && playbackThread_->joinable()) {
            playbackThread_->join();
            delete playbackThread_;
            playbackThread_ = nullptr;
        }

        state_.isStarted.store(false);
        UpdateState(StreamState::Stopped);
    }

    void Close() override {
        Stop();

        if (!state_.isOpen.load()) {
            return;
        }

        delete ringBuffer_;
        ringBuffer_ = nullptr;

        state_.isOpen.store(false);
        state_.isStarted.store(false);
        UpdateState(StreamState::Stopped);
    }

    size_t AvailableSpace() const override {
        if (!ringBuffer_) return 0;
        return ringBuffer_->AvailableForWrite() / config_.channelCount;
    }

    size_t Write(const AudioBuffer& buffer) override {
        if (!ringBuffer_ || !state_.isStarted.load()) {
            return 0;
        }

        if (buffer.data == nullptr || buffer.frameCount == 0) {
            return 0;
        }

        size_t samplesToWrite = buffer.frameCount * buffer.channelCount;
        size_t availableSpace = ringBuffer_->AvailableForWrite();
        size_t samplesToWriteActual = std::min(samplesToWrite, availableSpace);

        if (samplesToWriteActual == 0) {
            return 0;
        }

        size_t samplesWritten = ringBuffer_->Write(buffer.data, samplesToWriteActual);
        size_t framesWritten = samplesWritten / buffer.channelCount;

        // Note: timestamp not tracked in mock

        return framesWritten;
    }

    std::pair<float, float> GetPeakLevels() const override {
        return {peakLevelL_.load(), peakLevelR_.load()};
    }

    void SetStateCallback(StreamStateCallback callback) override {
        stateCallback_ = callback;
    }

    StreamState GetState() const override {
        return state_.currentState.load();
    }

    bool IsActive() const override {
        return state_.isStarted.load();
    }

    AudioDeviceInfo GetDeviceInfo() const override {
        AudioDeviceInfo info;
        info.deviceId = 1;
        info.name = "Mock Output Device";
        info.hostApi = "Mock API";
        info.maxInputChannels = 0;
        info.maxOutputChannels = config_.channelCount;
        info.defaultSampleRate = config_.sampleRate;
        info.isDefaultInput = false;
        info.isDefaultOutput = true;
        return info;
    }

    /// Get total frames played (for testing)
    size_t GetTotalFramesPlayed() const {
        return totalFramesPlayed_.load();
    }

private:
    void UpdateState(StreamState newState) {
        state_.currentState.store(newState);
        if (stateCallback_) {
            stateCallback_(newState);
        }
    }

    /// Playback loop (simulates audio output callback)
    void PlaybackLoop() {
        const size_t framesPerChunk = config_.framesPerBuffer;
        const size_t samplesPerChunk = framesPerChunk * config_.channelCount;
        float* buffer = new float[samplesPerChunk];

        while (playbackActive_.load()) {
            // Read from ring buffer
            size_t availableSamples = ringBuffer_->AvailableForRead();
            size_t samplesToRead = std::min(samplesPerChunk, availableSamples);

            if (samplesToRead > 0) {
                size_t samplesRead = ringBuffer_->Read(buffer, samplesToRead);
                size_t framesRead = samplesRead / config_.channelCount;

                // Calculate output levels
                float peakL = 0.0f, peakR = 0.0f;
                for (size_t i = 0; i < framesRead; ++i) {
                    float absL = std::abs(buffer[i * config_.channelCount]);
                    float absR = std::abs(buffer[i * config_.channelCount + 1]);
                    peakL = std::max(peakL, absL);
                    peakR = std::max(peakR, absR);
                }
                peakLevelL_.store(peakL);
                peakLevelR_.store(peakR);

                totalFramesPlayed_ += framesRead;
            }

            // Sleep to simulate real-time processing
            uint64_t sleepUs = (framesPerChunk * 1000000ULL) / config_.sampleRate;
            std::this_thread::sleep_for(std::chrono::microseconds(sleepUs));
        }

        delete[] buffer;
    }

    void* stream_;  // Not used in mock
    AudioStreamConfig config_;
    struct {
        std::atomic<bool> isOpen;
        std::atomic<bool> isStarted;
        std::atomic<StreamState> currentState;
    } state_;
    RingBuffer<float, 16384>* ringBuffer_;
    std::atomic<float> peakLevelL_;
    std::atomic<float> peakLevelR_;
    StreamStateCallback stateCallback_;
    std::thread* playbackThread_;
    std::atomic<bool> playbackActive_;
    std::atomic<size_t> totalFramesPlayed_;
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_MOCK_AUDIO_OUTPUT_H
