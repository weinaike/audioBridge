#ifndef AUDIOBRIDGE_TYPES_H
#define AUDIOBRIDGE_TYPES_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace audiobridge {

// Sample format enumeration
enum class SampleFormat {
    Float32  // Current phase only supports float32
};

// Audio stream configuration
struct AudioStreamConfig {
    double sampleRate = 48000.0;
    int framesPerBuffer = 128;
    int channelCount = 2;
    SampleFormat format = SampleFormat::Float32;

    /// Validate configuration validity
    bool IsValid() const {
        return sampleRate == 48000.0 &&
               framesPerBuffer == 128 &&
               (channelCount == 1 || channelCount == 2) &&
               format == SampleFormat::Float32;
    }
};

// Audio buffer structure
struct AudioBuffer {
    float* data = nullptr;          // Interleaved audio data
    size_t frameCount = 0;          // Number of frames
    int channelCount = 0;           // Number of channels
    uint64_t timestamp = 0;         // Optional timestamp (microseconds)

    /// Get total sample count
    size_t GetSampleCount() const {
        return frameCount * channelCount;
    }

    /// Get data size in bytes
    size_t GetByteSize() const {
        return GetSampleCount() * sizeof(float);
    }
};

// Stream state enumeration
enum class StreamState {
    Stopped,    // Stream is stopped
    Starting,   // Stream is starting
    Active,     // Stream is active and running
    Stopping,   // Stream is stopping
    Error       // Stream encountered an error
};

// Stream state callback type
using StreamStateCallback = std::function<void(StreamState newState)>;

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_TYPES_H
