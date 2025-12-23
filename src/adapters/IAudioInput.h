#ifndef AUDIOBRIDGE_IAUDIOINPUT_H
#define AUDIOBRIDGE_IAUDIOINPUT_H

#include "core/Types.h"
#include "core/AudioDeviceInfo.h"
#include <memory>

namespace audiobridge {

/// Abstract audio input interface following Adapter pattern
class IAudioInput {
public:
    virtual ~IAudioInput() = default;

    //--- Lifecycle management ---

    /// Open audio input stream
    /// @param deviceId Device ID
    /// @param config Stream configuration
    /// @return true on success
    virtual bool Open(int deviceId, const AudioStreamConfig& config) = 0;

    /// Start stream (call after Open)
    /// @return true on success
    virtual bool Start() = 0;

    /// Stop stream
    virtual void Stop() = 0;

    /// Close stream (release resources)
    virtual void Close() = 0;

    //--- Status query ---

    /// Get current stream state
    virtual StreamState GetState() const = 0;

    /// Check if stream is active
    virtual bool IsActive() const = 0;

    /// Get device information
    virtual AudioDeviceInfo GetDeviceInfo() const = 0;

    //--- Data access ---

    /// Get available frame count
    /// @note RT-safe, can be called from audio thread
    virtual size_t AvailableFrames() const = 0;

    /// Read audio data
    /// @param buffer Destination buffer
    /// @return Actual frames read
    /// @note RT-safe, can be called from audio thread
    virtual size_t Read(AudioBuffer& buffer) = 0;

    //--- Callback registration ---

    /// Set state change callback
    virtual void SetStateCallback(StreamStateCallback callback) = 0;
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_IAUDIOINPUT_H
