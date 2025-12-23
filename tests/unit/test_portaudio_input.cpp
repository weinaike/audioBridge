/**
 * @file test_portaudio_input.cpp
 * @brief Unit tests for PortAudioInput adapter
 *
 * Tests follow TDD principles and verify:
 * - Lifecycle: Open(), Start(), Stop(), Close()
 * - State management: StreamState transitions
 * - RT-safe operations: AvailableFrames(), Read()
 * - Error handling: Invalid device, invalid config
 * - Level monitoring: Peak calculation
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adapters/PortAudioInput.h"
#include "core/Types.h"
#include "utils/Logger.h"
#include <thread>
#include <chrono>

using namespace audiobridge;

class PortAudioInputTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for tests
        Logger::GetInstance().Initialize();
    }

    void TearDown() override {
        Logger::GetInstance().Flush();
    }

    // Helper: Get first available input device
    std::optional<int> GetFirstInputDevice() {
        PaDeviceIndex count = Pa_GetDeviceCount();
        if (count < 0) return std::nullopt;

        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && info->maxInputChannels > 0) {
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
};

// =============================================================================
// Test: Construction and Initial State
// =============================================================================

TEST_F(PortAudioInputTest, InitialState_IsStopped) {
    PortAudioInput input;

    EXPECT_EQ(input.GetState(), StreamState::Stopped);
    EXPECT_FALSE(input.IsActive());
}

TEST_F(PortAudioInputTest, DefaultConstructor_NoDeviceOpen) {
    PortAudioInput input;

    // Before Open(), device info should be empty/invalid
    auto deviceInfo = input.GetDeviceInfo();
    EXPECT_EQ(deviceInfo.deviceId, -1);
}

// =============================================================================
// Test: Open() - Lifecycle
// =============================================================================

TEST_F(PortAudioInputTest, Open_WithValidDevice_Success) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    bool success = input.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    EXPECT_EQ(input.GetState(), StreamState::Stopped);
}

TEST_F(PortAudioInputTest, Open_WithInvalidDevice_Fails) {
    PortAudioInput input;
    auto config = CreateValidConfig();

    // Use invalid device index
    bool success = input.Open(-999, config);

    EXPECT_FALSE(success);
    EXPECT_EQ(input.GetState(), StreamState::Error);
}

TEST_F(PortAudioInputTest, Open_WithInvalidConfig_Fails) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;

    // Invalid config (wrong sample rate)
    AudioStreamConfig badConfig;
    badConfig.sampleRate = 96000.0;  // Not supported in this phase
    badConfig.framesPerBuffer = 128;
    badConfig.channelCount = 2;
    badConfig.format = SampleFormat::Float32;

    bool success = input.Open(deviceId.value(), badConfig);

    EXPECT_FALSE(success);
}

TEST_F(PortAudioInputTest, Open_Twice_Fails) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    bool firstOpen = input.Open(deviceId.value(), config);
    ASSERT_TRUE(firstOpen);

    // Second open should fail
    bool secondOpen = input.Open(deviceId.value(), config);

    EXPECT_FALSE(secondOpen);
}

// =============================================================================
// Test: Start() and Stop()
// =============================================================================

TEST_F(PortAudioInputTest, Start_BeforeOpen_Fails) {
    PortAudioInput input;

    bool success = input.Start();

    EXPECT_FALSE(success);
}

TEST_F(PortAudioInputTest, Start_AfterOpen_Success) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    bool success = input.Start();

    EXPECT_TRUE(success);
    EXPECT_TRUE(input.IsActive());

    // Cleanup
    input.Stop();
    input.Close();
}

TEST_F(PortAudioInputTest, Start_TransitionStateToActive) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    // Capture state changes via callback
    StreamState lastState = StreamState::Stopped;
    input.SetStateCallback([&lastState](StreamState newState) {
        lastState = newState;
    });

    input.Start();

    // Give callback time to fire
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(lastState, StreamState::Active);

    // Cleanup
    input.Stop();
    input.Close();
}

TEST_F(PortAudioInputTest, Stop_AfterStart_ReturnsToStopped) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    input.Stop();

    EXPECT_FALSE(input.IsActive());
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // Cleanup
    input.Close();
}

// =============================================================================
// Test: Close()
// =============================================================================

TEST_F(PortAudioInputTest, Close_AfterOpen_Success) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    input.Close();

    EXPECT_EQ(input.GetState(), StreamState::Stopped);
}

TEST_F(PortAudioInputTest, Close_WithoutOpen_IsSafe) {
    PortAudioInput input;

    // Should not crash
    input.Close();

    EXPECT_EQ(input.GetState(), StreamState::Stopped);
}

// =============================================================================
// Test: Data Access (RT-safe)
// =============================================================================

TEST_F(PortAudioInputTest, AvailableFrames_IncreasesAfterStart) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    // Wait for some audio to be captured
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    size_t available = input.AvailableFrames();

    // Should have some frames available (implementation-dependent)
    // Just verify it doesn't crash and returns a reasonable value
    EXPECT_GT(available, 0u);

    // Cleanup
    input.Stop();
    input.Close();
}

TEST_F(PortAudioInputTest, Read_BeforeStart_ReturnsZero) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    // Read before starting
    float buffer[256];
    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    size_t read = input.Read(audioBuffer);

    EXPECT_EQ(read, 0u);

    // Cleanup
    input.Close();
}

TEST_F(PortAudioInputTest, Read_AfterStart_ReturnsData) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    // Wait for audio to be captured
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Read some data
    float buffer[256];
    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    size_t read = input.Read(audioBuffer);

    // Should read some frames (may be less than requested if not enough available)
    EXPECT_GT(read, 0u);
    EXPECT_LE(read, 128u);

    // Cleanup
    input.Stop();
    input.Close();
}

TEST_F(PortAudioInputTest, Read_MultipleTimes_IncreasingTimestamp) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    // Wait for initial audio
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    float buffer1[256];
    AudioBuffer buf1;
    buf1.data = buffer1;
    buf1.frameCount = 128;
    buf1.channelCount = 2;

    float buffer2[256];
    AudioBuffer buf2;
    buf2.data = buffer2;
    buf2.frameCount = 128;
    buf2.channelCount = 2;

    size_t read1 = input.Read(buf1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    size_t read2 = input.Read(buf2);

    EXPECT_GT(read1, 0u);
    EXPECT_GT(read2, 0u);

    // Timestamps should be increasing
    EXPECT_LT(buf1.timestamp, buf2.timestamp);

    // Cleanup
    input.Stop();
    input.Close();
}

// =============================================================================
// Test: Device Info
// =============================================================================

TEST_F(PortAudioInputTest, GetDeviceInfo_AfterOpen_ReturnsValidInfo) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    auto info = input.GetDeviceInfo();

    EXPECT_EQ(info.deviceId, deviceId.value());
    EXPECT_GT(info.maxInputChannels, 0);
    EXPECT_GT(info.defaultSampleRate, 0.0);
    EXPECT_FALSE(info.name.empty());

    // Cleanup
    input.Close();
}

// =============================================================================
// Test: State Callback
// =============================================================================

TEST_F(PortAudioInputTest, StateCallback_Registered_ReceivesStateChanges) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    // Track state changes
    std::vector<StreamState> states;
    input.SetStateCallback([&states](StreamState newState) {
        states.push_back(newState);
    });

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    input.Stop();

    // Should have seen at least: Active, Stopping, Stopped
    EXPECT_THAT(states, ::testing::Contains(StreamState::Active));

    // Cleanup
    input.Close();
}

// =============================================================================
// Test: Level Monitoring
// =============================================================================

TEST_F(PortAudioInputTest, Read_AudioData_ReturnsLevels) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    // Note: PortAudioInput doesn't have direct level callback in the interface
    // Levels are typically calculated in the audio engine
    // This test verifies that audio data is being captured

    ASSERT_TRUE(input.Start());

    // Wait for audio
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Read and verify we get some data
    float buffer[256];
    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    size_t read = input.Read(audioBuffer);

    EXPECT_GT(read, 0u);

    // Verify data is not all zeros (unless device is silent)
    float peakL = 0.0f;
    float peakR = 0.0f;
    bool hasNonZero = false;
    for (size_t i = 0; i < read * 2; ++i) {
        if (std::abs(buffer[i]) > 0.001f) {
            hasNonZero = true;
            break;
        }
    }

    // Note: This may fail if device is truly silent, which is valid
    // We're just verifying the read mechanism works
    if (hasNonZero) {
        // Calculate peaks
        for (size_t i = 0; i < read; ++i) {
            peakL = std::max(peakL, std::abs(buffer[i * 2]));
            peakR = std::max(peakR, std::abs(buffer[i * 2 + 1]));
        }
        EXPECT_GT(peakL, 0.0f);
    }

    // Cleanup
    input.Stop();
    input.Close();
}

// =============================================================================
// Test: Error Handling
// =============================================================================

TEST_F(PortAudioInputTest, Open_Close_Open_SameDevice_Success) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    // First cycle
    ASSERT_TRUE(input.Open(deviceId.value(), config));
    input.Close();

    // Second cycle - should succeed
    bool success = input.Open(deviceId.value(), config);

    EXPECT_TRUE(success);

    // Cleanup
    input.Close();
}

TEST_F(PortAudioInputTest, Start_Stop_Start_Success) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    // First cycle
    ASSERT_TRUE(input.Start());
    input.Stop();

    // Second cycle - should succeed
    bool success = input.Start();

    EXPECT_TRUE(success);

    // Cleanup
    input.Stop();
    input.Close();
}

// =============================================================================
// Test: Thread Safety (Basic)
// =============================================================================

TEST_F(PortAudioInputTest, ConcurrentRead_DoesNotCrash) {
    auto deviceId = GetFirstInputDevice();
    if (!deviceId.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    // Wait for audio
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Read from multiple threads (stress test)
    constexpr int kNumThreads = 4;
    constexpr int kReadsPerThread = 10;

    std::vector<std::thread> threads;
    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([this, &input]() {
            float buffer[256];
            AudioBuffer buf;
            buf.data = buffer;
            buf.frameCount = 128;
            buf.channelCount = 2;

            for (int i = 0; i < kReadsPerThread; ++i) {
                input.Read(buf);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // If we got here without crashing, test passes

    // Cleanup
    input.Stop();
    input.Close();
}

// No main() needed - using gtest_main
