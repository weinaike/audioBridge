#ifndef AUDIOBRIDGE_DEVICEENUMERATOR_H
#define AUDIOBRIDGE_DEVICEENUMERATOR_H

#include "adapters/IDeviceEnumerator.h"
#include "core/AudioDeviceInfo.h"
#include <vector>
#include <string>
#include <portaudio.h>

namespace audiobridge {

/// PortAudio-based device enumerator implementation
class DeviceEnumerator : public IDeviceEnumerator {
public:
    DeviceEnumerator();
    ~DeviceEnumerator() override;

    // Disable copy and move
    DeviceEnumerator(const DeviceEnumerator&) = delete;
    DeviceEnumerator& operator=(const DeviceEnumerator&) = delete;
    DeviceEnumerator(DeviceEnumerator&&) = delete;
    DeviceEnumerator& operator=(DeviceEnumerator&&) = delete;

    /// Get all available devices
    /// @return List of all device information
    std::vector<AudioDeviceInfo> GetAllDevices() override;

    /// Get all input devices
    /// @return List of devices supporting input
    std::vector<AudioDeviceInfo> GetInputDevices() override;

    /// Get all output devices
    /// @return List of devices supporting output
    std::vector<AudioDeviceInfo> GetOutputDevices() override;

    /// Refresh device list (terminates and reinitializes PortAudio)
    void Refresh() override;

    /// Get system default input device
    /// @return Device ID, -1 if no device available
    int GetDefaultInputDevice() override;

    /// Get system default output device
    /// @return Device ID, -1 if no device available
    int GetDefaultOutputDevice() override;

    /// Validate that input and output devices are different
    /// @param inputDeviceId Input device ID
    /// @param outputDeviceId Output device ID
    /// @return true if devices are different, false if same device
    bool ValidateDeviceSelection(int inputDeviceId, int outputDeviceId) const;

    /// Get device info by device ID
    /// @param deviceId PortAudio device index
    /// @return Device information, invalid device if not found
    AudioDeviceInfo GetDeviceInfo(int deviceId) const;

private:
    /// Convert PortAudio device info to AudioDeviceInfo
    /// @param deviceId PortAudio device index
    /// @return Converted device information
    AudioDeviceInfo ConvertPaDeviceInfo(PaDeviceIndex deviceId) const;

    /// Check if PortAudio is initialized
    /// @return true if initialized
    bool IsInitialized() const;

    bool initialized_;  // Track PortAudio initialization state
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_DEVICEENUMERATOR_H
