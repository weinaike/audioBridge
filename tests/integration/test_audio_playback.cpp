/**
 * @file test_audio_playback.cpp
 * @brief Integration test for audio playback functionality
 *
 * This test verifies the complete audio playback pipeline:
 * 1. Device enumeration and output device selection
 * 2. Opening audio output stream
 * 3. Playing test tones and audio data
 * 4. Monitoring output levels
 * 5. Closing the stream
 *
 * Success Criteria:
 * - System output device can be selected
 * - Audio data is played continuously
 * - Level meters show activity during playback
 * - No underruns or buffer starvation occurs during playback
 * - Stream can be stopped and restarted cleanly
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <cmath>

#include "adapters/PortAudioOutput.h"
#include "core/Types.h"
#include "utils/Logger.h"

using namespace audiobridge;

class AudioPlaybackIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::GetInstance().Initialize();
    }

    void TearDown() override {
        Logger::GetInstance().Flush();
        Logger::GetInstance().Reset();  // Reset logger state for next test
    }

    // Helper: Get system output device
    std::optional<int> GetSystemOutputDevice() {
        PaError err = Pa_Initialize();
        if (err != paNoError) return std::nullopt;

        PaDeviceIndex defaultOutput = Pa_GetDefaultOutputDevice();
        if (defaultOutput == paNoDevice) {
            return std::nullopt;
        }

        return static_cast<int>(defaultOutput);
    }

    // Helper: Create valid audio config
    AudioStreamConfig CreateValidConfig() {
        AudioStreamConfig config;
        config.sampleRate = 48000.0;
        config.framesPerBuffer = 128;
        config.channelCount = 2;
        config.format = SampleFormat::Float32;
        return config;
    }

    // Helper: Generate test tone
    void GenerateTestTone(float* buffer, size_t frames, int channels,
                         float frequency = 440.0f, float amplitude = 0.5f) {
        const double twoPi = 2.0 * M_PI;

        for (size_t i = 0; i < frames; ++i) {
            double t = static_cast<double>(i) / 48000.0;  // Use fixed sample rate
            float sample = amplitude * std::sin(twoPi * frequency * t);

            for (int ch = 0; ch < channels; ++ch) {
                buffer[i * channels + ch] = sample;
            }
        }
    }

    // Helper: Generate silence
    void GenerateSilence(float* buffer, size_t samples) {
        std::fill(buffer, buffer + samples, 0.0f);
    }

    AudioStreamConfig config;  // Will be initialized in test
};

// =============================================================================
// Integration Test: Basic Audio Playback
// =============================================================================

TEST_F(AudioPlaybackIntegrationTest, PlaybackTestTone_Success) {
    auto deviceId = GetSystemOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    config = CreateValidConfig();

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceId.value());
    std::cout << "Testing with device: " << deviceInfo->name << std::endl;

    PortAudioOutput output;

    // Open the device
    ASSERT_TRUE(output.Open(deviceId.value(), config))
        << "Failed to open audio device";

    // Start playback
    ASSERT_TRUE(output.Start())
        << "Failed to start audio playback";

    // Play test tone for 2 seconds
    std::cout << "Playing 440Hz test tone for 2 seconds..." << std::endl;
    constexpr auto kPlaybackDuration = std::chrono::seconds(2);
    const auto startTime = std::chrono::steady_clock::now();

    std::vector<float> playbackBuffer(config.framesPerBuffer * config.channelCount);
    size_t totalFrames = 0;
    size_t totalWrites = 0;
    float maxPeakL = 0.0f;
    float maxPeakR = 0.0f;

    // Generate test tone once
    GenerateTestTone(playbackBuffer.data(), config.framesPerBuffer, config.channelCount, 440.0f, 0.6f);

    while ((std::chrono::steady_clock::now() - startTime) < kPlaybackDuration) {
        AudioBuffer buffer;
        buffer.data = playbackBuffer.data();
        buffer.frameCount = config.framesPerBuffer;
        buffer.channelCount = config.channelCount;

        size_t framesWritten = output.Write(buffer);
        totalFrames += framesWritten;
        totalWrites++;

        if (framesWritten > 0) {
            // Calculate expected peaks (should be close to our test tone amplitude)
            for (size_t i = 0; i < framesWritten * config.channelCount; i += config.channelCount) {
                if (config.channelCount >= 1) {
                    maxPeakL = std::max(maxPeakL, std::abs(playbackBuffer[i]));
                }
                if (config.channelCount >= 2) {
                    maxPeakR = std::max(maxPeakR, std::abs(playbackBuffer[i + 1]));
                }
            }
        }

        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Stop playback
    output.Stop();
    output.Close();

    // Verify results
    std::cout << "Playback completed:" << std::endl;
    std::cout << "  Total frames: " << totalFrames << std::endl;
    std::cout << "  Total writes: " << totalWrites << std::endl;
    std::cout << "  Max peak L: " << maxPeakL << std::endl;
    std::cout << "  Max peak R: " << maxPeakR << std::endl;
    std::cout << "  Expected frames: " << (config.sampleRate * 2) << std::endl;

    // We should have written approximately 2 seconds of audio
    // Allow 20% tolerance due to timing variations
    const size_t expectedFrames = static_cast<size_t>(config.sampleRate * 2);
    EXPECT_GT(totalFrames, expectedFrames * 0.8);
    EXPECT_LT(totalFrames, expectedFrames * 1.5);

    // We should have done multiple writes
    EXPECT_GT(totalWrites, 10u);

    // Peaks should be non-zero (we wrote audio)
    EXPECT_GT(maxPeakL, 0.0f);
}

// =============================================================================
// Integration Test: Multiple Frequency Sweep
// =============================================================================

TEST_F(AudioPlaybackIntegrationTest, PlaybackFrequencySweep_VerifyAudible) {
    auto deviceId = GetSystemOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    config = CreateValidConfig();

    std::cout << "\n=== Frequency Sweep Test ===" << std::endl;
    std::cout << "Playing frequency sweep from 200Hz to 2000Hz..." << std::endl;

    PortAudioOutput output;
    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    // Wait for stream to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<float> playbackBuffer(config.framesPerBuffer * config.channelCount);
    constexpr int kNumSteps = 20;
    constexpr float kMinFreq = 200.0f;
    constexpr float kMaxFreq = 2000.0f;

    size_t totalFrames = 0;

    for (int i = 0; i < kNumSteps; ++i) {
        float frequency = kMinFreq + (kMaxFreq - kMinFreq) * i / (kNumSteps - 1);

        // Generate tone at current frequency
        GenerateTestTone(playbackBuffer.data(), config.framesPerBuffer, config.channelCount, frequency, 0.4f);

        AudioBuffer buffer;
        buffer.data = playbackBuffer.data();
        buffer.frameCount = config.framesPerBuffer;
        buffer.channelCount = config.channelCount;

        size_t written = output.Write(buffer);
        totalFrames += written;

        std::cout << "\r[" << std::string(30 * i / kNumSteps, '=') << std::string(30 - 30 * i / kNumSteps, ' ')
                  << "] " << static_cast<int>(frequency) << "Hz" << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "\n\nFrequency sweep completed:" << std::endl;
    std::cout << "  Total frames: " << totalFrames << std::endl;

    EXPECT_GT(totalFrames, 0u);

    output.Stop();
    output.Close();
}

// =============================================================================
// Integration Test: Continuous Playback
// =============================================================================

TEST_F(AudioPlaybackIntegrationTest, ExtendedPlayback_NoUnderruns) {
    auto deviceId = GetSystemOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    config = CreateValidConfig();

    std::cout << "\n=== Extended Playback Test (5 seconds) ===" << std::endl;

    PortAudioOutput output;
    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    // Wait for stream to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    constexpr auto kPlaybackDuration = std::chrono::seconds(5);
    const auto startTime = std::chrono::steady_clock::now();

    std::vector<float> playbackBuffer(config.framesPerBuffer * config.channelCount);
    size_t totalFrames = 0;
    size_t totalWrites = 0;
    size_t emptyWrites = 0;
    size_t partialWrites = 0;

    // Generate test tone
    GenerateTestTone(playbackBuffer.data(), config.framesPerBuffer, config.channelCount, 880.0f, 0.5f);

    while ((std::chrono::steady_clock::now() - startTime) < kPlaybackDuration) {
        AudioBuffer buffer;
        buffer.data = playbackBuffer.data();
        buffer.frameCount = config.framesPerBuffer;
        buffer.channelCount = config.channelCount;

        size_t framesWritten = output.Write(buffer);
        totalFrames += framesWritten;
        totalWrites++;

        if (framesWritten == 0) {
            emptyWrites++;
        } else if (static_cast<int>(framesWritten) < config.framesPerBuffer) {
            partialWrites++;
        }

        // Progress indicator
        if (totalWrites % 50 == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
            std::cout << "\r[" << elapsed << "s / 5s] Writes: " << totalWrites
                      << " | Empty: " << emptyWrites << "    " << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    output.Stop();
    output.Close();

    std::cout << "\n\nExtended playback completed:" << std::endl;
    std::cout << "  Total writes: " << totalWrites << std::endl;
    std::cout << "  Total frames: " << totalFrames << std::endl;
    std::cout << "  Empty writes: " << emptyWrites << " (" << (100.0 * emptyWrites / totalWrites) << "%)" << std::endl;
    std::cout << "  Partial writes: " << partialWrites << std::endl;
    std::cout << "  Average frames per write: " << (1.0 * totalFrames / totalWrites) << std::endl;

    // We should have minimal empty writes (indicates buffer underruns)
    float emptyRatio = 100.0 * emptyWrites / totalWrites;
    EXPECT_LT(emptyRatio, 10.0) << "Too many buffer underruns: " << emptyRatio << "%";
}

// =============================================================================
// Integration Test: Start/Stop Cycles
// =============================================================================

TEST_F(AudioPlaybackIntegrationTest, MultipleStartStopCycles_Success) {
    auto deviceId = GetSystemOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    config = CreateValidConfig();

    PortAudioOutput output;
    ASSERT_TRUE(output.Open(deviceId.value(), config));

    // Perform multiple start/stop cycles
    constexpr int kNumCycles = 5;

    for (int cycle = 0; cycle < kNumCycles; ++cycle) {
        std::cout << "Cycle " << (cycle + 1) << "/" << kNumCycles << std::endl;

        EXPECT_TRUE(output.Start()) << "Start failed on cycle " << cycle;

        // Wait for stream to be ready
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Play briefly
        std::vector<float> buffer(config.framesPerBuffer * config.channelCount);
        GenerateTestTone(buffer.data(), config.framesPerBuffer, config.channelCount, 440.0f + cycle * 100.0f, 0.3f);

        AudioBuffer buf;
        buf.data = buffer.data();
        buf.frameCount = config.framesPerBuffer;
        buf.channelCount = config.channelCount;

        for (int i = 0; i < 10; ++i) {
            output.Write(buf);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        output.Stop();

        // Verify state returned to Stopped
        EXPECT_EQ(output.GetState(), StreamState::Stopped);
        EXPECT_FALSE(output.IsActive());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    output.Close();

    std::cout << "Completed " << kNumCycles << " start/stop cycles successfully" << std::endl;
}

// =============================================================================
// Integration Test: Level Monitoring
// =============================================================================

TEST_F(AudioPlaybackIntegrationTest, PlaybackWithLevelMonitoring_ActivityDetected) {
    auto deviceId = GetSystemOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    config = CreateValidConfig();

    std::cout << "\n=== Level Monitoring Test ===" << std::endl;

    PortAudioOutput output;
    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    // Wait for stream to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Monitor levels during playback
    constexpr auto kMonitorDuration = std::chrono::seconds(3);
    const auto startTime = std::chrono::steady_clock::now();

    std::vector<float> playbackBuffer(config.framesPerBuffer * config.channelCount);
    GenerateTestTone(playbackBuffer.data(), config.framesPerBuffer, config.channelCount, 1000.0f, 0.7f);

    float maxPeakL = 0.0f;
    float maxPeakR = 0.0f;
    size_t framesWithActivity = 0;
    size_t totalFrames = 0;

    while ((std::chrono::steady_clock::now() - startTime) < kMonitorDuration) {
        AudioBuffer buffer;
        buffer.data = playbackBuffer.data();
        buffer.frameCount = config.framesPerBuffer;
        buffer.channelCount = config.channelCount;

        size_t framesWritten = output.Write(buffer);
        totalFrames += framesWritten;

        if (framesWritten > 0) {
            // Get current output levels
            auto [peakL, peakR] = output.GetPeakLevels();

            maxPeakL = std::max(maxPeakL, peakL);
            maxPeakR = std::max(maxPeakR, peakR);

            // Define "activity" as level above noise floor (-60dB = 0.001)
            constexpr float kNoiseFloor = 0.001f;
            if (peakL > kNoiseFloor || peakR > kNoiseFloor) {
                framesWithActivity += framesWritten;
            }

            // Print level bar graph
            std::cout << "\r[";
            int barWidthL = static_cast<int>(peakL * 50);
            int barWidthR = static_cast<int>(peakR * 50);
            for (int i = 0; i < 50; ++i) {
                std::cout << (i < barWidthL ? "L" : (i < barWidthR ? "R" : " "));
            }
            std::cout << "] L=" << peakL << " R=" << peakR << "    " << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    output.Stop();
    output.Close();

    std::cout << "\n\nLevel monitoring completed:" << std::endl;
    std::cout << "  Max peak L: " << maxPeakL << " (" << 20 * std::log10(maxPeakL + 1e-10) << " dB)" << std::endl;
    std::cout << "  Max peak R: " << maxPeakR << " (" << 20 * std::log10(maxPeakR + 1e-10) << " dB)" << std::endl;
    std::cout << "  Frames with activity: " << framesWithActivity << " / " << totalFrames << std::endl;

    // Test passes if we successfully wrote data
    // (Can't guarantee user heard it, but levels should be non-zero)
    EXPECT_GT(totalFrames, 0u);
    EXPECT_GT(maxPeakL, 0.0f);
}

// No main() needed - using gtest_main
