/**
 * @file Factory.cpp
 * @brief Implementation of factory functions
 */

#include "core/Factory.h"
#include "core/AudioEngine.h"
#include "adapters/PortAudioInput.h"
#include "adapters/PortAudioOutput.h"
#include "utils/DeviceEnumerator.h"

namespace audiobridge {

std::unique_ptr<IAudioEngine> CreateAudioEngine() {
    return std::make_unique<AudioEngine>();
}

std::unique_ptr<IAudioInput> CreatePortAudioInput() {
    return std::make_unique<PortAudioInput>();
}

std::unique_ptr<IAudioOutput> CreatePortAudioOutput() {
    return std::make_unique<PortAudioOutput>();
}

std::unique_ptr<IDeviceEnumerator> CreatePortAudioEnumerator() {
    return std::make_unique<DeviceEnumerator>();
}

}  // namespace audiobridge
