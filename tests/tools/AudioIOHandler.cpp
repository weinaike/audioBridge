/**
 * AudioIOHandler.cpp
 *
 * Real-time audio playback and capture using PortAudio
 */

#include "AudioIOHandler.h"
#include <spdlog/spdlog.h>
#include <portaudio.h>
#include <cstring>
#include <algorithm>

#ifdef SNDFILE_FOUND
#include <sndfile.h>
#endif

namespace audioBridge {
namespace testing {

// PortAudio callback for loopback test (playback + capture)
static int loopbackCallback(const void* inputBuffer,
                           void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void* userData) {
    (void)timeInfo;
    (void)statusFlags;

    AudioCallbackData* data = static_cast<AudioCallbackData*>(userData);
    if (!data) {
        return paComplete;
    }

    float* out = static_cast<float*>(outputBuffer);
    const float* in = static_cast<const float*>(inputBuffer);

    // Handle playback
    if (out && data->playbackBuffer) {
        size_t framesToPlay = std::min(static_cast<size_t>(framesPerBuffer),
                                       data->totalFrames - data->playbackPosition.load());

        for (unsigned long i = 0; i < framesPerBuffer; ++i) {
            for (int ch = 0; ch < data->channels; ++ch) {
                if (i < framesToPlay) {
                    size_t srcIdx = data->playbackPosition.load() * data->channels + ch;
                    *out++ = (*data->playbackBuffer)[srcIdx];
                } else {
                    *out++ = 0.0f; // Silence after playback complete
                }
            }
        }

        data->playbackPosition += framesToPlay;
        if (data->playbackPosition >= data->totalFrames) {
            data->playbackComplete = true;
        }
    }

    // Handle capture
    if (in && data->captureBuffer) {
        size_t capturePos = data->capturePosition.load();
        size_t spaceAvailable = data->captureBuffer->size() - (capturePos * data->channels);

        size_t framesToCapture = std::min(static_cast<size_t>(framesPerBuffer),
                                          spaceAvailable / data->channels);

        for (unsigned long i = 0; i < framesToCapture; ++i) {
            for (int ch = 0; ch < data->channels; ++ch) {
                size_t dstIdx = capturePos * data->channels + (i * data->channels + ch);
                if (dstIdx < data->captureBuffer->size()) {
                    (*data->captureBuffer)[dstIdx] = *in++;
                }
            }
        }

        data->capturePosition += framesToCapture;

        // Stop capture if buffer is full
        if (capturePos * data->channels + framesPerBuffer * data->channels >= data->captureBuffer->size()) {
            data->captureComplete = true;
            return paComplete;
        }
    }

    // Continue if not complete
    if (data->playbackComplete && data->captureComplete) {
        return paComplete;
    }
    return paContinue;
}

// PortAudio callback for playback only
static int playbackCallback(const void* input,
                            void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData) {
    (void)input;
    (void)timeInfo;
    (void)statusFlags;

    AudioCallbackData* data = static_cast<AudioCallbackData*>(userData);
    if (!data || !data->playbackBuffer) {
        return paComplete;
    }

    float* out = static_cast<float*>(outputBuffer);
    size_t framesToPlay = std::min(static_cast<size_t>(framesPerBuffer),
                                   data->totalFrames - data->playbackPosition.load());

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        for (int ch = 0; ch < data->channels; ++ch) {
            if (i < framesToPlay) {
                size_t srcIdx = data->playbackPosition.load() * data->channels + ch;
                *out++ = (*data->playbackBuffer)[srcIdx];
            } else {
                *out++ = 0.0f;
            }
        }
    }

    data->playbackPosition += framesToPlay;
    if (data->playbackPosition >= data->totalFrames) {
        return paComplete;
    }

    return paContinue;
}

// PortAudio callback for capture only
static int captureCallback(const void* input,
                           void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void* userData) {
    (void)outputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    AudioCallbackData* data = static_cast<AudioCallbackData*>(userData);
    if (!data || !data->captureBuffer) {
        return paComplete;
    }

    const float* in = static_cast<const float*>(input);
    size_t capturePos = data->capturePosition.load();
    size_t spaceAvailable = data->captureBuffer->size() - (capturePos * data->channels);

    size_t framesToCapture = std::min(static_cast<size_t>(framesPerBuffer),
                                      spaceAvailable / data->channels);

    for (unsigned long i = 0; i < framesToCapture; ++i) {
        for (int ch = 0; ch < data->channels; ++ch) {
            size_t dstIdx = capturePos * data->channels + (i * data->channels + ch);
            if (dstIdx < data->captureBuffer->size()) {
                (*data->captureBuffer)[dstIdx] = *in++;
            }
        }
    }

    data->capturePosition += framesToCapture;

    // Stop if buffer is full
    if (capturePos * data->channels + framesPerBuffer * data->channels >= data->captureBuffer->size()) {
        return paComplete;
    }

    return paContinue;
}

// ============================================================================
// AudioIOHandler Implementation
// ============================================================================

AudioIOHandler::AudioIOHandler()
    : active_(false), initialized_(false), paStream_(nullptr) {
}

AudioIOHandler::~AudioIOHandler() {
    stop();
}

bool AudioIOHandler::initialize(const AudioIOConfig& config) {
    if (initialized_) {
        spdlog::warn("AudioIOHandler already initialized");
        return true;
    }

    spdlog::info("Initializing AudioIOHandler: sr={}, ch={}, buffer={}",
                 config.sampleRate, config.channels, config.framesPerBuffer);

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        lastError_ = std::string("PortAudio init failed: ") + Pa_GetErrorText(err);
        spdlog::error(lastError_);
        return false;
    }

    config_ = config;
    initialized_ = true;
    spdlog::info("PortAudio initialized successfully");
    return true;
}

bool AudioIOHandler::loadAudioFile(const std::string& filepath,
                                   int& outSampleRate,
                                   int& outChannels,
                                   std::vector<float>& outAudio) {
#ifdef SNDFILE_FOUND
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    SNDFILE* sndFile = sf_open(filepath.c_str(), SFM_READ, &sfinfo);
    if (!sndFile) {
        lastError_ = std::string("Failed to open audio file: ") + sf_strerror(nullptr);
        spdlog::error(lastError_);
        return false;
    }

    outSampleRate = sfinfo.samplerate;
    outChannels = sfinfo.channels;

    outAudio.resize(sfinfo.frames * outChannels);
    sf_count_t framesRead = sf_readf_float(sndFile, outAudio.data(), sfinfo.frames);

    sf_close(sndFile);

    spdlog::info("Loaded audio: {} frames, {} ch, {} Hz",
                 framesRead, outChannels, outSampleRate);
    return true;
#else
    lastError_ = "libsndfile not available";
    spdlog::error(lastError_);
    return false;
#endif
}

bool AudioIOHandler::startLoopbackTest(const std::vector<float>& playbackAudio,
                                        std::vector<float>& capturedAudio,
                                        int sampleRate,
                                        int channels) {
    if (!initialized_) {
        lastError_ = "AudioIOHandler not initialized";
        spdlog::error(lastError_);
        return false;
    }

    if (active_) {
        lastError_ = "Audio I/O already active";
        spdlog::error(lastError_);
        return false;
    }

    spdlog::info("Starting loopback test: {} frames, {} ch, {} Hz",
                 playbackAudio.size() / channels, channels, sampleRate);

    // Setup callback data
    AudioCallbackData callbackData;
    callbackData.playbackBuffer = const_cast<std::vector<float>*>(&playbackAudio);
    callbackData.captureBuffer = &capturedAudio;
    callbackData.totalFrames = playbackAudio.size() / channels;
    callbackData.channels = channels;
    callbackData.playbackPosition = 0;
    callbackData.capturePosition = 0;
    callbackData.playbackComplete = false;
    callbackData.captureComplete = false;

    // Pre-allocate capture buffer
    capturedAudio.resize(playbackAudio.size()); // Same size as playback

    // Try to find loopback devices for full-duplex operation
    std::vector<int> loopbackInputs;
    std::vector<int> loopbackOutputs;

    // First find all loopback devices
    int deviceCount = Pa_GetDeviceCount();
    for (int i = 0; i < deviceCount; ++i) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        std::string deviceName = deviceInfo->name;

        // Check for "Loopback" in device name
        if (deviceName.find("Loopback") != std::string::npos ||
            deviceName.find("loopback") != std::string::npos ||
            deviceName.find("hw:3") != std::string::npos) {

            spdlog::info("Found loopback device: {} [{}]", deviceName, i);

            if (deviceInfo->maxOutputChannels > 0) {
                loopbackOutputs.push_back(i);
            }
            if (deviceInfo->maxInputChannels > 0) {
                loopbackInputs.push_back(i);
            }
        }
    }

    // Select devices - use first output and second input for snd-aloop
    // (snd-aloop creates pairs where device N is output, device N+1 is input)
    int inputDevice = -1;
    int outputDevice = -1;

    if (!loopbackOutputs.empty()) {
        outputDevice = loopbackOutputs[0];
        spdlog::info("Using loopback output device: {}", outputDevice);
    }
    if (!loopbackInputs.empty()) {
        // For snd-aloop, use the second device in the pair (or different from output)
        for (int dev : loopbackInputs) {
            if (dev != outputDevice) {
                inputDevice = dev;
                break;
            }
        }
        if (inputDevice < 0 && !loopbackInputs.empty()) {
            inputDevice = loopbackInputs[0];
        }
        spdlog::info("Using loopback input device: {}", inputDevice);
    }

    // Fallback to default devices if no loopback found
    if (inputDevice < 0) {
        spdlog::warn("No loopback input device found, using default");
        inputDevice = Pa_GetDefaultInputDevice();
    }
    if (outputDevice < 0) {
        spdlog::warn("No loopback output device found, using default");
        outputDevice = Pa_GetDefaultOutputDevice();
    }

    spdlog::info("Using devices: input={}, output={}", inputDevice, outputDevice);

    // Open PortAudio stream (full-duplex)
    PaStreamParameters inputParams;
    PaStreamParameters outputParams;

    std::memset(&inputParams, 0, sizeof(inputParams));
    std::memset(&outputParams, 0, sizeof(outputParams));

    inputParams.device = inputDevice;
    inputParams.channelCount = channels;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    outputParams.device = outputDevice;
    outputParams.channelCount = channels;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    // Open stream
    PaError err = Pa_OpenStream(
        reinterpret_cast<PaStream**>(&paStream_),
        &inputParams,
        &outputParams,
        sampleRate,
        config_.framesPerBuffer,
        paClipOff,
        loopbackCallback,
        &callbackData
    );

    if (err != paNoError) {
        lastError_ = std::string("Failed to open stream: ") + Pa_GetErrorText(err);
        spdlog::error(lastError_);
        return false;
    }

    // Start stream
    err = Pa_StartStream(*reinterpret_cast<PaStream**>(&paStream_));
    if (err != paNoError) {
        lastError_ = std::string("Failed to start stream: ") + Pa_GetErrorText(err);
        spdlog::error(lastError_);
        closeStream();
        return false;
    }

    active_ = true;
    spdlog::info("Loopback test started");

    // Wait for completion
    while (Pa_IsStreamActive(*reinterpret_cast<PaStream**>(&paStream_))) {
        Pa_Sleep(100);
    }

    // Stop stream
    err = Pa_StopStream(*reinterpret_cast<PaStream**>(&paStream_));
    if (err != paNoError) {
        spdlog::warn("Error stopping stream: {}", Pa_GetErrorText(err));
    }

    active_ = false;
    closeStream();

    spdlog::info("Loopback test complete: captured {} frames",
                 callbackData.capturePosition.load());

    // Trim captured audio to actual captured size
    capturedAudio.resize(callbackData.capturePosition.load() * channels);

    return true;
}

bool AudioIOHandler::playAudio(const std::vector<float>& audio,
                               int sampleRate,
                               int channels,
                               const std::string& deviceName) {
    if (!initialized_) {
        lastError_ = "AudioIOHandler not initialized";
        spdlog::error(lastError_);
        return false;
    }

    if (active_) {
        lastError_ = "Audio I/O already active";
        spdlog::error(lastError_);
        return false;
    }

    spdlog::info("Playing audio: {} frames, {} ch, {} Hz",
                 audio.size() / channels, channels, sampleRate);

    // Setup callback data
    AudioCallbackData callbackData;
    callbackData.playbackBuffer = const_cast<std::vector<float>*>(&audio);
    callbackData.totalFrames = audio.size() / channels;
    callbackData.channels = channels;
    callbackData.playbackPosition = 0;

    // Open stream (output only)
    PaStreamParameters outputParams;
    std::memset(&outputParams, 0, sizeof(outputParams));

    if (deviceName.empty()) {
        outputParams.device = Pa_GetDefaultOutputDevice();
    } else {
        int deviceIdx = findDeviceByName(deviceName, true);
        if (deviceIdx < 0) {
            spdlog::warn("Device '{}' not found, using default", deviceName);
            outputParams.device = Pa_GetDefaultOutputDevice();
        } else {
            outputParams.device = deviceIdx;
        }
    }

    outputParams.channelCount = channels;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    // Open stream
    PaError err = Pa_OpenStream(
        reinterpret_cast<PaStream**>(&paStream_),
        nullptr,
        &outputParams,
        sampleRate,
        config_.framesPerBuffer,
        paClipOff,
        playbackCallback,
        &callbackData
    );

    if (err != paNoError) {
        lastError_ = std::string("Failed to open stream: ") + Pa_GetErrorText(err);
        spdlog::error(lastError_);
        return false;
    }

    // Start stream
    err = Pa_StartStream(*reinterpret_cast<PaStream**>(&paStream_));
    if (err != paNoError) {
        lastError_ = std::string("Failed to start stream: ") + Pa_GetErrorText(err);
        spdlog::error(lastError_);
        closeStream();
        return false;
    }

    active_ = true;
    spdlog::info("Playback started");

    // Wait for completion
    while (Pa_IsStreamActive(*reinterpret_cast<PaStream**>(&paStream_))) {
        Pa_Sleep(100);
    }

    // Stop stream
    err = Pa_StopStream(*reinterpret_cast<PaStream**>(&paStream_));
    if (err != paNoError) {
        spdlog::warn("Error stopping stream: {}", Pa_GetErrorText(err));
    }

    active_ = false;
    closeStream();

    spdlog::info("Playback complete");
    return true;
}

bool AudioIOHandler::captureAudio(std::vector<float>& audio,
                                  int sampleRate,
                                  int channels,
                                  float durationSeconds,
                                  const std::string& deviceName) {
    if (!initialized_) {
        lastError_ = "AudioIOHandler not initialized";
        spdlog::error(lastError_);
        return false;
    }

    if (active_) {
        lastError_ = "Audio I/O already active";
        spdlog::error(lastError_);
        return false;
    }

    size_t totalFrames = static_cast<size_t>(durationSeconds * sampleRate);
    audio.resize(totalFrames * channels);

    spdlog::info("Capturing audio: {} frames, {} ch, {} Hz, {:.2f}s",
                 totalFrames, channels, sampleRate, durationSeconds);

    // Setup callback data
    AudioCallbackData callbackData;
    callbackData.captureBuffer = &audio;
    callbackData.channels = channels;
    callbackData.capturePosition = 0;

    // Open stream (input only)
    PaStreamParameters inputParams;
    std::memset(&inputParams, 0, sizeof(inputParams));

    if (deviceName.empty()) {
        inputParams.device = Pa_GetDefaultInputDevice();
    } else {
        int deviceIdx = findDeviceByName(deviceName, false);
        if (deviceIdx < 0) {
            spdlog::warn("Device '{}' not found, using default", deviceName);
            inputParams.device = Pa_GetDefaultInputDevice();
        } else {
            inputParams.device = deviceIdx;
        }
    }

    inputParams.channelCount = channels;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    // Open stream
    PaError err = Pa_OpenStream(
        reinterpret_cast<PaStream**>(&paStream_),
        &inputParams,
        nullptr,
        sampleRate,
        config_.framesPerBuffer,
        paClipOff,
        captureCallback,
        &callbackData
    );

    if (err != paNoError) {
        lastError_ = std::string("Failed to open stream: ") + Pa_GetErrorText(err);
        spdlog::error(lastError_);
        return false;
    }

    // Start stream
    err = Pa_StartStream(*reinterpret_cast<PaStream**>(&paStream_));
    if (err != paNoError) {
        lastError_ = std::string("Failed to start stream: ") + Pa_GetErrorText(err);
        spdlog::error(lastError_);
        closeStream();
        return false;
    }

    active_ = true;
    spdlog::info("Capture started");

    // Wait for completion (or timeout)
    Pa_Sleep(static_cast<long>(durationSeconds * 1000) + 500);

    // Stop stream
    err = Pa_StopStream(*reinterpret_cast<PaStream**>(&paStream_));
    if (err != paNoError) {
        spdlog::warn("Error stopping stream: {}", Pa_GetErrorText(err));
    }

    active_ = false;
    closeStream();

    // Trim to actual captured size
    audio.resize(callbackData.capturePosition.load() * channels);

    spdlog::info("Capture complete: {} frames", callbackData.capturePosition.load());
    return true;
}

void AudioIOHandler::stop() {
    if (active_) {
        Pa_AbortStream(*reinterpret_cast<PaStream**>(&paStream_));
        active_ = false;
    }

    closeStream();

    if (initialized_) {
        Pa_Terminate();
        initialized_ = false;
        spdlog::info("PortAudio terminated");
    }
}

bool AudioIOHandler::saveToWav(const std::string& filepath,
                                const std::vector<float>& audio,
                                int sampleRate,
                                int channels) {
#ifdef SNDFILE_FOUND
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    sfinfo.samplerate = sampleRate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sndFile = sf_open(filepath.c_str(), SFM_WRITE, &sfinfo);
    if (!sndFile) {
        lastError_ = std::string("Failed to create WAV file: ") + sf_strerror(nullptr);
        spdlog::error(lastError_);
        return false;
    }

    sf_count_t framesWritten = sf_writef_float(sndFile, audio.data(),
                                                 audio.size() / channels);

    sf_close(sndFile);

    spdlog::info("Saved WAV file: {} frames written to {}", framesWritten, filepath);
    return true;
#else
    lastError_ = "libsndfile not available";
    spdlog::error(lastError_);
    return false;
#endif
}

int AudioIOHandler::findDeviceByName(const std::string& deviceName, bool forOutput) {
    int deviceCount = Pa_GetDeviceCount();

    for (int i = 0; i < deviceCount; ++i) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);

        if (forOutput && deviceInfo->maxOutputChannels > 0) {
            std::string name = deviceInfo->name;
            if (name.find(deviceName) != std::string::npos) {
                return i;
            }
        } else if (!forOutput && deviceInfo->maxInputChannels > 0) {
            std::string name = deviceInfo->name;
            if (name.find(deviceName) != std::string::npos) {
                return i;
            }
        }
    }

    return -1;
}

void AudioIOHandler::closeStream() {
    if (paStream_) {
        Pa_CloseStream(*reinterpret_cast<PaStream**>(&paStream_));
        paStream_ = nullptr;
    }
}

} // namespace testing
} // namespace audioBridge
