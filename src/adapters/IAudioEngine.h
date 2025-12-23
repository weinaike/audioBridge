#ifndef AUDIOBRIDGE_IAUDIOENGINE_H
#define AUDIOBRIDGE_IAUDIOENGINE_H

#include "core/Types.h"
#include "core/AudioLevels.h"
#include <functional>
#include <memory>

namespace audiobridge {

// Level callback type (from AudioLevels.h)
using LevelCallback = std::function<void(const AudioLevels& levels)>;

// Forward declarations
class IDeviceEnumerator;
class IAudioInput;
class IAudioOutput;

/// Audio engine interface
/// Manages audio input/output devices and pass-through functionality
class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    //--- Device Management ---

    /// Get device enumerator
    /// @return Reference to device enumerator
    virtual IDeviceEnumerator& GetDeviceEnumerator() = 0;

    /// Select input device
    /// @param deviceId Device ID from device enumerator
    /// @return true on success
    virtual bool SelectInputDevice(int deviceId) = 0;

    /// Select output device
    /// @param deviceId Device ID from device enumerator
    /// @return true on success
    virtual bool SelectOutputDevice(int deviceId) = 0;

    //--- Engine Control ---

    /// Start audio engine
    /// @return true on success
    virtual bool Start() = 0;

    /// Stop audio engine
    virtual void Stop() = 0;

    /// Check if engine is running
    /// @return true if running
    virtual bool IsRunning() const = 0;

    //--- Pass-through Mode ---

    /// Enable/disable pass-through mode
    /// @param enabled true to enable pass-through
    virtual void SetPassThroughEnabled(bool enabled) = 0;

    /// Check pass-through mode status
    /// @return true if pass-through is enabled
    virtual bool IsPassThroughEnabled() const = 0;

    //--- Monitoring ---

    /// Set level callback for real-time monitoring
    /// @param callback Function to call with level updates
    virtual void SetLevelCallback(LevelCallback callback) = 0;

    /// Get current latency in milliseconds
    /// @return Latency in ms
    virtual double GetCurrentLatency() const = 0;
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_IAUDIOENGINE_H
