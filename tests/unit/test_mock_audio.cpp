/**
 * @file test_mock_audio.cpp
 * @brief Unit tests using mock audio adapters (no hardware required)
 *
 * These tests verify the audio pipeline functionality using mock adapters
 * that simulate audio input/output without requiring actual audio hardware.
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "mocks/MockAudioInput.h"
#include "mocks/MockAudioOutput.h"
#include "core/AudioPipeline.h"
#include "core/Types.h"

using namespace audiobridge;

class MockAudioTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    AudioStreamConfig CreateDefaultConfig() {
        AudioStreamConfig config;
        config.sampleRate = 48000;
        config.channelCount = 2;
        config.framesPerBuffer = 128;
        return config;
    }
};

// =============================================================================
// Mock Input Tests
// =============================================================================

TEST_F(MockAudioTest, MockInput_Open_Start_Close_Success) {
    MockAudioInput input;
    auto config = CreateDefaultConfig();

    // Open
    ASSERT_TRUE(input.Open(0, config));
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // Start
    ASSERT_TRUE(input.Start());
    EXPECT_EQ(input.GetState(), StreamState::Active);

    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop
    input.Stop();
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // Close
    input.Close();
}

TEST_F(MockAudioTest, MockInput_ReadAudioData_Success) {
    MockAudioInput input;
    auto config = CreateDefaultConfig();

    ASSERT_TRUE(input.Open(0, config));
    ASSERT_TRUE(input.Start());

    // Wait for some data to be generated
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Read data
    size_t available = input.AvailableFrames();
    EXPECT_GT(available, 0u);

    if (available > 0) {
        float buffer[256];
        AudioBuffer audioBuffer;
        audioBuffer.data = buffer;
        audioBuffer.frameCount = 128;
        audioBuffer.channelCount = 2;

        size_t read = input.Read(audioBuffer);
        EXPECT_GT(read, 0u);
    }

    input.Stop();
    input.Close();
}

TEST_F(MockAudioTest, MockInput_LevelMonitoring_Success) {
    MockAudioInput input;
    auto config = CreateDefaultConfig();

    ASSERT_TRUE(input.Open(0, config));
    ASSERT_TRUE(input.Start());

    // Wait for audio generation
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Check levels
    auto [peakL, peakR] = input.GetPeakLevels();
    EXPECT_GT(peakL, 0.0f);
    EXPECT_GT(peakR, 0.0f);
    EXPECT_LE(peakL, 1.0f);
    EXPECT_LE(peakR, 1.0f);

    input.Stop();
    input.Close();
}

// =============================================================================
// Mock Output Tests
// =============================================================================

TEST_F(MockAudioTest, MockOutput_Open_Start_Close_Success) {
    MockAudioOutput output;
    auto config = CreateDefaultConfig();

    // Open
    ASSERT_TRUE(output.Open(0, config));
    EXPECT_EQ(output.GetState(), StreamState::Stopped);

    // Start
    ASSERT_TRUE(output.Start());
    EXPECT_EQ(output.GetState(), StreamState::Active);

    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop
    output.Stop();
    EXPECT_EQ(output.GetState(), StreamState::Stopped);

    // Close
    output.Close();
}

TEST_F(MockAudioTest, MockOutput_WriteAudioData_Success) {
    MockAudioOutput output;
    auto config = CreateDefaultConfig();

    ASSERT_TRUE(output.Open(0, config));
    ASSERT_TRUE(output.Start());

    // Write test data
    float buffer[256];
    for (size_t i = 0; i < 256; ++i) {
        buffer[i] = 0.5f;  // Constant test signal
    }

    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    size_t written = output.Write(audioBuffer);
    EXPECT_GT(written, 0u);

    // Wait for playback
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_GT(output.GetTotalFramesPlayed(), 0u);

    output.Stop();
    output.Close();
}

TEST_F(MockAudioTest, MockOutput_LevelMonitoring_Success) {
    MockAudioOutput output;
    auto config = CreateDefaultConfig();

    ASSERT_TRUE(output.Open(0, config));
    ASSERT_TRUE(output.Start());

    // Write test data
    float buffer[256];
    for (size_t i = 0; i < 256; ++i) {
        buffer[i] = 0.5f;
    }

    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    output.Write(audioBuffer);

    // Wait for playback
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Check levels
    auto [peakL, peakR] = output.GetPeakLevels();
    EXPECT_GT(peakL, 0.0f);
    EXPECT_GT(peakR, 0.0f);

    output.Stop();
    output.Close();
}

// =============================================================================
// Pass-through Tests with Mocks
// =============================================================================

TEST_F(MockAudioTest, PassThrough_InputToOutput_Success) {
    MockAudioInput input;
    MockAudioOutput output;
    AudioPipeline pipeline;
    auto config = CreateDefaultConfig();

    // Open and start input
    ASSERT_TRUE(input.Open(0, config));
    ASSERT_TRUE(input.Start());

    // Open and start output
    ASSERT_TRUE(output.Open(1, config));
    ASSERT_TRUE(output.Start());

    // Connect pipeline
    pipeline.SetInput(&input);
    pipeline.SetOutput(&output);
    pipeline.SetPassThroughEnabled(true);

    // Run pass-through for 500ms
    auto startTime = std::chrono::steady_clock::now();
    size_t iterations = 0;

    while ((std::chrono::steady_clock::now() - startTime) < std::chrono::milliseconds(500)) {
        pipeline.Process();
        iterations++;
        std::this_thread::sleep_for(std::chrono::microseconds(100));  // ~10kHz processing
    }

    // Verify audio flowed through
    EXPECT_GT(output.GetTotalFramesPlayed(), 0u);

    // Cleanup
    input.Stop();
    input.Close();
    output.Stop();
    output.Close();

    std::cout << "Pass-through: Processed " << iterations << " iterations" << std::endl;
}

TEST_F(MockAudioTest, PassThrough_LatencyMeasurement_Success) {
    MockAudioInput input;
    MockAudioOutput output;
    AudioPipeline pipeline;
    auto config = CreateDefaultConfig();

    // Setup
    ASSERT_TRUE(input.Open(0, config));
    ASSERT_TRUE(input.Start());
    ASSERT_TRUE(output.Open(1, config));
    ASSERT_TRUE(output.Start());

    pipeline.SetInput(&input);
    pipeline.SetOutput(&output);
    pipeline.SetPassThroughEnabled(true);

    // Process some audio
    for (int i = 0; i < 100; ++i) {
        pipeline.Process();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Measure latency
    size_t latencyFrames = pipeline.GetLatencyFrames();
    double latencyMs = (latencyFrames * 1000.0) / config.sampleRate;

    std::cout << "Measured latency: " << latencyMs << " ms (" << latencyFrames << " frames)" << std::endl;

    // Latency should be reasonable (less than 200ms)
    EXPECT_LT(latencyMs, 200.0);

    // Cleanup
    input.Stop();
    input.Close();
    output.Stop();
    output.Close();
}

// =============================================================================
// State Management Tests
// =============================================================================

TEST_F(MockAudioTest, MockInput_StateTransitions_Success) {
    MockAudioInput input;
    auto config = CreateDefaultConfig();

    // Initial state
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // Open
    ASSERT_TRUE(input.Open(0, config));
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // Start
    ASSERT_TRUE(input.Start());
    EXPECT_EQ(input.GetState(), StreamState::Active);

    // Stop
    input.Stop();
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // Close
    input.Close();

    // Can re-open
    ASSERT_TRUE(input.Open(0, config));
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    input.Close();
}

TEST_F(MockAudioTest, MockOutput_StateTransitions_Success) {
    MockAudioOutput output;
    auto config = CreateDefaultConfig();

    // Initial state
    EXPECT_EQ(output.GetState(), StreamState::Stopped);

    // Open
    ASSERT_TRUE(output.Open(0, config));
    EXPECT_EQ(output.GetState(), StreamState::Stopped);

    // Start
    ASSERT_TRUE(output.Start());
    EXPECT_EQ(output.GetState(), StreamState::Active);

    // Stop
    output.Stop();
    EXPECT_EQ(output.GetState(), StreamState::Stopped);

    // Close
    output.Close();

    // Can re-open
    ASSERT_TRUE(output.Open(0, config));
    EXPECT_EQ(output.GetState(), StreamState::Stopped);

    output.Close();
}

// =============================================================================
// Error Handling Tests
// =============================================================================

TEST_F(MockAudioTest, MockInput_OpenWithInvalidDevice_Fails) {
    MockAudioInput input;
    auto config = CreateDefaultConfig();

    EXPECT_FALSE(input.Open(-1, config));  // Invalid device ID
}

TEST_F(MockAudioTest, MockInput_OpenWithInvalidConfig_Fails) {
    MockAudioInput input;

    // Invalid sample rate
    AudioStreamConfig config;
    config.sampleRate = 0;
    config.channelCount = 2;
    config.framesPerBuffer = 128;

    EXPECT_FALSE(input.Open(0, config));
}

TEST_F(MockAudioTest, MockInput_StartWithoutOpen_Fails) {
    MockAudioInput input;

    EXPECT_FALSE(input.Start());  // Cannot start without opening
}

TEST_F(MockAudioTest, MockOutput_WriteBeforeStart_ReturnsZero) {
    MockAudioOutput output;
    auto config = CreateDefaultConfig();

    ASSERT_TRUE(output.Open(0, config));

    float buffer[128];
    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 64;
    audioBuffer.channelCount = 2;

    // Should return 0 if not started
    size_t written = output.Write(audioBuffer);
    EXPECT_EQ(written, 0u);

    output.Close();
}

// No main() needed - using gtest_main
