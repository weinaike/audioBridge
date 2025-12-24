#ifndef AUDIOBRIDGE_AUDIOPIPELINE_H
#define AUDIOBRIDGE_AUDIOPIPELINE_H

#include "core/Types.h"
#include "core/RingBuffer.h"
#include <atomic>
#include <memory>

namespace audiobridge {

// Forward declarations
class IAudioInput;
class IAudioOutput;

/// Audio pipeline for routing audio from input to output
/// Manages RT-safe data flow between ring buffers
class AudioPipeline {
public:
    AudioPipeline();
    ~AudioPipeline();

    // Disable copy and move
    AudioPipeline(const AudioPipeline&) = delete;
    AudioPipeline& operator=(const AudioPipeline&) = delete;
    AudioPipeline(AudioPipeline&&) = delete;
    AudioPipeline& operator=(AudioPipeline&&) = delete;

    /// Set input source
    /// @param input Audio input interface
    void SetInput(IAudioInput* input);

    /// Set output destination
    /// @param output Audio output interface
    void SetOutput(IAudioOutput* output);

    /// Enable/disable pass-through
    /// @param enabled true to route input to output
    void SetPassThroughEnabled(bool enabled);

    /// Check if pass-through is enabled
    /// @return true if enabled
    bool IsPassThroughEnabled() const;

    /// Process audio (called from audio thread)
    /// Reads from input ring buffer and writes to output ring buffer
    /// @note RT-safe
    void Process();

    /// Get current latency in frames
    /// @return Total latency (input + output + processing)
    size_t GetLatencyFrames() const;

    /// Reset pipeline state
    void Reset();

private:
    IAudioInput* input_;
    IAudioOutput* output_;
    std::atomic<bool> passThroughEnabled_;

    static constexpr size_t kTransferBufferSize = 512;
    float transferBuffer_[kTransferBufferSize];
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_AUDIOPIPELINE_H
