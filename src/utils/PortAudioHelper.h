#ifndef AUDIOBRIDGE_PORTAUDIOHELPER_H
#define AUDIOBRIDGE_PORTAUDIOHELPER_H

#include <portaudio.h>

namespace audiobridge {

/// PortAudio initialization helper with reference counting
/// Ensures Pa_Initialize() is called once and Pa_Terminate() is called
/// when the last user is done
class PortAudioHelper {
public:
    /// Get the singleton instance
    /// @return Reference to the singleton instance
    static PortAudioHelper& GetInstance();

    /// Increment reference count and initialize PortAudio if needed
    /// @return true on success
    bool Initialize();

    /// Decrement reference count and terminate PortAudio if no users remain
    void Terminate();

    /// Check if PortAudio is initialized
    /// @return true if initialized
    bool IsInitialized() const;

    // Disable copy and move
    PortAudioHelper(const PortAudioHelper&) = delete;
    PortAudioHelper& operator=(const PortAudioHelper&) = delete;
    PortAudioHelper(PortAudioHelper&&) = delete;
    PortAudioHelper& operator=(PortAudioHelper&&) = delete;

private:
    PortAudioHelper() = default;
    ~PortAudioHelper();

    int refCount_ = 0;  // Reference count for PortAudio users
    bool initialized_ = false;  // Track PortAudio initialization state
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_PORTAUDIOHELPER_H
