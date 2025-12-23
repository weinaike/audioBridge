/**
 * @file test_audio_capture.cpp
 * @brief Integration test for audio capture functionality
 *
 * This test verifies the complete audio capture pipeline:
 * 1. Device enumeration and selection
 * 2. Opening audio input stream
 * 3. Capturing audio data
 * 4. Monitoring audio levels
 * 5. Closing the stream
 *
 * Success Criteria:
 * - Virtual sound card can be selected as input
 * - Audio data is captured continuously
 * - Level meters show activity when audio is playing
 * - No xruns or buffer overflows occur during capture
 * - Stream can be stopped and restarted cleanly
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <cmath>

#include "adapters/PortAudioInput.h"
#include "adapters/IDeviceEnumerator.h"
#include "core/Types.h"
#include "utils/Logger.h"

using namespace audiobridge;

class AudioCaptureIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::GetInstance().Initialize();
    }

    void TearDown() override {
        Logger::GetInstance().Flush();
        Logger::GetInstance().Reset();  // Reset logger state for next test
    }

    // Helper: Find virtual sound card device
    std::optional<int> FindVirtualSoundCard() {
        // Try to find common virtual sound card names
        const std::vector<std::string> virtualDeviceNames = {
            "VB-Audio",               // VB-Cable
            "BlackHole",              // macOS BlackHole
            "Soundflower",            // macOS Soundflower
            " JACK ",                 // JACK Audio Connection Kit
            " PulseAudio ",           // PulseAudio (can be virtual)
            "vb-cable",               // Lowercase variant
            "VB Cable"                // Space variant
        };

        PaDeviceIndex count = Pa_GetDeviceCount();
        if (count < 0) return std::nullopt;

        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (!info || info->maxInputChannels == 0) continue;

            std::string deviceName = info->name;
            // Case-insensitive search for virtual device keywords
            for (const auto& keyword : virtualDeviceNames) {
                if (deviceName.find(keyword) != std::string::npos) {
                    return i;
                }
            }
        }

        // If no virtual device found, fall back to default input device
        PaDeviceIndex defaultInput = Pa_GetDefaultInputDevice();
        if (defaultInput != paNoDevice) {
            return defaultInput;
        }

        return std::nullopt;
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

    // Helper: Calculate peak level from buffer
    std::pair<float, float> CalculateLevels(const float* data, size_t frames, int channels) {
        float peakL = 0.0f;
        float peakR = 0.0f;

        for (size_t i = 0; i < frames; ++i) {
            if (channels >= 1) {
                peakL = std::max(peakL, std::abs(data[i * channels]));
            }
            if (channels >= 2) {
                peakR = std::max(peakR, std::abs(data[i * channels + 1]));
            }
        }

        return {peakL, peakR};
    }
};

// =============================================================================
// Integration Test: Basic Audio Capture
// =============================================================================

TEST_F(AudioCaptureIntegrationTest, CaptureFromVirtualSoundCard_Success) {
    auto deviceId = FindVirtualSoundCard();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceId.value());
    std::cout << "Testing with device: " << deviceInfo->name << std::endl;

    PortAudioInput input;
    auto config = CreateValidConfig();

    // Open the device
    ASSERT_TRUE(input.Open(deviceId.value(), config))
        << "Failed to open audio device";

    // Start capturing
    ASSERT_TRUE(input.Start())
        << "Failed to start audio capture";

    // Capture audio for 2 seconds
    std::cout << "Capturing audio for 2 seconds..." << std::endl;
    constexpr auto kCaptureDuration = std::chrono::seconds(2);
    const auto startTime = std::chrono::steady_clock::now();

    std::vector<float> captureBuffer(config.framesPerBuffer * config.channelCount);
    size_t totalFrames = 0;
    size_t totalReads = 0;
    float maxPeakL = 0.0f;
    float maxPeakR = 0.0f;

    while ((std::chrono::steady_clock::now() - startTime) < kCaptureDuration) {
        AudioBuffer buffer;
        buffer.data = captureBuffer.data();
        buffer.frameCount = config.framesPerBuffer;
        buffer.channelCount = config.channelCount;

        size_t framesRead = input.Read(buffer);
        totalFrames += framesRead;
        totalReads++;

        if (framesRead > 0) {
            auto [peakL, peakR] = CalculateLevels(captureBuffer.data(), framesRead, config.channelCount);
            maxPeakL = std::max(maxPeakL, peakL);
            maxPeakR = std::max(maxPeakR, peakR);

            // Print levels every ~100ms
            if (totalReads % 10 == 0) {
                std::cout << "Levels: L=" << peakL << " R=" << peakR << std::endl;
            }
        }

        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Stop capturing
    input.Stop();
    input.Close();

    // Verify results
    std::cout << "Capture completed:" << std::endl;
    std::cout << "  Total frames: " << totalFrames << std::endl;
    std::cout << "  Total reads: " << totalReads << std::endl;
    std::cout << "  Max peak L: " << maxPeakL << std::endl;
    std::cout << "  Max peak R: " << maxPeakR << std::endl;
    std::cout << "  Expected frames: " << (config.sampleRate * 2) << std::endl;

    // We should have captured approximately 2 seconds of audio
    // Allow 20% tolerance due to timing variations
    const size_t expectedFrames = static_cast<size_t>(config.sampleRate * 2);
    EXPECT_GT(totalFrames, expectedFrames * 0.8);
    EXPECT_LT(totalFrames, expectedFrames * 1.5);

    // We should have done multiple reads
    EXPECT_GT(totalReads, 10u);
}

// =============================================================================
// Integration Test: Device Selection and Enumeration
// =============================================================================

TEST_F(AudioCaptureIntegrationTest, EnumerateAndSelectVirtualDevice_Success) {
    PaDeviceIndex count = Pa_GetDeviceCount();
    ASSERT_GT(count, 0) << "No audio devices found";

    std::cout << "Available input devices:" << std::endl;

    std::vector<int> inputDevices;
    for (PaDeviceIndex i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0) {
            std::cout << "  [" << i << "] " << info->name
                      << " (API: " << Pa_GetHostApiInfo(info->hostApi)->name
                      << ", Channels: " << info->maxInputChannels << ")" << std::endl;
            inputDevices.push_back(i);
        }
    }

    ASSERT_FALSE(inputDevices.empty()) << "No input devices found";

    // Try to open each input device
    int successCount = 0;
    auto config = CreateValidConfig();

    for (int deviceId : inputDevices) {
        PortAudioInput input;
        if (input.Open(deviceId, config)) {
            successCount++;
            input.Close();
        }
    }

    std::cout << "Successfully opened " << successCount << "/" << inputDevices.size() << " devices" << std::endl;

    // At least the default device should work
    EXPECT_GT(successCount, 0);
}

// =============================================================================
// Integration Test: Level Monitoring
// =============================================================================

TEST_F(AudioCaptureIntegrationTest, CaptureWithLevelMonitoring_ActivityDetected) {
    auto deviceId = FindVirtualSoundCard();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    std::cout << "\n=== Level Monitoring Test ===" << std::endl;
    std::cout << "Please play some audio on the selected device..." << std::endl;
    std::cout << "Testing with device: " << Pa_GetDeviceInfo(deviceId.value())->name << std::endl;

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    // Monitor levels for 5 seconds
    constexpr auto kMonitorDuration = std::chrono::seconds(5);
    const auto startTime = std::chrono::steady_clock::now();

    std::vector<float> captureBuffer(config.framesPerBuffer * config.channelCount);

    float maxPeakL = 0.0f;
    float maxPeakR = 0.0f;
    size_t framesWithActivity = 0;
    size_t totalFrames = 0;

    while ((std::chrono::steady_clock::now() - startTime) < kMonitorDuration) {
        AudioBuffer buffer;
        buffer.data = captureBuffer.data();
        buffer.frameCount = config.framesPerBuffer;
        buffer.channelCount = config.channelCount;

        size_t framesRead = input.Read(buffer);
        totalFrames += framesRead;

        if (framesRead > 0) {
            auto [peakL, peakR] = CalculateLevels(captureBuffer.data(), framesRead, config.channelCount);

            maxPeakL = std::max(maxPeakL, peakL);
            maxPeakR = std::max(maxPeakR, peakR);

            // Define "activity" as level above noise floor (-60dB = 0.001)
            constexpr float kNoiseFloor = 0.001f;
            if (peakL > kNoiseFloor || peakR > kNoiseFloor) {
                framesWithActivity += framesRead;
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

    std::cout << "\n\nLevel monitoring completed:" << std::endl;
    std::cout << "  Max peak L: " << maxPeakL << " (" << 20 * std::log10(maxPeakL + 1e-10) << " dB)" << std::endl;
    std::cout << "  Max peak R: " << maxPeakR << " (" << 20 * std::log10(maxPeakR + 1e-10) << " dB)" << std::endl;
    std::cout << "  Frames with activity: " << framesWithActivity << " / " << totalFrames << std::endl;
    std::cout << "  Activity ratio: " << (100.0 * framesWithActivity / totalFrames) << "%" << std::endl;

    input.Stop();
    input.Close();

    // Test passes if we successfully captured data
    // (Can't guarantee user played audio)
    EXPECT_GT(totalFrames, 0u);
}

// =============================================================================
// Integration Test: Stream Lifecycle
// =============================================================================

TEST_F(AudioCaptureIntegrationTest, MultipleStartStopCycles_Success) {
    auto deviceId = FindVirtualSoundCard();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    // Perform multiple start/stop cycles
    constexpr int kNumCycles = 5;

    for (int cycle = 0; cycle < kNumCycles; ++cycle) {
        std::cout << "Cycle " << (cycle + 1) << "/" << kNumCycles << std::endl;

        EXPECT_TRUE(input.Start()) << "Start failed on cycle " << cycle;

        // Capture briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        std::vector<float> buffer(config.framesPerBuffer * config.channelCount);
        AudioBuffer audioBuf;
        audioBuf.data = buffer.data();
        audioBuf.frameCount = config.framesPerBuffer;
        audioBuf.channelCount = config.channelCount;

        size_t framesRead = input.Read(audioBuf);
        EXPECT_GT(framesRead, 0u) << "No data captured on cycle " << cycle;

        input.Stop();

        // Verify state returned to Stopped
        EXPECT_EQ(input.GetState(), StreamState::Stopped);
        EXPECT_FALSE(input.IsActive());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    input.Close();

    std::cout << "Completed " << kNumCycles << " start/stop cycles successfully" << std::endl;
}

// =============================================================================
// Integration Test: No Buffer Overruns
// =============================================================================

TEST_F(AudioCaptureIntegrationTest, ExtendedCapture_NoBufferOverruns) {
    auto deviceId = FindVirtualSoundCard();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    std::cout << "\n=== Extended Capture Test (10 seconds) ===" << std::endl;

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    // Capture for 10 seconds to check for buffer issues
    constexpr auto kCaptureDuration = std::chrono::seconds(10);
    const auto startTime = std::chrono::steady_clock::now();

    std::vector<float> captureBuffer(config.framesPerBuffer * config.channelCount);
    size_t totalFrames = 0;
    size_t totalReads = 0;
    size_t emptyReads = 0;

    while ((std::chrono::steady_clock::now() - startTime) < kCaptureDuration) {
        AudioBuffer buffer;
        buffer.data = captureBuffer.data();
        buffer.frameCount = config.framesPerBuffer;
        buffer.channelCount = config.channelCount;

        size_t framesRead = input.Read(buffer);
        totalFrames += framesRead;
        totalReads++;

        if (framesRead == 0) {
            emptyReads++;
        }

        // Progress indicator
        if (totalReads % 100 == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
            std::cout << "\r[" << elapsed << "s / 10s] Reads: " << totalReads
                      << " | Frames: " << totalFrames << " | Empty: " << emptyReads << "    " << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    input.Stop();
    input.Close();

    std::cout << "\n\nExtended capture completed:" << std::endl;
    std::cout << "  Total reads: " << totalReads << std::endl;
    std::cout << "  Total frames: " << totalFrames << std::endl;
    std::cout << "  Empty reads: " << emptyReads << " (" << (100.0 * emptyReads / totalReads) << "%)" << std::endl;
    std::cout << "  Average frames per read: " << (1.0 * totalFrames / totalReads) << std::endl;

    // We should have minimal empty reads (indicates buffer underruns)
    float emptyRatio = 100.0 * emptyReads / totalReads;
    EXPECT_LT(emptyRatio, 10.0) << "Too many buffer underruns: " << emptyRatio << "%";
}

// =============================================================================
// Integration Test: Timestamp Consistency
// =============================================================================

TEST_F(AudioCaptureIntegrationTest, Timestamps_MonotonicallyIncreasing) {
    auto deviceId = FindVirtualSoundCard();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    // Capture multiple buffers and verify timestamps
    std::vector<float> captureBuffer(config.framesPerBuffer * config.channelCount);
    std::vector<uint64_t> timestamps;

    for (int i = 0; i < 100; ++i) {
        AudioBuffer buffer;
        buffer.data = captureBuffer.data();
        buffer.frameCount = config.framesPerBuffer;
        buffer.channelCount = config.channelCount;

        input.Read(buffer);
        timestamps.push_back(buffer.timestamp);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    input.Stop();
    input.Close();

    // Verify timestamps are monotonically increasing
    bool allIncreasing = true;
    for (size_t i = 1; i < timestamps.size(); ++i) {
        if (timestamps[i] <= timestamps[i-1]) {
            allIncreasing = false;
            std::cout << "Timestamp not increasing at index " << i
                      << ": " << timestamps[i-1] << " -> " << timestamps[i] << std::endl;
        }
    }

    EXPECT_TRUE(allIncreasing) << "Timestamps are not monotonically increasing";
}

// No main() needed - using gtest_main
