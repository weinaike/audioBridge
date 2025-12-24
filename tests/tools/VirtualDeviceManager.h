/**
 * VirtualDeviceManager.h
 *
 * Manages virtual audio device enumeration and selection for loopback testing
 * on Linux using ALSA snd-aloop kernel module.
 */

#ifndef VIRTUAL_DEVICE_MANAGER_H
#define VIRTUAL_DEVICE_MANAGER_H

#include <string>
#include <vector>
#include <memory>

namespace audioBridge {
namespace testing {

/**
 * Represents a virtual audio device
 */
struct VirtualAudioDevice {
    int deviceId;
    std::string deviceName;
    enum class DeviceType { LOOPBACK, PHYSICAL, VIRTUAL } deviceType;
    enum class Direction { INPUT, OUTPUT, DUPLEX } direction;
    int maxChannels;
    double defaultSampleRate;
    bool isAvailable;
    std::string alsaName;
    std::string module;

    // Helper to check if device is loopback
    bool isLoopback() const {
        return deviceType == DeviceType::LOOPBACK;
    }
};

// Type aliases for backward compatibility
using DeviceType = VirtualAudioDevice::DeviceType;
using DeviceDirection = VirtualAudioDevice::Direction;

/**
 * Manages enumeration and selection of virtual audio devices
 */
class VirtualDeviceManager {
public:
    VirtualDeviceManager();
    ~VirtualDeviceManager();

    // Enumerate all available audio devices
    std::vector<VirtualAudioDevice> enumerateDevices();

    // Find loopback devices specifically
    std::vector<VirtualAudioDevice> findLoopbackDevices();

    // Auto-detect best loopback pair (input and output)
    bool autoDetectLoopbackPair(VirtualAudioDevice& outPlayback, VirtualAudioDevice& outCapture);

    // Find device by pattern matching
    std::vector<VirtualAudioDevice> findDevicesByPattern(const std::string& pattern,
                                                          VirtualAudioDevice::Direction direction);

    // Get device by ID
    bool getDeviceById(int deviceId, VirtualAudioDevice& outDevice);

    // Check if ALSA loopback module is loaded
    bool isLoopbackModuleLoaded();

    // Get system error message
    std::string getLastError() const { return lastError_; }

private:
    std::string lastError_;

    // Helper: Parse PortAudio device info
    void parseDeviceInfo(int index, VirtualAudioDevice& device);

    // Helper: Check if device name matches loopback pattern
    bool isLoopbackDevice(const std::string& deviceName) const;

    // Helper: Detect device type from name
    VirtualAudioDevice::DeviceType detectDeviceType(const std::string& deviceName) const;
};

} // namespace testing
} // namespace audioBridge

#endif // VIRTUAL_DEVICE_MANAGER_H
