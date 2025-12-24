/**
 * @file AudioEngine.cpp
 * @brief Implementation of AudioEngine class
 */

#include "core/AudioEngine.h"
#include "adapters/PortAudioInput.h"
#include "adapters/PortAudioOutput.h"
#include "utils/Logger.h"
#include <chrono>
#include <thread>
#include <mutex>

namespace audiobridge {

// =============================================================================
// Construction / Destruction
// =============================================================================

AudioEngine::AudioEngine()
    : config_({})
    , enumerator_(std::make_unique<DeviceEnumerator>())
    , input_(nullptr)
    , output_(nullptr)
    , pipeline_(std::make_unique<AudioPipeline>())
    , running_(false)
    , passThroughEnabled_(false)
    , selectedInputDevice_(-1)
    , selectedOutputDevice_(-1)
    , levelCallback_(nullptr)
    , processingThreadActive_(false)
    , currentLatency_(0.0) {
    AB_LOG_INFO("AudioEngine: Constructor called");

    // Set default audio config
    config_.sampleRate = 48000.0;
    config_.framesPerBuffer = 128;
    config_.channelCount = 2;
    config_.format = SampleFormat::Float32;

    // Initialize levels
    currentLevels_.inputPeakL = 0.0f;
    currentLevels_.inputPeakR = 0.0f;
    currentLevels_.outputPeakL = 0.0f;
    currentLevels_.outputPeakR = 0.0f;

    AB_LOG_INFO("AudioEngine: Engine created");
}

AudioEngine::~AudioEngine() {
    AB_LOG_INFO("AudioEngine: Destructor called");

    // Stop engine if running
    if (running_.load()) {
        Stop();
    }

    // Clean up
    if (input_) {
        input_->Close();
        input_.reset();
    }

    if (output_) {
        output_->Close();
        output_.reset();
    }

    AB_LOG_INFO("AudioEngine: Engine destroyed");
}

// =============================================================================
// Device Management
// =============================================================================

IDeviceEnumerator& AudioEngine::GetDeviceEnumerator() {
    return *enumerator_;
}

bool AudioEngine::SelectInputDevice(int deviceId) {
    AB_LOG_INFO("AudioEngine: Selecting input device " + std::to_string(deviceId));

    // Validate device
    if (!enumerator_->GetDeviceInfo(deviceId).IsValid()) {
        AB_LOG_ERROR("AudioEngine: Invalid input device: " + std::to_string(deviceId));
        return false;
    }

    // Stop if running
    if (running_.load()) {
        AB_LOG_ERROR("AudioEngine: Cannot change devices while running");
        return false;
    }

    // Close existing input
    if (input_) {
        input_->Close();
        input_.reset();
    }

    // Create new input
    input_ = std::make_unique<PortAudioInput>();

    // Open device
    if (!input_->Open(deviceId, config_)) {
        AB_LOG_ERROR("AudioEngine: Failed to open input device");
        input_.reset();
        return false;
    }

    // Connect to pipeline
    pipeline_->SetInput(input_.get());

    selectedInputDevice_.store(deviceId);
    AB_LOG_INFO("AudioEngine: Input device selected successfully");
    return true;
}

bool AudioEngine::SelectOutputDevice(int deviceId) {
    AB_LOG_INFO("AudioEngine: Selecting output device " + std::to_string(deviceId));

    // Validate device
    if (!enumerator_->GetDeviceInfo(deviceId).IsValid()) {
        AB_LOG_ERROR("AudioEngine: Invalid output device: " + std::to_string(deviceId));
        return false;
    }

    // Validate input and output are different
    if (deviceId == selectedInputDevice_.load()) {
        AB_LOG_ERROR("AudioEngine: Input and output cannot be the same device");
        return false;
    }

    // Stop if running
    if (running_.load()) {
        AB_LOG_ERROR("AudioEngine: Cannot change devices while running");
        return false;
    }

    // Close existing output
    if (output_) {
        output_->Close();
        output_.reset();
    }

    // Create new output
    output_ = std::make_unique<PortAudioOutput>();

    // Open device
    if (!output_->Open(deviceId, config_)) {
        AB_LOG_ERROR("AudioEngine: Failed to open output device");
        output_.reset();
        return false;
    }

    // Connect to pipeline
    pipeline_->SetOutput(output_.get());

    selectedOutputDevice_.store(deviceId);
    AB_LOG_INFO("AudioEngine: Output device selected successfully");
    return true;
}

// =============================================================================
// Engine Control
// =============================================================================

bool AudioEngine::Start() {
    AB_LOG_INFO("AudioEngine: Starting engine");

    // Check if devices are selected
    if (!input_ || !output_) {
        AB_LOG_ERROR("AudioEngine: Input and output devices must be selected");
        return false;
    }

    // Check if already running
    if (running_.load()) {
        AB_LOG_WARNING("AudioEngine: Already running");
        return true;
    }

    // Start input stream
    if (!input_->Start()) {
        AB_LOG_ERROR("AudioEngine: Failed to start input");
        return false;
    }

    // Start output stream
    if (!output_->Start()) {
        AB_LOG_ERROR("AudioEngine: Failed to start output");
        input_->Stop();
        return false;
    }

    // Start processing thread
    processingThreadActive_.store(true);
    processingThread_ = std::thread(&AudioEngine::ProcessingThread, this);

    running_.store(true);
    AB_LOG_INFO("AudioEngine: Engine started");
    return true;
}

void AudioEngine::Stop() {
    AB_LOG_INFO("AudioEngine: Stopping engine");

    if (!running_.load()) {
        AB_LOG_DEBUG("AudioEngine: Already stopped");
        return;
    }

    // Stop processing thread
    processingThreadActive_.store(false);
    if (processingThread_.joinable()) {
        processingThread_.join();
    }

    // Stop streams
    if (input_) {
        input_->Stop();
    }

    if (output_) {
        output_->Stop();
    }

    running_.store(false);
    AB_LOG_INFO("AudioEngine: Engine stopped");
}

bool AudioEngine::IsRunning() const {
    return running_.load();
}

// =============================================================================
// Pass-through Mode
// =============================================================================

void AudioEngine::SetPassThroughEnabled(bool enabled) {
    AB_LOG_INFO("AudioEngine: Pass-through " + std::string(enabled ? "enabled" : "disabled"));
    passThroughEnabled_.store(enabled);
    pipeline_->SetPassThroughEnabled(enabled);
}

bool AudioEngine::IsPassThroughEnabled() const {
    return passThroughEnabled_.load();
}

// =============================================================================
// Monitoring
// =============================================================================

void AudioEngine::SetLevelCallback(LevelCallback callback) {
    std::lock_guard<std::mutex> lock(levelCallbackMutex_);
    levelCallback_ = std::move(callback);
    AB_LOG_DEBUG("AudioEngine: Level callback registered");
}

double AudioEngine::GetCurrentLatency() const {
    std::lock_guard<std::mutex> lock(latencyMutex_);

    // Calculate latency from frames
    size_t latencyFrames = pipeline_->GetLatencyFrames();

    // Convert to milliseconds
    double latencyMs = (latencyFrames / config_.sampleRate) * 1000.0;

    return latencyMs;
}

// =============================================================================
// Internal Helpers
// =============================================================================

void AudioEngine::ProcessingThread() {
    AB_LOG_DEBUG("AudioEngine: Processing thread started");

    while (processingThreadActive_.load()) {
        // Process audio pipeline
        pipeline_->Process();

        // Update levels
        UpdateLevels();

        // Sleep for a short time (10ms = 100Hz update rate)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    AB_LOG_DEBUG("AudioEngine: Processing thread stopped");
}

void AudioEngine::UpdateLevels() {
    if (!input_ || !output_) {
        return;
    }

    // Get input levels (from PortAudioInput)
    auto [inputPeakL, inputPeakR] = input_->GetPeakLevels();

    // Get output levels (from PortAudioOutput)
    auto [outputPeakL, outputPeakR] = output_->GetPeakLevels();

    // Update current levels
    currentLevels_.inputPeakL = inputPeakL;
    currentLevels_.inputPeakR = inputPeakR;
    currentLevels_.outputPeakL = outputPeakL;
    currentLevels_.outputPeakR = outputPeakR;

    // Notify callback if registered
    std::lock_guard<std::mutex> lock(levelCallbackMutex_);
    if (levelCallback_) {
        levelCallback_(currentLevels_);
    }
}

}  // namespace audiobridge
