/**
 * @file PortAudioOutput.h
 * @brief PortAudio implementation of IAudioOutput interface
 *
 * This adapter provides real-time audio playback using PortAudio library.
 * It uses a lock-free RingBuffer for RT-safe audio data transfer.
 *
 * Key Features:
 * - RT-safe audio callback (no blocking, no malloc)
 * - Lock-free SPSC RingBuffer for data transfer
 * - State management with callback notifications
 * - Peak level calculation for monitoring
 * - Thread-safe operations (const methods are RT-safe)
 *
 * @author AudioBridge Team
 * @date 2025-12-23
 */

#pragma once

#include "adapters/IAudioOutput.h"
#include "core/RingBuffer.h"
#include "core/Types.h"
#include <portaudio.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <functional>

namespace audiobridge {

/**
 * @class PortAudioOutput
 * @brief PortAudio-based audio output playback adapter
 *
 * Implements real-time audio playback using PortAudio streams.
 * Audio data is transferred from the write methods to the audio callback
 * via a lock-free SPSC ring buffer.
 *
 * State Management:
 * - Stopped: Initial state, stream not open or closed
 * - Starting: Transition state during Open()
 * - Active: Stream is running and playing audio
 * - Stopping: Transition state during Stop()
 * - Error: Error occurred, stream must be closed
 *
 * Thread Safety:
 * - Audio callback: RT-safe, no blocking operations
 * - Public API: Thread-safe via mutexes where needed
 * - const methods: RT-safe (AvailableSpace, Write, GetState, IsActive)
 */
class PortAudioOutput : public IAudioOutput {
public:
    /**
     * @brief Constructor
     *
     * Initializes PortAudio if not already initialized.
     * Stream is in Stopped state after construction.
     */
    PortAudioOutput();

    /**
     * @brief Destructor
     *
     * Automatically closes stream if open.
     * Does NOT terminate PortAudio (may be used by other components).
     */
    ~PortAudioOutput() override;

    // Disable copy and move (PortAudio stream is not copyable)
    PortAudioOutput(const PortAudioOutput&) = delete;
    PortAudioOutput& operator=(const PortAudioOutput&) = delete;
    PortAudioOutput(PortAudioOutput&&) = delete;
    PortAudioOutput& operator=(PortAudioOutput&&) = delete;

    //--- Lifecycle Management ---

    /**
     * @brief Open audio output stream
     *
     * Opens PortAudio stream with specified device and configuration.
     * Transitions state from Stopped to Stopped (ready to start).
     *
     * @param deviceId PortAudio device index
     * @param config Audio stream configuration
     * @return true if stream opened successfully
     *
     * @pre GetState() == StreamState::Stopped
     * @post GetState() == StreamState::Stopped (ready)
     *
     * Thread safety: NOT thread-safe, call from one thread
     */
    bool Open(int deviceId, const AudioStreamConfig& config) override;

    /**
     * @brief Start audio playback
     *
     * Starts the PortAudio stream and begins playing audio.
     * Transitions state from Stopped to Active.
     *
     * @return true if stream started successfully
     *
     * @pre Open() must have been called successfully
     * @post GetState() == StreamState::Active
     *
     * Thread safety: NOT thread-safe, call from one thread
     */
    bool Start() override;

    /**
     * @brief Stop audio playback
     *
     * Stops the PortAudio stream. Audio callback will cease.
     * Transitions state from Active to Stopped.
     *
     * @pre Stream must be Active
     * @post GetState() == StreamState::Stopped
     *
     * Thread safety: Thread-safe (can call from any thread)
     */
    void Stop() override;

    /**
     * @brief Close audio output stream
     *
     * Closes the PortAudio stream and releases resources.
     * Transitions state from any state to Stopped.
     *
     * @post GetState() == StreamState::Stopped
     *
     * Thread safety: Thread-safe (can call from any thread)
     */
    void Close() override;

    //--- State Query ---

    /**
     * @brief Get current stream state
     *
     * @return Current StreamState
     *
     * Thread safety: RT-safe, lock-free
     */
    StreamState GetState() const override;

    /**
     * @brief Check if stream is actively playing
     *
     * @return true if stream is in Active state
     *
     * Thread safety: RT-safe, lock-free
     */
    bool IsActive() const override;

    /**
     * @brief Get device information
     *
     * @return AudioDeviceInfo for currently open device
     *
     * Thread safety: Safe, reads cached device info
     */
    AudioDeviceInfo GetDeviceInfo() const override;

    //--- Data Access (RT-safe) ---

    /**
     * @brief Get number of frames that can be written without blocking
     *
     * Returns the number of frames that can be written without blocking.
     *
     * @return Number of frames available for writing
     *
     * Thread safety: RT-safe, lock-free
     */
    size_t AvailableSpace() const override;

    /**
     * @brief Write audio data to ring buffer
     *
     * Writes up to buffer.frameCount frames to the internal ring buffer.
     * Actual frames written may be less if not enough space available.
     *
     * @param buffer AudioBuffer containing audio data to write
     * @return Actual number of frames written
     *
     * @pre buffer.data != nullptr
     * @pre buffer.frameCount > 0
     *
     * Thread safety: RT-safe, lock-free
     */
    size_t Write(const AudioBuffer& buffer) override;

    //--- Callback Registration ---

    /**
     * @brief Set state change callback
     *
     * Registers a callback to be invoked when stream state changes.
     * Callback is invoked from the calling thread (not audio callback).
     *
     * @param callback Function to call on state changes
     *
     * Thread safety: Thread-safe, can set anytime
     */
    void SetStateCallback(StreamStateCallback callback) override;

    /**
     * @brief Get peak audio levels
     *
     * Returns the peak levels since last call.
     * Levels are reset after reading.
     *
     * @return Pair of (leftPeak, rightPeak) in range [0.0, 1.0]
     *
     * Thread safety: Thread-safe (uses atomic values)
     */
    std::pair<float, float> GetPeakLevels() const;

private:
    //--- Internal Types ---

    /**
     * @brief Internal state flags
     */
    struct StateFlags {
        std::atomic<StreamState> streamState;
        std::atomic<bool> isOpen;
        std::atomic<bool> isStarted;

        StateFlags() : streamState(StreamState::Stopped),
                       isOpen(false),
                       isStarted(false) {}
    };

    //--- Audio Callback ---

    /**
     * @brief PortAudio audio callback function
     *
     * Called by PortAudio in real-time audio thread.
     * CRITICAL: Must be RT-safe (no blocking, no malloc)
     *
     * @param inputBuffer Not used for output stream
     * @param outputBuffer Output audio data to device
     * @param frameCount Number of frames to process
     * @param timeInfo Timing information
     * @param statusFlags Stream status flags
     * @param userData Pointer to PortAudioOutput instance
     * @return paContinue or paComplete/paAbort on error
     */
    static int AudioCallback(const void* inputBuffer,
                             void* outputBuffer,
                             unsigned long frameCount,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void* userData);

    /**
     * @brief Internal callback implementation
     *
     * Reads audio data from ring buffer and writes to PortAudio output.
     * Calculates peak levels for monitoring.
     * Handles underruns by outputting silence.
     *
     * @param outputBuffer Output audio data to device
     * @param frameCount Number of frames
     * @param statusFlags Stream status flags
     */
    int ProcessAudio(float* outputBuffer,
                     unsigned long frameCount,
                     PaStreamCallbackFlags statusFlags);

    /**
     * @brief Update stream state and notify callback
     *
     * @param newState New stream state
     */
    void UpdateState(StreamState newState);

    /**
     * @brief Validate audio configuration
     *
     * @param config Configuration to validate
     * @return true if configuration is valid
     */
    bool ValidateConfig(const AudioStreamConfig& config) const;

    /**
     * @brief Calculate peak levels from audio buffer
     *
     * @param data Audio data
     * @param frames Number of frames
     * @param channels Number of channels
     * @return Pair of (leftPeak, rightPeak)
     */
    std::pair<float, float> CalculatePeaks(const float* data,
                                            unsigned long frames,
                                            int channels) const;

    //--- Member Variables ---

    PaStream* stream_;                  ///< PortAudio stream handle
    AudioStreamConfig config_;          ///< Current stream configuration
    AudioDeviceInfo deviceInfo_;        ///< Cached device information
    StateFlags state_;                  ///< Stream state flags

    // Ring buffer with fixed size (8192 = 2^13)
    static constexpr size_t kRingBufferSize = 8192;
    RingBuffer<float, kRingBufferSize>* ringBuffer_;  ///< Lock-free audio buffer

    std::atomic<float> peakLevelL_;     ///< Current peak level left
    std::atomic<float> peakLevelR_;     ///< Current peak level right

    StreamStateCallback stateCallback_; ///< State change callback
    mutable std::mutex stateCallbackMutex_;  ///< Protects callback assignment

    std::atomic<uint64_t> timestamp_;   ///< Current frame timestamp (microseconds)
};

} // namespace audiobridge
