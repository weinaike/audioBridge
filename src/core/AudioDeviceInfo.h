#ifndef AUDIOBRIDGE_AUDIODEVICEINFO_H
#define AUDIOBRIDGE_AUDIODEVICEINFO_H

#include <string>

namespace audiobridge {

// Audio device information structure
struct AudioDeviceInfo {
    int deviceId = -1;                    // PortAudio device index
    std::string name;                     // Device display name
    std::string hostApi;                  // Host API name (WASAPI/CoreAudio/ALSA)
    int maxInputChannels = 0;             // Maximum input channels
    int maxOutputChannels = 0;            // Maximum output channels
    double defaultSampleRate = 0.0;       // Default sample rate
    bool isDefaultInput = false;          // Is system default input device
    bool isDefaultOutput = false;         // Is system default output device

    /// Validate device info
    bool IsValid() const {
        return deviceId >= 0 &&
               (maxInputChannels > 0 || maxOutputChannels > 0) &&
               defaultSampleRate > 0.0;
    }

    /// Check if device supports input
    bool SupportsInput() const {
        return maxInputChannels > 0;
    }

    /// Check if device supports output
    bool SupportsOutput() const {
        return maxOutputChannels > 0;
    }
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_AUDIODEVICEINFO_H
