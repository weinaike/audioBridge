#ifndef AUDIOBRIDGE_FACTORY_H
#define AUDIOBRIDGE_FACTORY_H

#include "adapters/IAudioEngine.h"
#include "adapters/IAudioInput.h"
#include "adapters/IAudioOutput.h"
#include "adapters/IDeviceEnumerator.h"
#include <memory>

namespace audiobridge {

/// Factory functions for creating audio components
/// Note: This is the concrete factory. The PortAudio-specific implementations
/// are created directly, maintaining simplicity for this phase.

/// Create audio engine
/// @return Unique pointer to audio engine instance
std::unique_ptr<IAudioEngine> CreateAudioEngine();

/// Create PortAudio input adapter
/// @return Unique pointer to audio input instance
std::unique_ptr<IAudioInput> CreatePortAudioInput();

/// Create PortAudio output adapter
/// @return Unique pointer to audio output instance
std::unique_ptr<IAudioOutput> CreatePortAudioOutput();

/// Create PortAudio device enumerator
/// @return Unique pointer to device enumerator instance
std::unique_ptr<IDeviceEnumerator> CreatePortAudioEnumerator();

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_FACTORY_H
