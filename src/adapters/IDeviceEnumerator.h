#ifndef AUDIOBRIDGE_IDEVICEENUMERATOR_H
#define AUDIOBRIDGE_IDEVICEENUMERATOR_H

#include "core/AudioDeviceInfo.h"
#include <vector>

namespace audiobridge {

/// Abstract device enumerator interface
class IDeviceEnumerator {
public:
    virtual ~IDeviceEnumerator() = default;

    /// Get all available devices
    /// @return List of device information
    virtual std::vector<AudioDeviceInfo> GetAllDevices() = 0;

    /// Get all input devices
    /// @return List of devices supporting input
    virtual std::vector<AudioDeviceInfo> GetInputDevices() = 0;

    /// Get all output devices
    /// @return List of devices supporting output
    virtual std::vector<AudioDeviceInfo> GetOutputDevices() = 0;

    /// Refresh device list
    virtual void Refresh() = 0;

    /// Get system default input device
    /// @return Device ID, -1 if no device available
    virtual int GetDefaultInputDevice() = 0;

    /// Get system default output device
    /// @return Device ID, -1 if no device available
    virtual int GetDefaultOutputDevice() = 0;
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_IDEVICEENUMERATOR_H
