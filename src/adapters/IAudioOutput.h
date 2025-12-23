#ifndef AUDIOBRIDGE_IAUDIOOUTPUT_H
#define AUDIOBRIDGE_IAUDIOOUTPUT_H

#include "core/Types.h"
#include "core/AudioDeviceInfo.h"
#include <memory>

namespace audiobridge {

/// Abstract audio output interface following Adapter pattern
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    //--- Lifecycle management ---

    /// Open audio output stream
    /// @param deviceId Device ID
    /// @param config Stream configuration
    /// @return true on success
    virtual bool Open(int deviceId, const AudioStreamConfig& config) = 0;

    /// Start stream
    /// @return true on success
    virtual bool Start() = 0;

    /// Stop stream
    virtual void Stop() = 0;

    /// Close stream
    virtual void Close() = 0;

    //--- Status query ---

    /// Get current stream state
    virtual StreamState GetState() const = 0;

    /// Check if stream is active
    virtual bool IsActive() const = 0;

    /// Get device information
    virtual AudioDeviceInfo GetDeviceInfo() const = 0;

    //--- Data access ---

    /// Get available frame count for writing
    /// @note RT-safe
    virtual size_t AvailableSpace() const = 0;

    /// Write audio data
    /// @param buffer Source buffer
    /// @return Actual frames written
    /// @note RT-safe
    virtual size_t Write(const AudioBuffer& buffer) = 0;

    //--- Callback registration ---

    /// Set state change callback
    virtual void SetStateCallback(StreamStateCallback callback) = 0;
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_IAUDIOOUTPUT_H
