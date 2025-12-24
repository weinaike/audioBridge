/**
 * VirtualDeviceManager.cpp
 *
 * Implementation of virtual audio device enumeration and management
 */

#include "VirtualDeviceManager.h"
#include "utils/PortAudioHelper.h"
#include <spdlog/spdlog.h>
#include <portaudio.h>
#include <regex>
#include <fstream>
#include <sstream>

namespace audioBridge {
namespace testing {

VirtualDeviceManager::VirtualDeviceManager() {
    spdlog::debug("VirtualDeviceManager initialized");
}

VirtualDeviceManager::~VirtualDeviceManager() {
    spdlog::debug("VirtualDeviceManager destroyed");
}

std::vector<VirtualAudioDevice> VirtualDeviceManager::enumerateDevices() {
    std::vector<VirtualAudioDevice> devices;

    // Initialize PortAudio if not already initialized
    PaError err = Pa_Initialize();
    if (err != paNoError && err != paNotInitialized) {
        lastError_ = std::string("PortAudio initialization failed: ") + Pa_GetErrorText(err);
        spdlog::error(lastError_);
        return devices;
    }

    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        lastError_ = std::string("Pa_GetDeviceCount failed: ") + Pa_GetErrorText(numDevices);
        spdlog::error(lastError_);
        return devices;
    }

    spdlog::info("Enumerating {} audio devices", numDevices);

    for (int i = 0; i < numDevices; i++) {
        VirtualAudioDevice device;
        device.deviceId = i;
        parseDeviceInfo(i, device);
        devices.push_back(device);

        spdlog::debug("  [{}] {} - Type: {}, Direction: {}, Channels: {}",
                     i, device.deviceName,
                     static_cast<int>(device.deviceType),
                     static_cast<int>(device.direction),
                     device.maxChannels);
    }

    return devices;
}

std::vector<VirtualAudioDevice> VirtualDeviceManager::findLoopbackDevices() {
    auto allDevices = enumerateDevices();
    std::vector<VirtualAudioDevice> loopbackDevices;

    for (const auto& device : allDevices) {
        if (device.isLoopback()) {
            loopbackDevices.push_back(device);
        }
    }

    spdlog::info("Found {} loopback devices", loopbackDevices.size());
    return loopbackDevices;
}

bool VirtualDeviceManager::autoDetectLoopbackPair(VirtualAudioDevice& outPlayback,
                                                    VirtualAudioDevice& outCapture) {
    auto loopbackDevices = findLoopbackDevices();

    if (loopbackDevices.empty()) {
        lastError_ = "No loopback devices found. Load snd-aloop module: sudo modprobe snd-aloop";
        spdlog::error(lastError_);
        return false;
    }

    // Find output device
    VirtualAudioDevice* playbackPtr = nullptr;
    VirtualAudioDevice* capturePtr = nullptr;

    for (auto& device : loopbackDevices) {
        if (device.direction == VirtualAudioDevice::Direction::OUTPUT && !playbackPtr) {
            playbackPtr = &device;
        } else if (device.direction == VirtualAudioDevice::Direction::INPUT && !capturePtr) {
            capturePtr = &device;
        }

        if (playbackPtr && capturePtr) {
            break;
        }
    }

    // Handle duplex devices
    if (!playbackPtr || !capturePtr) {
        for (auto& device : loopbackDevices) {
            if (device.direction == VirtualAudioDevice::Direction::DUPLEX) {
                if (!playbackPtr) playbackPtr = &device;
                if (!capturePtr) capturePtr = &device;
                break;
            }
        }
    }

    if (!playbackPtr || !capturePtr) {
        lastError_ = "Could not find suitable loopback input/output pair";
        spdlog::error(lastError_);
        return false;
    }

    outPlayback = *playbackPtr;
    outCapture = *capturePtr;

    spdlog::info("Auto-detected loopback pair:");
    spdlog::info("  Playback: [{}] {}", outPlayback.deviceId, outPlayback.deviceName);
    spdlog::info("  Capture:  [{}] {}", outCapture.deviceId, outCapture.deviceName);

    return true;
}

std::vector<VirtualAudioDevice> VirtualDeviceManager::findDevicesByPattern(
    const std::string& pattern,
    VirtualAudioDevice::Direction direction) {

    std::vector<VirtualAudioDevice> matchedDevices;
    auto allDevices = enumerateDevices();

    try {
        std::regex regex(pattern, std::regex::icase);

        for (const auto& device : allDevices) {
            if (device.direction == direction || device.direction == VirtualAudioDevice::Direction::DUPLEX) {
                if (std::regex_search(device.deviceName, regex)) {
                    matchedDevices.push_back(device);
                }
            }
        }
    } catch (const std::regex_error& e) {
        lastError_ = std::string("Invalid regex pattern: ") + e.what();
        spdlog::error(lastError_);
    }

    spdlog::info("Pattern '{}' matched {} devices with direction {}", pattern,
                 matchedDevices.size(), static_cast<int>(direction));

    return matchedDevices;
}

bool VirtualDeviceManager::getDeviceById(int deviceId, VirtualAudioDevice& outDevice) {
    auto allDevices = enumerateDevices();

    for (const auto& device : allDevices) {
        if (device.deviceId == deviceId) {
            outDevice = device;
            return true;
        }
    }

    lastError_ = "Device ID " + std::to_string(deviceId) + " not found";
    spdlog::error(lastError_);
    return false;
}

bool VirtualDeviceManager::isLoopbackModuleLoaded() {
    std::ifstream modules("/proc/modules");
    std::string line;

    while (std::getline(modules, line)) {
        if (line.find("snd_aloop") != std::string::npos) {
            spdlog::debug("snd-aloop module is loaded");
            return true;
        }
    }

    spdlog::warn("snd-aloop module not loaded");
    return false;
}

void VirtualDeviceManager::parseDeviceInfo(int index, VirtualAudioDevice& device) {
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);

    if (!deviceInfo) {
        spdlog::error("Failed to get device info for index {}", index);
        return;
    }

    // Device name
    device.deviceName = deviceInfo->name;

    // Detect device type
    device.deviceType = detectDeviceType(device.deviceName);

    // Direction: Determine based on input/output capabilities
    bool hasInput = deviceInfo->maxInputChannels > 0;
    bool hasOutput = deviceInfo->maxOutputChannels > 0;

    if (hasInput && hasOutput) {
        device.direction = VirtualAudioDevice::Direction::DUPLEX;
    } else if (hasInput) {
        device.direction = VirtualAudioDevice::Direction::INPUT;
    } else if (hasOutput) {
        device.direction = VirtualAudioDevice::Direction::OUTPUT;
    } else {
        device.direction = VirtualAudioDevice::Direction::DUPLEX; // Fallback
    }

    // Max channels (prioritize input for capture, output for playback)
    device.maxChannels = std::max(deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);

    // Default sample rate
    if (deviceInfo->defaultSampleRate > 0) {
        device.defaultSampleRate = static_cast<double>(deviceInfo->defaultSampleRate);
    } else {
        device.defaultSampleRate = 48000.0; // Default fallback
    }

    // Availability
    device.isAvailable = true; // PortAudio only enumerates available devices

    // ALSA name (extract from device name if available)
    if (device.deviceName.find("hw:") != std::string::npos) {
        device.alsaName = device.deviceName;
    }

    // Module
    if (device.isLoopback()) {
        device.module = "snd-aloop";
    }
}

bool VirtualDeviceManager::isLoopbackDevice(const std::string& deviceName) const {
    // Common loopback device patterns
    static const std::vector<std::string> loopbackPatterns = {
        "loopback",
        "Loopback",
        "LOOPBACK",
        "snd-aloop",
        "ALSA"
    };

    for (const auto& pattern : loopbackPatterns) {
        if (deviceName.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

VirtualAudioDevice::DeviceType VirtualDeviceManager::detectDeviceType(
    const std::string& deviceName) const {

    if (isLoopbackDevice(deviceName)) {
        return VirtualAudioDevice::DeviceType::LOOPBACK;
    }

    // Check for virtual device indicators
    if (deviceName.find("virtual") != std::string::npos ||
        deviceName.find("Virtual") != std::string::npos) {
        return VirtualAudioDevice::DeviceType::VIRTUAL;
    }

    // Default to physical
    return VirtualAudioDevice::DeviceType::PHYSICAL;
}

} // namespace testing
} // namespace audioBridge
