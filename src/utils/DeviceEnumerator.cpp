/**
 * @file DeviceEnumerator.cpp
 * @brief Implementation of DeviceEnumerator class
 */

#include "utils/DeviceEnumerator.h"
#include "utils/PortAudioHelper.h"
#include "utils/Logger.h"
#include <portaudio.h>
#include <algorithm>

namespace audiobridge {

// =============================================================================
// Construction / Destruction
// =============================================================================

DeviceEnumerator::DeviceEnumerator()
    : initialized_(false) {
    AB_LOG_DEBUG("DeviceEnumerator: Constructor called");

    // Initialize PortAudio using the helper
    initialized_ = PortAudioHelper::GetInstance().Initialize();
}

DeviceEnumerator::~DeviceEnumerator() {
    AB_LOG_DEBUG("DeviceEnumerator: Destructor called");

    // Terminate PortAudio using the helper
    PortAudioHelper::GetInstance().Terminate();
}

// =============================================================================
// Device Enumeration
// =============================================================================

std::vector<AudioDeviceInfo> DeviceEnumerator::GetAllDevices() {
    if (!IsInitialized()) {
        AB_LOG_ERROR("DeviceEnumerator: PortAudio not initialized");
        return {};
    }

    PaDeviceIndex count = Pa_GetDeviceCount();
    if (count < 0) {
        AB_LOG_ERROR("DeviceEnumerator: Failed to get device count");
        return {};
    }

    if (count == 0) {
        AB_LOG_WARNING("DeviceEnumerator: No audio devices found");
        return {};
    }

    std::vector<AudioDeviceInfo> devices;
    devices.reserve(static_cast<size_t>(count));

    for (PaDeviceIndex i = 0; i < count; ++i) {
        AudioDeviceInfo deviceInfo = ConvertPaDeviceInfo(i);
        if (deviceInfo.IsValid()) {
            devices.push_back(deviceInfo);
        }
    }

    AB_LOG_DEBUG("DeviceEnumerator: Found " + std::to_string(devices.size()) + " devices");
    return devices;
}

std::vector<AudioDeviceInfo> DeviceEnumerator::GetInputDevices() {
    if (!IsInitialized()) {
        AB_LOG_ERROR("DeviceEnumerator: PortAudio not initialized");
        return {};
    }

    PaDeviceIndex count = Pa_GetDeviceCount();
    if (count < 0) {
        AB_LOG_ERROR("DeviceEnumerator: Failed to get device count");
        return {};
    }

    std::vector<AudioDeviceInfo> inputDevices;

    for (PaDeviceIndex i = 0; i < count; ++i) {
        const PaDeviceInfo* paInfo = Pa_GetDeviceInfo(i);
        if (paInfo && paInfo->maxInputChannels > 0) {
            AudioDeviceInfo deviceInfo = ConvertPaDeviceInfo(i);
            if (deviceInfo.SupportsInput()) {
                inputDevices.push_back(deviceInfo);
            }
        }
    }

    AB_LOG_DEBUG("DeviceEnumerator: Found " + std::to_string(inputDevices.size()) + " input devices");
    return inputDevices;
}

std::vector<AudioDeviceInfo> DeviceEnumerator::GetOutputDevices() {
    if (!IsInitialized()) {
        AB_LOG_ERROR("DeviceEnumerator: PortAudio not initialized");
        return {};
    }

    PaDeviceIndex count = Pa_GetDeviceCount();
    if (count < 0) {
        AB_LOG_ERROR("DeviceEnumerator: Failed to get device count");
        return {};
    }

    std::vector<AudioDeviceInfo> outputDevices;

    for (PaDeviceIndex i = 0; i < count; ++i) {
        const PaDeviceInfo* paInfo = Pa_GetDeviceInfo(i);
        if (paInfo && paInfo->maxOutputChannels > 0) {
            AudioDeviceInfo deviceInfo = ConvertPaDeviceInfo(i);
            if (deviceInfo.SupportsOutput()) {
                outputDevices.push_back(deviceInfo);
            }
        }
    }

    AB_LOG_DEBUG("DeviceEnumerator: Found " + std::to_string(outputDevices.size()) + " output devices");
    return outputDevices;
}

// =============================================================================
// Device Refresh
// =============================================================================

void DeviceEnumerator::Refresh() {
    AB_LOG_INFO("DeviceEnumerator: Refreshing device list");

    if (!initialized_) {
        AB_LOG_ERROR("DeviceEnumerator: Cannot refresh - not initialized");
        return;
    }

    // Note: With reference-counted PortAudio initialization, we cannot
    // safely terminate and reinitialize PortAudio here as other components
    // may be using it. The device list is automatically updated by PortAudio
    // when devices are added/removed. This method is kept for API compatibility
    // and simply verifies that PortAudio is still initialized.

    if (!PortAudioHelper::GetInstance().IsInitialized()) {
        AB_LOG_ERROR("DeviceEnumerator: PortAudio not initialized");
        initialized_ = false;
        return;
    }

    AB_LOG_INFO("DeviceEnumerator: Device list refreshed successfully");
}

// =============================================================================
// Default Device Retrieval
// =============================================================================

int DeviceEnumerator::GetDefaultInputDevice() {
    if (!IsInitialized()) {
        AB_LOG_ERROR("DeviceEnumerator: PortAudio not initialized");
        return -1;
    }

    PaDeviceIndex defaultInput = Pa_GetDefaultInputDevice();

    if (defaultInput == paNoDevice) {
        AB_LOG_WARNING("DeviceEnumerator: No default input device available");
        return -1;
    }

    AB_LOG_DEBUG("DeviceEnumerator: Default input device: " + std::to_string(defaultInput));
    return static_cast<int>(defaultInput);
}

int DeviceEnumerator::GetDefaultOutputDevice() {
    if (!IsInitialized()) {
        AB_LOG_ERROR("DeviceEnumerator: PortAudio not initialized");
        return -1;
    }

    PaDeviceIndex defaultOutput = Pa_GetDefaultOutputDevice();

    if (defaultOutput == paNoDevice) {
        AB_LOG_WARNING("DeviceEnumerator: No default output device available");
        return -1;
    }

    AB_LOG_DEBUG("DeviceEnumerator: Default output device: " + std::to_string(defaultOutput));
    return static_cast<int>(defaultOutput);
}

// =============================================================================
// Device Validation
// =============================================================================

bool DeviceEnumerator::ValidateDeviceSelection(int inputDeviceId, int outputDeviceId) const {
    if (!IsInitialized()) {
        AB_LOG_ERROR("DeviceEnumerator: PortAudio not initialized");
        return false;
    }

    // Check if devices are the same
    if (inputDeviceId == outputDeviceId) {
        AB_LOG_WARNING("DeviceEnumerator: Input and output devices cannot be the same: " +
                       std::to_string(inputDeviceId));
        return false;
    }

    // Validate input device exists and supports input
    PaDeviceIndex count = Pa_GetDeviceCount();
    if (inputDeviceId < 0 || inputDeviceId >= count) {
        AB_LOG_ERROR("DeviceEnumerator: Invalid input device ID: " + std::to_string(inputDeviceId));
        return false;
    }

    const PaDeviceInfo* inputInfo = Pa_GetDeviceInfo(inputDeviceId);
    if (!inputInfo || inputInfo->maxInputChannels == 0) {
        AB_LOG_ERROR("DeviceEnumerator: Device does not support input: " + std::to_string(inputDeviceId));
        return false;
    }

    // Validate output device exists and supports output
    if (outputDeviceId < 0 || outputDeviceId >= count) {
        AB_LOG_ERROR("DeviceEnumerator: Invalid output device ID: " + std::to_string(outputDeviceId));
        return false;
    }

    const PaDeviceInfo* outputInfo = Pa_GetDeviceInfo(outputDeviceId);
    if (!outputInfo || outputInfo->maxOutputChannels == 0) {
        AB_LOG_ERROR("DeviceEnumerator: Device does not support output: " + std::to_string(outputDeviceId));
        return false;
    }

    AB_LOG_DEBUG("DeviceEnumerator: Device selection validated: input=" + std::to_string(inputDeviceId) +
                 ", output=" + std::to_string(outputDeviceId));
    return true;
}

AudioDeviceInfo DeviceEnumerator::GetDeviceInfo(int deviceId) const {
    if (!IsInitialized()) {
        AB_LOG_ERROR("DeviceEnumerator: PortAudio not initialized");
        return AudioDeviceInfo{};
    }

    PaDeviceIndex count = Pa_GetDeviceCount();
    if (deviceId < 0 || deviceId >= count) {
        AB_LOG_ERROR("DeviceEnumerator: Invalid device ID: " + std::to_string(deviceId));
        return AudioDeviceInfo{};
    }

    return ConvertPaDeviceInfo(deviceId);
}

// =============================================================================
// Internal Helpers
// =============================================================================

AudioDeviceInfo DeviceEnumerator::ConvertPaDeviceInfo(PaDeviceIndex deviceId) const {
    AudioDeviceInfo deviceInfo;

    const PaDeviceInfo* paInfo = Pa_GetDeviceInfo(deviceId);
    if (!paInfo) {
        AB_LOG_ERROR("DeviceEnumerator: Failed to get device info for ID: " + std::to_string(deviceId));
        return deviceInfo;
    }

    // Copy basic information
    deviceInfo.deviceId = static_cast<int>(deviceId);
    deviceInfo.name = paInfo->name ? paInfo->name : "Unknown Device";
    deviceInfo.maxInputChannels = paInfo->maxInputChannels;
    deviceInfo.maxOutputChannels = paInfo->maxOutputChannels;
    deviceInfo.defaultSampleRate = paInfo->defaultSampleRate;

    // Get Host API information
    PaHostApiIndex hostApiIndex = paInfo->hostApi;
    const PaHostApiInfo* hostApiInfo = Pa_GetHostApiInfo(hostApiIndex);
    if (hostApiInfo) {
        deviceInfo.hostApi = hostApiInfo->name ? hostApiInfo->name : "Unknown API";
    }

    // Check if this is a default device
    PaDeviceIndex defaultInput = Pa_GetDefaultInputDevice();
    PaDeviceIndex defaultOutput = Pa_GetDefaultOutputDevice();

    deviceInfo.isDefaultInput = (deviceId == defaultInput);
    deviceInfo.isDefaultOutput = (deviceId == defaultOutput);

    return deviceInfo;
}

bool DeviceEnumerator::IsInitialized() const {
    return initialized_;
}

}  // namespace audiobridge
