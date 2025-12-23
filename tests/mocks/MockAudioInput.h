#ifndef AUDIOBRIDGE_MOCK_AUDIO_INPUT_H
#define AUDIOBRIDGE_MOCK_AUDIO_INPUT_H

#include "adapters/IAudioInput.h"
#include "core/RingBuffer.h"
#include <atomic>
#include <cstring>
#include <cmath>

namespace audiobridge {

/// Mock audio input adapter for testing
/// Simulates audio input without requiring actual audio hardware
class MockAudioInput : public IAudioInput {
public:
    MockAudioInput()
        : stream_(nullptr)
        , config_{}
        , state_()
        , ringBuffer_(nullptr)
        , peakLevelL_(0.0f)
        , peakLevelR_(0.0f)
        , stateCallback_(nullptr)
        , simulationThread_(nullptr)
        , simulateAudio_(false)
    {
        state_.isOpen = false;
        state_.isStarted = false;
        state_.currentState = StreamState::Stopped;
    }

    ~MockAudioInput() override {
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
            size_t bufferSamples = config_.framesPerBuffer * config_.channelCount * 4;  // 4x buffer
            size_t bufferSize = 1;
            while (bufferSize < bufferSamples) bufferSize <<= 1;
            ringBuffer_ = new RingBuffer<float, 16384>();  // Fixed size for simplicity
        }

        state_.isStarted.store(true);
        UpdateState(StreamState::Active);

        // Start simulation thread
        simulateAudio_.store(true);
        simulationThread_ = new std::thread(&MockAudioInput::SimulateAudio, this);

        return true;
    }

    void Stop() override {
        if (!state_.isStarted.load()) {
            return;
        }

        simulateAudio_.store(false);
        if (simulationThread_ && simulationThread_->joinable()) {
            simulationThread_->join();
            delete simulationThread_;
            simulationThread_ = nullptr;
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

    size_t AvailableFrames() const override {
        if (!ringBuffer_) return 0;
        return ringBuffer_->AvailableForRead() / config_.channelCount;
    }

    size_t Read(AudioBuffer& buffer) override {
        if (!ringBuffer_ || !state_.isStarted.load()) {
            return 0;
        }

        size_t samplesToRead = buffer.frameCount * buffer.channelCount;
        size_t availableSamples = ringBuffer_->AvailableForRead();
        size_t samplesRead = ringBuffer_->Read(buffer.data, std::min(samplesToRead, availableSamples));

        // Update timestamp
        using namespace std::chrono;
        // Note: timestamp not tracked in mock

        return samplesRead / buffer.channelCount;
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
        info.deviceId = 0;
        info.name = "Mock Input Device";
        info.hostApi = "Mock API";
        info.maxInputChannels = config_.channelCount;
        info.maxOutputChannels = 0;
        info.defaultSampleRate = config_.sampleRate;
        info.isDefaultInput = true;
        info.isDefaultOutput = false;
        return info;
    }

private:
    void UpdateState(StreamState newState) {
        state_.currentState.store(newState);
        if (stateCallback_) {
            stateCallback_(newState);
        }
    }

    /// Simulate audio data generation (runs in separate thread)
    void SimulateAudio() {
        const size_t framesPerChunk = config_.framesPerBuffer;
        const size_t samplesPerChunk = framesPerChunk * config_.channelCount;
        float* buffer = new float[samplesPerChunk];

        while (simulateAudio_.load()) {
            // Generate test tone (1kHz sine wave)
            for (size_t i = 0; i < framesPerChunk; ++i) {
                float t = static_cast<float>(generatedFrames_ + i) / config_.sampleRate;
                float sample = 0.5f * std::sin(2.0f * 3.14159f * 1000.0f * t);

                for (int ch = 0; ch < static_cast<int>(config_.channelCount); ++ch) {
                    buffer[i * config_.channelCount + ch] = sample;
                }
            }

            // Calculate levels
            float peakL = 0.0f, peakR = 0.0f;
            for (size_t i = 0; i < framesPerChunk; ++i) {
                float absL = std::abs(buffer[i * config_.channelCount]);
                float absR = std::abs(buffer[i * config_.channelCount + 1]);
                peakL = std::max(peakL, absL);
                peakR = std::max(peakR, absR);
            }
            peakLevelL_.store(peakL);
            peakLevelR_.store(peakR);

            // Write to ring buffer
            if (ringBuffer_) {
                size_t written = ringBuffer_->Write(buffer, samplesPerChunk);
                if (written < samplesPerChunk) {
                    // Buffer overflow handling
                }
            }

            generatedFrames_ += framesPerChunk;

            // Sleep to simulate real-time processing (roughly)
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
    std::thread* simulationThread_;
    std::atomic<bool> simulateAudio_;
    size_t generatedFrames_ = 0;
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_MOCK_AUDIO_INPUT_H
