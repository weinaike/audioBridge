/**
 * @file PortAudioInput.cpp
 * @brief Implementation of PortAudioInput class
 */

#include "adapters/PortAudioInput.h"
#include "utils/PortAudioHelper.h"
#include "utils/Logger.h"
#include <chrono>
#include <cstring>
#include <cassert>

namespace audiobridge {

// =============================================================================
// Construction / Destruction
// =============================================================================

PortAudioInput::PortAudioInput()
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
    AB_LOG_DEBUG("PortAudioInput: Constructor called");

    // Initialize PortAudio using the helper
    if (!PortAudioHelper::GetInstance().Initialize()) {
        AB_LOG_ERROR("PortAudioInput: Failed to initialize PortAudio");
        UpdateState(StreamState::Error);
        return;
    }

    UpdateState(StreamState::Stopped);
}

PortAudioInput::~PortAudioInput() {
    AB_LOG_DEBUG("PortAudioInput: Destructor called");

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

bool PortAudioInput::Open(int deviceId, const AudioStreamConfig& config) {
    AB_LOG_INFO("PortAudioInput: Opening device " + std::to_string(deviceId));

    // Validate preconditions
    if (state_.isOpen.load() || state_.isStarted.load()) {
        AB_LOG_ERROR("PortAudioInput: Already open");
        return false;
    }

    if (!ValidateConfig(config)) {
        AB_LOG_ERROR("PortAudioInput: Invalid configuration");
        UpdateState(StreamState::Error);
        return false;
    }

    // Get device info
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceId);
    if (!deviceInfo) {
        AB_LOG_ERROR("PortAudioInput: Invalid device ID " + std::to_string(deviceId));
        UpdateState(StreamState::Error);
        return false;
    }

    if (deviceInfo->maxInputChannels < config.channelCount) {
        AB_LOG_ERROR("PortAudioInput: Device does not support " +
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

    AB_LOG_INFO("PortAudioInput: Device: " + deviceInfo_.name);
    AB_LOG_INFO("PortAudioInput: Channels: " + std::to_string(config.channelCount) + "/" +
                std::to_string(deviceInfo->maxInputChannels));
    AB_LOG_INFO("PortAudioInput: Sample rate: " + std::to_string(config.sampleRate));

    // Create ring buffer (raw pointer for template with size parameter)
    ringBuffer_ = new RingBuffer<float, kRingBufferSize>();

    // Store configuration
    config_ = config;

    // Open PortAudio stream
    PaStreamParameters inputParams;
    std::memset(&inputParams, 0, sizeof(inputParams));
    inputParams.device = deviceId;
    inputParams.channelCount = config.channelCount;
    inputParams.sampleFormat = paFloat32;  // Always use float32
    inputParams.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    UpdateState(StreamState::Starting);

    PaError err = Pa_OpenStream(
        &stream_,
        &inputParams,           // Input parameters
        nullptr,                // No output (input-only stream)
        config.sampleRate,
        config.framesPerBuffer,
        paPrimeOutputBuffersUsingStreamCallback,  // Flag for input-only
        &PortAudioInput::AudioCallback,
        this                   // User data
    );

    if (err != paNoError) {
        AB_LOG_ERROR(std::string("PortAudioInput: Failed to open stream: ") + Pa_GetErrorText(err));
        stream_ = nullptr;
        delete ringBuffer_;
        ringBuffer_ = nullptr;
        UpdateState(StreamState::Error);
        return false;
    }

    state_.isOpen.store(true);
    UpdateState(StreamState::Stopped);

    AB_LOG_INFO("PortAudioInput: Stream opened successfully");
    return true;
}

bool PortAudioInput::Start() {
    AB_LOG_INFO("PortAudioInput: Starting stream");

    // Validate preconditions
    if (!state_.isOpen.load()) {
        AB_LOG_ERROR("PortAudioInput: Not open");
        return false;
    }

    if (state_.isStarted.load()) {
        AB_LOG_ERROR("PortAudioInput: Already started");
        return false;
    }

    if (!stream_) {
        AB_LOG_ERROR("PortAudioInput: Invalid stream handle");
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
        AB_LOG_ERROR(std::string("PortAudioInput: Failed to start stream: ") + Pa_GetErrorText(err));
        UpdateState(StreamState::Error);
        return false;
    }

    state_.isStarted.store(true);
    UpdateState(StreamState::Active);

    AB_LOG_INFO("PortAudioInput: Stream started");
    return true;
}

void PortAudioInput::Stop() {
    AB_LOG_INFO("PortAudioInput: Stopping stream");

    if (!state_.isStarted.load()) {
        AB_LOG_DEBUG("PortAudioInput: Already stopped");
        return;
    }

    if (!stream_) {
        AB_LOG_ERROR("PortAudioInput: Invalid stream handle");
        return;
    }

    UpdateState(StreamState::Stopping);

    // Stop the stream
    PaError err = Pa_StopStream(stream_);
    if (err != paNoError) {
        AB_LOG_ERROR(std::string("PortAudioInput: Failed to stop stream: ") + Pa_GetErrorText(err));
        UpdateState(StreamState::Error);
        return;
    }

    state_.isStarted.store(false);
    UpdateState(StreamState::Stopped);

    AB_LOG_INFO("PortAudioInput: Stream stopped");
}

void PortAudioInput::Close() {
    AB_LOG_INFO("PortAudioInput: Closing stream");

    // Stop if running
    if (state_.isStarted.load()) {
        Stop();
    }

    if (!state_.isOpen.load()) {
        AB_LOG_DEBUG("PortAudioInput: Already closed");
        return;
    }

    // Close stream
    if (stream_) {
        PaError err = Pa_CloseStream(stream_);
        if (err != paNoError) {
            AB_LOG_ERROR(std::string("PortAudioInput: Failed to close stream: ") + Pa_GetErrorText(err));
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

    AB_LOG_INFO("PortAudioInput: Stream closed");
}

// =============================================================================
// State Query
// =============================================================================

StreamState PortAudioInput::GetState() const {
    return state_.streamState.load();
}

bool PortAudioInput::IsActive() const {
    return state_.streamState.load() == StreamState::Active;
}

AudioDeviceInfo PortAudioInput::GetDeviceInfo() const {
    return deviceInfo_;
}

// =============================================================================
// Data Access (RT-safe)
// =============================================================================

size_t PortAudioInput::AvailableFrames() const {
    if (!ringBuffer_) {
        return 0;
    }

    // Ring buffer stores samples, we want frames
    size_t availableSamples = ringBuffer_->AvailableForRead();
    return availableSamples / config_.channelCount;
}

size_t PortAudioInput::Read(AudioBuffer& buffer) {
    if (!ringBuffer_) {
        return 0;
    }

    if (buffer.data == nullptr || buffer.frameCount == 0) {
        return 0;
    }

    // Calculate how many frames we can actually read
    size_t availableFrames = AvailableFrames();
    size_t framesToRead = std::min(buffer.frameCount, availableFrames);
    size_t samplesToRead = framesToRead * buffer.channelCount;

    if (samplesToRead == 0) {
        return 0;
    }

    // Read from ring buffer
    size_t samplesRead = ringBuffer_->Read(buffer.data, samplesToRead);
    size_t framesRead = samplesRead / buffer.channelCount;

    // Update timestamp
    buffer.timestamp = timestamp_.load();

    return framesRead;
}

// =============================================================================
// Callback Registration
// =============================================================================

void PortAudioInput::SetStateCallback(StreamStateCallback callback) {
    std::lock_guard<std::mutex> lock(stateCallbackMutex_);
    stateCallback_ = std::move(callback);
}

std::pair<float, float> PortAudioInput::GetPeakLevels() const {
    return {peakLevelL_.load(), peakLevelR_.load()};
}

// =============================================================================
// Audio Callback (RT-safe)
// =============================================================================

int PortAudioInput::AudioCallback(const void* inputBuffer,
                                  void* outputBuffer,
                                  unsigned long frameCount,
                                  const PaStreamCallbackTimeInfo* timeInfo,
                                  PaStreamCallbackFlags statusFlags,
                                  void* userData) {
    (void)outputBuffer;  // Unused for input stream
    (void)timeInfo;

    auto* input = static_cast<PortAudioInput*>(userData);
    return input->ProcessAudio(static_cast<const float*>(inputBuffer),
                               frameCount,
                               statusFlags);
}

int PortAudioInput::ProcessAudio(const float* inputBuffer,
                                 unsigned long frameCount,
                                 PaStreamCallbackFlags statusFlags) {
    // Handle stream status flags
    if (statusFlags & paInputOverflow) {
        AB_LOG_WARNING("PortAudioInput: Input overflow detected");
    }
    if (statusFlags & paInputUnderflow) {
        AB_LOG_WARNING("PortAudioInput: Input underflow detected");
    }

    // Check for null buffer (shouldn't happen, but be defensive)
    if (!inputBuffer) {
        AB_LOG_ERROR("PortAudioInput: Null input buffer in callback");
        return paComplete;
    }

    // Calculate total samples
    size_t totalSamples = frameCount * config_.channelCount;

    // Try to write to ring buffer
    size_t availableSpace = ringBuffer_->AvailableForWrite();
    if (availableSpace < totalSamples) {
        // Ring buffer overflow - drop oldest samples
        AB_LOG_WARNING("PortAudioInput: Ring buffer overflow, dropping " +
                      std::to_string(totalSamples - availableSpace) + " samples");

        // Make space by reading/dropping oldest data
        size_t samplesToDrop = totalSamples - availableSpace;

        // Temporary buffer for dropping samples
        // Note: In RT context, this could be problematic, but overflow is rare
        static float dropBuffer[256];
        while (samplesToDrop > 0) {
            size_t toDrop = std::min(samplesToDrop, sizeof(dropBuffer)/sizeof(dropBuffer[0]));
            ringBuffer_->Read(dropBuffer, toDrop);
            samplesToDrop -= toDrop;
        }
    }

    // Write new audio data
    size_t samplesWritten = ringBuffer_->Write(inputBuffer, totalSamples);

    if (samplesWritten != totalSamples) {
        AB_LOG_ERROR("PortAudioInput: Write mismatch: " + std::to_string(samplesWritten) + " != " + std::to_string(totalSamples));
    }

    // Calculate peak levels
    auto [peakL, peakR] = CalculatePeaks(inputBuffer, frameCount, config_.channelCount);

    // Update atomic peaks (keep max since last read)
    float currentPeakL = peakLevelL_.load();
    float currentPeakR = peakLevelR_.load();
    peakLevelL_.store(std::max(currentPeakL, peakL));
    peakLevelR_.store(std::max(currentPeakR, peakR));

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

void PortAudioInput::UpdateState(StreamState newState) {
    StreamState oldState = state_.streamState.load();

    if (oldState == newState) {
        return;  // No change
    }

    state_.streamState.store(newState);
    AB_LOG_DEBUG("PortAudioInput: State change: " + std::to_string(static_cast<int>(oldState)) + " -> " + std::to_string(static_cast<int>(newState)));

    // Notify callback if registered
    std::lock_guard<std::mutex> lock(stateCallbackMutex_);
    if (stateCallback_) {
        stateCallback_(newState);
    }
}

bool PortAudioInput::ValidateConfig(const AudioStreamConfig& config) const {
    if (!config.IsValid()) {
        AB_LOG_ERROR("PortAudioInput: Config validation failed");
        return false;
    }
    return true;
}

std::pair<float, float> PortAudioInput::CalculatePeaks(const float* data,
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
