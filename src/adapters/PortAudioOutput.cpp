/**
 * @file PortAudioOutput.cpp
 * @brief Implementation of PortAudioOutput class
 */

#include "adapters/PortAudioOutput.h"
#include "utils/PortAudioHelper.h"
#include "utils/Logger.h"
#include <chrono>
#include <cstring>
#include <cassert>

namespace audiobridge {

// =============================================================================
// Construction / Destruction
// =============================================================================

PortAudioOutput::PortAudioOutput()
    : stream_(nullptr)
    , config_{}
    , deviceInfo_{}
    , state_()
    , ringBuffer_(nullptr)
    , peakLevelL_(0.0f)
    , peakLevelR_(0.0f)
    , stateCallback_(nullptr)
    , timestamp_(0)
{
    AB_LOG_DEBUG("PortAudioOutput: Constructor called");

    // Initialize PortAudio using the helper
    if (!PortAudioHelper::GetInstance().Initialize()) {
        AB_LOG_ERROR("PortAudioOutput: Failed to initialize PortAudio");
        UpdateState(StreamState::Error);
        return;
    }

    UpdateState(StreamState::Stopped);
}

PortAudioOutput::~PortAudioOutput() {
    AB_LOG_DEBUG("PortAudioOutput: Destructor called");

    // Close stream if still open
    if (state_.isOpen.load()) {
        Close();
    }

    // Release PortAudio reference
    PortAudioHelper::GetInstance().Terminate();
}

// =============================================================================
// Lifecycle Management
// =============================================================================

bool PortAudioOutput::Open(int deviceId, const AudioStreamConfig& config) {
    AB_LOG_INFO("PortAudioOutput: Opening device " + std::to_string(deviceId));

    // Validate preconditions
    if (state_.isOpen.load() || state_.isStarted.load()) {
        AB_LOG_ERROR("PortAudioOutput: Already open");
        return false;
    }

    if (!ValidateConfig(config)) {
        AB_LOG_ERROR("PortAudioOutput: Invalid configuration");
        UpdateState(StreamState::Error);
        return false;
    }

    // Get device info
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceId);
    if (!deviceInfo) {
        AB_LOG_ERROR("PortAudioOutput: Invalid device ID " + std::to_string(deviceId));
        UpdateState(StreamState::Error);
        return false;
    }

    if (deviceInfo->maxOutputChannels < config.channelCount) {
        AB_LOG_ERROR("PortAudioOutput: Device does not support " +
                     std::to_string(config.channelCount) + " channels");
        UpdateState(StreamState::Error);
        return false;
    }

    // Cache device info
    deviceInfo_.deviceId = deviceId;
    deviceInfo_.name = deviceInfo->name;
    deviceInfo_.hostApi = Pa_GetHostApiInfo(deviceInfo->hostApi)->name;
    deviceInfo_.maxInputChannels = deviceInfo->maxInputChannels;
    deviceInfo_.maxOutputChannels = deviceInfo->maxOutputChannels;
    deviceInfo_.defaultSampleRate = deviceInfo->defaultSampleRate;
    deviceInfo_.isDefaultInput = (deviceId == Pa_GetDefaultInputDevice());
    deviceInfo_.isDefaultOutput = (deviceId == Pa_GetDefaultOutputDevice());

    AB_LOG_INFO("PortAudioOutput: Device: " + deviceInfo_.name);
    AB_LOG_INFO("PortAudioOutput: Channels: " + std::to_string(config.channelCount) + "/" +
                std::to_string(deviceInfo->maxOutputChannels));
    AB_LOG_INFO("PortAudioOutput: Sample rate: " + std::to_string(config.sampleRate));

    // Create ring buffer (raw pointer for template with size parameter)
    ringBuffer_ = new RingBuffer<float, kRingBufferSize>();

    // Store configuration
    config_ = config;

    // Open PortAudio stream
    PaStreamParameters outputParams;
    std::memset(&outputParams, 0, sizeof(outputParams));
    outputParams.device = deviceId;
    outputParams.channelCount = config.channelCount;
    outputParams.sampleFormat = paFloat32;  // Always use float32
    outputParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    UpdateState(StreamState::Starting);

    PaError err = Pa_OpenStream(
        &stream_,
        nullptr,                // No input (output-only stream)
        &outputParams,          // Output parameters
        config.sampleRate,
        config.framesPerBuffer,
        paPrimeOutputBuffersUsingStreamCallback,  // Flag for output-only
        &PortAudioOutput::AudioCallback,
        this                   // User data
    );

    if (err != paNoError) {
        AB_LOG_ERROR(std::string("PortAudioOutput: Failed to open stream: ") + Pa_GetErrorText(err));
        stream_ = nullptr;
        delete ringBuffer_;
        ringBuffer_ = nullptr;
        UpdateState(StreamState::Error);
        return false;
    }

    state_.isOpen.store(true);
    UpdateState(StreamState::Stopped);

    AB_LOG_INFO("PortAudioOutput: Stream opened successfully");
    return true;
}

bool PortAudioOutput::Start() {
    AB_LOG_INFO("PortAudioOutput: Starting stream");

    // Validate preconditions
    if (!state_.isOpen.load()) {
        AB_LOG_ERROR("PortAudioOutput: Not open");
        return false;
    }

    if (state_.isStarted.load()) {
        AB_LOG_ERROR("PortAudioOutput: Already started");
        return false;
    }

    if (!stream_) {
        AB_LOG_ERROR("PortAudioOutput: Invalid stream handle");
        UpdateState(StreamState::Error);
        return false;
    }

    // Clear ring buffer
    if (ringBuffer_) {
        ringBuffer_->Reset();
    }

    // Reset peak levels
    peakLevelL_.store(0.0f);
    peakLevelR_.store(0.0f);

    // Start the stream
    PaError err = Pa_StartStream(stream_);
    if (err != paNoError) {
        AB_LOG_ERROR(std::string("PortAudioOutput: Failed to start stream: ") + Pa_GetErrorText(err));
        UpdateState(StreamState::Error);
        return false;
    }

    state_.isStarted.store(true);
    UpdateState(StreamState::Active);

    AB_LOG_INFO("PortAudioOutput: Stream started");
    return true;
}

void PortAudioOutput::Stop() {
    AB_LOG_INFO("PortAudioOutput: Stopping stream");

    if (!state_.isStarted.load()) {
        AB_LOG_DEBUG("PortAudioOutput: Already stopped");
        return;
    }

    if (!stream_) {
        AB_LOG_ERROR("PortAudioOutput: Invalid stream handle");
        return;
    }

    UpdateState(StreamState::Stopping);

    // Stop the stream
    PaError err = Pa_StopStream(stream_);
    if (err != paNoError) {
        AB_LOG_ERROR(std::string("PortAudioOutput: Failed to stop stream: ") + Pa_GetErrorText(err));
        UpdateState(StreamState::Error);
        return;
    }

    state_.isStarted.store(false);
    UpdateState(StreamState::Stopped);

    AB_LOG_INFO("PortAudioOutput: Stream stopped");
}

void PortAudioOutput::Close() {
    AB_LOG_INFO("PortAudioOutput: Closing stream");

    // Stop if running
    if (state_.isStarted.load()) {
        Stop();
    }

    if (!state_.isOpen.load()) {
        AB_LOG_DEBUG("PortAudioOutput: Already closed");
        return;
    }

    // Close stream
    if (stream_) {
        PaError err = Pa_CloseStream(stream_);
        if (err != paNoError) {
            AB_LOG_ERROR(std::string("PortAudioOutput: Failed to close stream: ") + Pa_GetErrorText(err));
        }
        stream_ = nullptr;
    }

    // Release resources
    delete ringBuffer_;
    ringBuffer_ = nullptr;

    state_.isOpen.store(false);
    state_.isStarted.store(false);
    UpdateState(StreamState::Stopped);

    // Clear device info
    deviceInfo_ = AudioDeviceInfo{};

    AB_LOG_INFO("PortAudioOutput: Stream closed");
}

// =============================================================================
// State Query
// =============================================================================

StreamState PortAudioOutput::GetState() const {
    return state_.streamState.load();
}

bool PortAudioOutput::IsActive() const {
    return state_.streamState.load() == StreamState::Active;
}

AudioDeviceInfo PortAudioOutput::GetDeviceInfo() const {
    return deviceInfo_;
}

// =============================================================================
// Data Access (RT-safe)
// =============================================================================

size_t PortAudioOutput::AvailableSpace() const {
    if (!ringBuffer_) {
        return 0;
    }

    // Ring buffer stores samples, we want frames
    size_t availableSpace = ringBuffer_->AvailableForWrite();
    return availableSpace / config_.channelCount;
}

size_t PortAudioOutput::Write(const AudioBuffer& buffer) {
    if (!ringBuffer_) {
        return 0;
    }

    if (!state_.isStarted.load()) {
        // Cannot write if stream is not started
        return 0;
    }

    if (buffer.data == nullptr || buffer.frameCount == 0) {
        return 0;
    }

    // Calculate how many frames we can actually write
    size_t availableSpace = AvailableSpace();
    size_t framesToWrite = std::min(buffer.frameCount, availableSpace);
    size_t samplesToWrite = framesToWrite * buffer.channelCount;

    if (samplesToWrite == 0) {
        return 0;
    }

    // Write to ring buffer
    size_t samplesWritten = ringBuffer_->Write(buffer.data, samplesToWrite);
    size_t framesWritten = samplesWritten / buffer.channelCount;

    // Update timestamp
    using namespace std::chrono;
    uint64_t now = duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
    timestamp_.store(now);

    return framesWritten;
}

// =============================================================================
// Callback Registration
// =============================================================================

void PortAudioOutput::SetStateCallback(StreamStateCallback callback) {
    std::lock_guard<std::mutex> lock(stateCallbackMutex_);
    stateCallback_ = std::move(callback);
}

std::pair<float, float> PortAudioOutput::GetPeakLevels() const {
    return {peakLevelL_.load(), peakLevelR_.load()};
}

// =============================================================================
// Audio Callback (RT-safe)
// =============================================================================

int PortAudioOutput::AudioCallback(const void* inputBuffer,
                                   void* outputBuffer,
                                   unsigned long frameCount,
                                   const PaStreamCallbackTimeInfo* timeInfo,
                                   PaStreamCallbackFlags statusFlags,
                                   void* userData) {
    (void)inputBuffer;  // Unused for output stream
    (void)timeInfo;

    auto* output = static_cast<PortAudioOutput*>(userData);
    return output->ProcessAudio(static_cast<float*>(outputBuffer),
                                frameCount,
                                statusFlags);
}

int PortAudioOutput::ProcessAudio(float* outputBuffer,
                                  unsigned long frameCount,
                                  PaStreamCallbackFlags statusFlags) {
    // Handle stream status flags
    if (statusFlags & paOutputUnderflow) {
        AB_LOG_WARNING("PortAudioOutput: Output underflow detected");
    }
    if (statusFlags & paOutputOverflow) {
        AB_LOG_WARNING("PortAudioOutput: Output overflow detected");
    }

    // Check for null buffer (shouldn't happen, but be defensive)
    if (!outputBuffer) {
        AB_LOG_ERROR("PortAudioOutput: Null output buffer in callback");
        return paComplete;
    }

    // Calculate total samples
    size_t totalSamples = frameCount * config_.channelCount;

    // Try to read from ring buffer
    size_t availableSamples = ringBuffer_->AvailableForRead();
    size_t samplesToRead = std::min(totalSamples, availableSamples);

    if (samplesToRead < totalSamples) {
        // Buffer underrun - fill remaining with silence
        std::fill(outputBuffer + samplesToRead, outputBuffer + totalSamples, 0.0f);
        (void)totalSamples;  // Suppress unused warning in release builds
    }

    // Read audio data from ring buffer
    if (samplesToRead > 0) {
        size_t samplesRead = ringBuffer_->Read(outputBuffer, samplesToRead);

        if (samplesRead != samplesToRead) {
            AB_LOG_ERROR("PortAudioOutput: Read mismatch: " + std::to_string(samplesRead) + " != " + std::to_string(samplesToRead));
        }

        // Calculate peak levels from output data
        auto [peakL, peakR] = CalculatePeaks(outputBuffer, frameCount, config_.channelCount);

        // Update atomic peaks (keep max since last read)
        float currentPeakL = peakLevelL_.load();
        float currentPeakR = peakLevelR_.load();
        peakLevelL_.store(std::max(currentPeakL, peakL));
        peakLevelR_.store(std::max(currentPeakR, peakR));
    }

    // Update timestamp (convert to microseconds)
    using namespace std::chrono;
    uint64_t now = duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
    timestamp_.store(now);

    return paContinue;
}

// =============================================================================
// Internal Helpers
// =============================================================================

void PortAudioOutput::UpdateState(StreamState newState) {
    StreamState oldState = state_.streamState.load();

    if (oldState == newState) {
        return;  // No change
    }

    state_.streamState.store(newState);
    AB_LOG_DEBUG("PortAudioOutput: State change: " + std::to_string(static_cast<int>(oldState)) + " -> " + std::to_string(static_cast<int>(newState)));

    // Notify callback if registered
    std::lock_guard<std::mutex> lock(stateCallbackMutex_);
    if (stateCallback_) {
        stateCallback_(newState);
    }
}

bool PortAudioOutput::ValidateConfig(const AudioStreamConfig& config) const {
    if (!config.IsValid()) {
        AB_LOG_ERROR("PortAudioOutput: Config validation failed");
        return false;
    }
    return true;
}

std::pair<float, float> PortAudioOutput::CalculatePeaks(const float* data,
                                                         unsigned long frames,
                                                         int channels) const {
    float peakL = 0.0f;
    float peakR = 0.0f;

    for (unsigned long i = 0; i < frames; ++i) {
        if (channels >= 1) {
            peakL = std::max(peakL, std::abs(data[i * channels]));
        }
        if (channels >= 2) {
            peakR = std::max(peakR, std::abs(data[i * channels + 1]));
        }
    }

    return {peakL, peakR};
}

} // namespace audiobridge
