/**
 * AudioIOHandler.h
 *
 * Real-time audio playback and capture using PortAudio
 */

#ifndef AUDIO_IO_HANDLER_H
#define AUDIO_IO_HANDLER_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>

#ifdef SNDFILE_FOUND
#include <sndfile.h>
#endif

namespace audioBridge {
namespace testing {

/**
 * Audio playback/capture configuration
 */
struct AudioIOConfig {
    int sampleRate = 48000;
    int channels = 2;
    int framesPerBuffer = 512;
    std::string playbackDevice;
    std::string captureDevice;
};

/**
 * Audio I/O callback data
 */
struct AudioCallbackData {
    std::vector<float>* playbackBuffer = nullptr;
    std::vector<float>* captureBuffer = nullptr;
    std::atomic<size_t> playbackPosition{0};
    std::atomic<size_t> capturePosition{0};
    std::atomic<bool> playbackComplete{false};
    std::atomic<bool> captureComplete{false};
    size_t totalFrames = 0;
    int channels = 2;
};

/**
 * Audio I/O handler for real-time playback and capture
 */
class AudioIOHandler {
public:
    AudioIOHandler();
    ~AudioIOHandler();

    // Initialize audio I/O
    bool initialize(const AudioIOConfig& config);

    // Load audio file for playback
    bool loadAudioFile(const std::string& filepath,
                       int& outSampleRate,
                       int& outChannels,
                       std::vector<float>& outAudio);

    // Start playback and capture (full-duplex loopback test)
    bool startLoopbackTest(const std::vector<float>& playbackAudio,
                           std::vector<float>& capturedAudio,
                           int sampleRate,
                           int channels);

    // Simple playback only
    bool playAudio(const std::vector<float>& audio,
                   int sampleRate,
                   int channels,
                   const std::string& deviceName = "");

    // Simple capture only
    bool captureAudio(std::vector<float>& audio,
                      int sampleRate,
                      int channels,
                      float durationSeconds,
                      const std::string& deviceName = "");

    // Stop all audio I/O
    void stop();

    // Check if currently playing/capturing
    bool isActive() const { return active_; }

    // Get last error
    std::string getLastError() const { return lastError_; }

    // Save captured audio to WAV file
    bool saveToWav(const std::string& filepath,
                   const std::vector<float>& audio,
                   int sampleRate,
                   int channels);

private:
    AudioIOConfig config_;
    bool active_;
    bool initialized_;
    std::string lastError_;

    // PortAudio stream (opaque pointer to avoid including portaudio.h in header)
    void* paStream_;

    // Helper: Find device index by name
    int findDeviceByName(const std::string& deviceName, bool forOutput);

    // Helper: Open default device
    bool openDefaultDevice(bool forOutput, int sampleRate, int channels);

    // Helper: Close stream
    void closeStream();
};

} // namespace testing
} // namespace audioBridge

#endif // AUDIO_IO_HANDLER_H
