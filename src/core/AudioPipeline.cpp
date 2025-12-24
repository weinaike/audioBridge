/**
 * @file AudioPipeline.cpp
 * @brief Implementation of AudioPipeline class
 */

#include "core/AudioPipeline.h"
#include "adapters/IAudioInput.h"
#include "adapters/IAudioOutput.h"
#include "utils/Logger.h"
#include <cstring>

namespace audiobridge {

// =============================================================================
// Construction / Destruction
// =============================================================================

AudioPipeline::AudioPipeline()
    : input_(nullptr)
    , output_(nullptr)
    , passThroughEnabled_(false) {
    AB_LOG_DEBUG("AudioPipeline: Constructor called");

    // Initialize transfer buffer to silence
    std::fill(transferBuffer_, transferBuffer_ + kTransferBufferSize, 0.0f);
}

AudioPipeline::~AudioPipeline() {
    AB_LOG_DEBUG("AudioPipeline: Destructor called");
}

// =============================================================================
// Pipeline Configuration
// =============================================================================

void AudioPipeline::SetInput(IAudioInput* input) {
    input_ = input;
    AB_LOG_DEBUG("AudioPipeline: Input set");
}

void AudioPipeline::SetOutput(IAudioOutput* output) {
    output_ = output;
    AB_LOG_DEBUG("AudioPipeline: Output set");
}

void AudioPipeline::SetPassThroughEnabled(bool enabled) {
    passThroughEnabled_.store(enabled);
    AB_LOG_DEBUG("AudioPipeline: Pass-through " + std::string(enabled ? "enabled" : "disabled"));
}

bool AudioPipeline::IsPassThroughEnabled() const {
    return passThroughEnabled_.load();
}

// =============================================================================
// Audio Processing (RT-safe)
// =============================================================================

void AudioPipeline::Process() {
    // Only process if pass-through is enabled and both input/output are set
    if (!passThroughEnabled_.load() || !input_ || !output_) {
        return;
    }

    // Check if input has data available
    size_t availableFrames = input_->AvailableFrames();
    if (availableFrames == 0) {
        return;  // No input data
    }

    // Check if output has space
    size_t availableSpace = output_->AvailableSpace();
    if (availableSpace == 0) {
        return;  // Output full
    }

    // Determine how many frames to transfer
    size_t framesToTransfer = std::min({availableFrames, availableSpace, kTransferBufferSize});

    if (framesToTransfer == 0) {
        return;
    }

    // Read from input
    AudioBuffer buffer;
    buffer.data = transferBuffer_;
    buffer.frameCount = framesToTransfer;
    buffer.channelCount = 2;  // Stereo

    size_t framesRead = input_->Read(buffer);
    if (framesRead == 0) {
        return;  // No data read
    }

    // Update actual frames read
    buffer.frameCount = framesRead;

    // Write to output
    size_t framesWritten = output_->Write(buffer);

    // Log underruns/overruns (should be rare)
    if (framesWritten < framesRead) {
        AB_LOG_WARNING("AudioPipeline: Output underrun: wrote " +
                       std::to_string(framesWritten) + " of " +
                       std::to_string(framesRead) + " frames");
    }
}

size_t AudioPipeline::GetLatencyFrames() const {
    size_t latency = 0;

    // Add input buffer latency
    if (input_) {
        latency += input_->AvailableFrames();
    }

    // Add output buffer latency
    if (output_) {
        // Approximate: use available space as proxy for buffer depth
        latency += output_->AvailableSpace();
    }

    return latency;
}

void AudioPipeline::Reset() {
    passThroughEnabled_.store(false);
    std::fill(transferBuffer_, transferBuffer_ + kTransferBufferSize, 0.0f);
    AB_LOG_DEBUG("AudioPipeline: Reset");
}

}  // namespace audiobridge
