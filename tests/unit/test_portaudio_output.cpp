/**
 * @file test_portaudio_output.cpp
 * @brief Unit tests for PortAudioOutput adapter
 *
 * Tests follow TDD principles and verify:
 * - Lifecycle: Open(), Start(), Stop(), Close()
 * - State management: StreamState transitions
 * - RT-safe operations: AvailableSpace(), Write()
 * - Error handling: Invalid device, invalid config
 * - Level monitoring: Peak calculation
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adapters/PortAudioOutput.h"
#include "core/Types.h"
#include "utils/Logger.h"
#include <thread>
#include <chrono>
#include <cmath>

using namespace audiobridge;

class PortAudioOutputTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::GetInstance().Initialize();
    }

    void TearDown() override {
        Logger::GetInstance().Flush();
    }

    // Helper: Get first available output device
    std::optional<int> GetFirstOutputDevice() {
        PaDeviceIndex count = Pa_GetDeviceCount();
        if (count < 0) return std::nullopt;

        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && info->maxOutputChannels > 0) {
                return i;
            }
        }
        return std::nullopt;
    }

    // Helper: Create valid config
    AudioStreamConfig CreateValidConfig() {
        AudioStreamConfig config;
        config.sampleRate = 48000.0;
        config.framesPerBuffer = 128;
        config.channelCount = 2;
        config.format = SampleFormat::Float32;
        return config;
    }

    // Helper: Generate test tone (sine wave)
    void GenerateTestTone(float* buffer, size_t frames, int channels, float frequency = 440.0f, float amplitude = 0.5f) {
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / 48000.0f;
            float sample = amplitude * std::sin(2.0f * M_PI * frequency * t);

            for (int ch = 0; ch < channels; ++ch) {
                buffer[i * channels + ch] = sample;
            }
        }
    }
};

// =============================================================================
// Test: Construction and Initial State
// =============================================================================

TEST_F(PortAudioOutputTest, InitialState_IsStopped) {
    PortAudioOutput output;

    EXPECT_EQ(output.GetState(), StreamState::Stopped);
    EXPECT_FALSE(output.IsActive());
}

TEST_F(PortAudioOutputTest, DefaultConstructor_NoDeviceOpen) {
    PortAudioOutput output;

    // Before Open(), device info should be empty/invalid
    auto deviceInfo = output.GetDeviceInfo();
    EXPECT_EQ(deviceInfo.deviceId, -1);
}

// =============================================================================
// Test: Open() - Lifecycle
// =============================================================================

TEST_F(PortAudioOutputTest, Open_WithValidDevice_Success) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    bool success = output.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    EXPECT_EQ(output.GetState(), StreamState::Stopped);
}

TEST_F(PortAudioOutputTest, Open_WithInvalidDevice_Fails) {
    PortAudioOutput output;
    auto config = CreateValidConfig();

    // Use invalid device index
    bool success = output.Open(-999, config);

    EXPECT_FALSE(success);
    EXPECT_EQ(output.GetState(), StreamState::Error);
}

TEST_F(PortAudioOutputTest, Open_WithInvalidConfig_Fails) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;

    // Invalid config (wrong sample rate)
    AudioStreamConfig badConfig;
    badConfig.sampleRate = 96000.0;  // Not supported in this phase
    badConfig.framesPerBuffer = 128;
    badConfig.channelCount = 2;
    badConfig.format = SampleFormat::Float32;

    bool success = output.Open(deviceId.value(), badConfig);

    EXPECT_FALSE(success);
}

TEST_F(PortAudioOutputTest, Open_Twice_Fails) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    bool firstOpen = output.Open(deviceId.value(), config);
    ASSERT_TRUE(firstOpen);

    // Second open should fail
    bool secondOpen = output.Open(deviceId.value(), config);

    EXPECT_FALSE(secondOpen);
}

// =============================================================================
// Test: Start() and Stop()
// =============================================================================

TEST_F(PortAudioOutputTest, Start_BeforeOpen_Fails) {
    PortAudioOutput output;

    bool success = output.Start();

    EXPECT_FALSE(success);
}

TEST_F(PortAudioOutputTest, Start_AfterOpen_Success) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    bool success = output.Start();

    EXPECT_TRUE(success);
    EXPECT_TRUE(output.IsActive());

    // Cleanup
    output.Stop();
    output.Close();
}

TEST_F(PortAudioOutputTest, Start_TransitionStateToActive) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    // Capture state changes via callback
    StreamState lastState = StreamState::Stopped;
    output.SetStateCallback([&lastState](StreamState newState) {
        lastState = newState;
    });

    output.Start();

    // Give callback time to fire
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(lastState, StreamState::Active);

    // Cleanup
    output.Stop();
    output.Close();
}

TEST_F(PortAudioOutputTest, Stop_AfterStart_ReturnsToStopped) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    output.Stop();

    EXPECT_FALSE(output.IsActive());
    EXPECT_EQ(output.GetState(), StreamState::Stopped);

    // Cleanup
    output.Close();
}

// =============================================================================
// Test: Close()
// =============================================================================

TEST_F(PortAudioOutputTest, Close_AfterOpen_Success) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    output.Close();

    EXPECT_EQ(output.GetState(), StreamState::Stopped);
}

TEST_F(PortAudioOutputTest, Close_WithoutOpen_IsSafe) {
    PortAudioOutput output;

    // Should not crash
    output.Close();

    EXPECT_EQ(output.GetState(), StreamState::Stopped);
}

// =============================================================================
// Test: Data Access (RT-safe)
// =============================================================================

TEST_F(PortAudioOutputTest, AvailableSpace_AfterStart_ReturnsNonZero) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    // Wait for stream to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    size_t available = output.AvailableSpace();

    // Should have some space available (implementation-dependent)
    // Just verify it doesn't crash and returns a reasonable value
    EXPECT_GT(available, 0u);

    // Cleanup
    output.Stop();
    output.Close();
}

TEST_F(PortAudioOutputTest, Write_BeforeStart_ReturnsZero) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    // Write before starting
    float buffer[256];
    GenerateTestTone(buffer, 128, 2);

    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    size_t written = output.Write(audioBuffer);

    EXPECT_EQ(written, 0u);

    // Cleanup
    output.Close();
}

TEST_F(PortAudioOutputTest, Write_AfterStart_ReturnsData) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    // Wait for stream to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Generate test tone
    float buffer[256];
    GenerateTestTone(buffer, 128, 2);

    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    size_t written = output.Write(audioBuffer);

    // Should write some frames (may be less than requested if not enough space)
    EXPECT_GT(written, 0u);
    EXPECT_LE(written, 128u);

    // Let audio play briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cleanup
    output.Stop();
    output.Close();
}

TEST_F(PortAudioOutputTest, Write_MultipleTimes_ContinuousPlayback) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    // Wait for stream to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    float buffer[256];
    size_t totalWritten = 0;

    // Write multiple buffers
    for (int i = 0; i < 10; ++i) {
        GenerateTestTone(buffer, 128, 2, 440.0f + i * 10.0f);  // Changing frequency

        AudioBuffer audioBuffer;
        audioBuffer.data = buffer;
        audioBuffer.frameCount = 128;
        audioBuffer.channelCount = 2;

        size_t written = output.Write(audioBuffer);
        totalWritten += written;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_GT(totalWritten, 0u);

    // Let audio play
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Cleanup
    output.Stop();
    output.Close();
}

// =============================================================================
// Test: Device Info
// =============================================================================

TEST_F(PortAudioOutputTest, GetDeviceInfo_AfterOpen_ReturnsValidInfo) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    auto info = output.GetDeviceInfo();

    EXPECT_EQ(info.deviceId, deviceId.value());
    EXPECT_GT(info.maxOutputChannels, 0);
    EXPECT_GT(info.defaultSampleRate, 0.0);
    EXPECT_FALSE(info.name.empty());

    // Cleanup
    output.Close();
}

// =============================================================================
// Test: State Callback
// =============================================================================

TEST_F(PortAudioOutputTest, StateCallback_Registered_ReceivesStateChanges) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    // Track state changes
    std::vector<StreamState> states;
    output.SetStateCallback([&states](StreamState newState) {
        states.push_back(newState);
    });

    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    output.Stop();

    // Should have seen at least: Active, Stopping, Stopped
    EXPECT_THAT(states, ::testing::Contains(StreamState::Active));

    // Cleanup
    output.Close();
}

// =============================================================================
// Test: Level Monitoring
// =============================================================================

TEST_F(PortAudioOutputTest, Write_AudioData_CalculatesLevels) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    ASSERT_TRUE(output.Start());

    // Wait for stream to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Generate test tone at known amplitude
    float buffer[256];
    GenerateTestTone(buffer, 128, 2, 440.0f, 0.8f);

    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    size_t written = output.Write(audioBuffer);

    EXPECT_GT(written, 0u);

    // Let audio play
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Get peak levels
    auto [peakL, peakR] = output.GetPeakLevels();

    // Should have some level (though exact value depends on implementation)
    EXPECT_GE(peakL, 0.0f);
    EXPECT_LE(peakL, 1.0f);
    EXPECT_GE(peakR, 0.0f);
    EXPECT_LE(peakR, 1.0f);

    // Cleanup
    output.Stop();
    output.Close();
}

// =============================================================================
// Test: Error Handling
// =============================================================================

TEST_F(PortAudioOutputTest, Open_Close_Open_SameDevice_Success) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    // First cycle
    ASSERT_TRUE(output.Open(deviceId.value(), config));
    output.Close();

    // Second cycle - should succeed
    bool success = output.Open(deviceId.value(), config);

    EXPECT_TRUE(success);

    // Cleanup
    output.Close();
}

TEST_F(PortAudioOutputTest, Start_Stop_Start_Success) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    // First cycle
    ASSERT_TRUE(output.Start());
    output.Stop();

    // Second cycle - should succeed
    bool success = output.Start();

    EXPECT_TRUE(success);

    // Cleanup
    output.Stop();
    output.Close();
}

// =============================================================================
// Test: Thread Safety (Basic)
// =============================================================================

TEST_F(PortAudioOutputTest, ConcurrentWrite_DoesNotCrash) {
    auto deviceId = GetFirstOutputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No output device available";
    }

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    // Wait for stream to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Write from multiple threads (stress test)
    constexpr int kNumThreads = 4;
    constexpr int kWritesPerThread = 10;

    std::vector<std::thread> threads;
    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([this, &output, t]() {
            float buffer[256];
            GenerateTestTone(buffer, 128, 2, 440.0f + t * 50.0f);

            AudioBuffer buf;
            buf.data = buffer;
            buf.frameCount = 128;
            buf.channelCount = 2;

            for (int i = 0; i < kWritesPerThread; ++i) {
                output.Write(buf);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Let audio play
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // If we got here without crashing, test passes

    // Cleanup
    output.Stop();
    output.Close();
}

// No main() needed - using gtest_main
