/**
 * @file test_virtual_audio.cpp
 * @brief Tests using virtual ALSA devices (no physical hardware required)
 *
 * These tests use ALSA dummy and loopback devices to test PortAudio
 * integration without requiring physical audio hardware. This enables
 * better test coverage in CI/CD environments.
 *
 * Setup:
 *   sudo ./tests/setup_virtual_audio.sh
 *
 * Teardown:
 *   sudo ./tests/teardown_virtual_audio.sh
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adapters/PortAudioInput.h"
#include "adapters/PortAudioOutput.h"
#include "core/AudioPipeline.h"
#include "core/Types.h"
#include "utils/Logger.h"
#include <thread>
#include <chrono>

using namespace audiobridge;

class VirtualAudioTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::GetInstance().Initialize();

        // Check if virtual devices are available
        if (!HasVirtualDevices()) {
            GTEST_SKIP() << "Virtual ALSA devices not available.\n"
                         << "Run: sudo ./tests/setup_virtual_audio.sh";
        }
    }

    void TearDown() override {
        Logger::GetInstance().Flush();
    }

    // Check if dummy/loopback devices are available
    bool HasVirtualDevices() {
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            return false;
        }

        PaDeviceIndex count = Pa_GetDeviceCount();
        if (count <= 0) {
            Pa_Terminate();
            return false;
        }

        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info) {
                std::string name = info->name ? info->name : "";
                // Check for Dummy or Loopback devices
                if (name.find("Dummy") != std::string::npos ||
                    name.find("Loopback") != std::string::npos) {
                    Pa_Terminate();
                    return true;
                }
            }
        }

        Pa_Terminate();
        return false;
    }

    // Find first dummy input device
    std::optional<int> GetVirtualInputDevice() {
        PaDeviceIndex count = Pa_GetDeviceCount();
        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && info->maxInputChannels > 0) {
                std::string name = info->name ? info->name : "";
                if (name.find("Dummy") != std::string::npos ||
                    name.find("Loopback") != std::string::npos) {
                    return i;
                }
            }
        }
        return std::nullopt;
    }

    // Find first dummy output device
    std::optional<int> GetVirtualOutputDevice() {
        PaDeviceIndex count = Pa_GetDeviceCount();
        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && info->maxOutputChannels > 0) {
                std::string name = info->name ? info->name : "";
                if (name.find("Dummy") != std::string::npos ||
                    name.find("Loopback") != std::string::npos) {
                    return i;
                }
            }
        }
        return std::nullopt;
    }

    // Create valid audio config
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
// Test: Virtual Device Detection
// =============================================================================

TEST_F(VirtualAudioTest, DetectVirtualDevices_Success) {
    EXPECT_TRUE(HasVirtualDevices());

    auto inputDevice = GetVirtualInputDevice();
    auto outputDevice = GetVirtualOutputDevice();

    ASSERT_TRUE(inputDevice.has_value()) << "No virtual input device found";
    ASSERT_TRUE(outputDevice.has_value()) << "No virtual output device found";

    std::cout << "Virtual devices:" << std::endl;
    std::cout << "  Input: " << inputDevice.value() << std::endl;
    std::cout << "  Output: " << outputDevice.value() << std::endl;
}

// =============================================================================
// Test: PortAudioInput with Virtual Devices
// =============================================================================

TEST_F(VirtualAudioTest, PortAudioInput_OpenVirtualDevice_Success) {
    auto deviceId = GetVirtualInputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual input device available";

    PortAudioInput input;
    auto config = CreateValidConfig();

    bool success = input.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    if (success) {
        EXPECT_EQ(input.GetState(), StreamState::Stopped);
        input.Close();
    }
}

TEST_F(VirtualAudioTest, PortAudioInput_StartVirtualDevice_Success) {
    auto deviceId = GetVirtualInputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual input device available";

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    bool started = input.Start();

    EXPECT_TRUE(started);
    if (started) {
        EXPECT_EQ(input.GetState(), StreamState::Active);
        EXPECT_TRUE(input.IsActive());

        // Let it run briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        input.Stop();
        EXPECT_EQ(input.GetState(), StreamState::Stopped);

        input.Close();
    }
}

TEST_F(VirtualAudioTest, PortAudioInput_ReadVirtualDevice_Success) {
    auto deviceId = GetVirtualInputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual input device available";

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));
    ASSERT_TRUE(input.Start());

    // Wait for some data to be available
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    size_t available = input.AvailableFrames();
    // Note: Dummy device may not produce data, so this is informational
    std::cout << "Available frames: " << available << std::endl;

    input.Stop();
    input.Close();
}

TEST_F(VirtualAudioTest, PortAudioInput_StateTransitions_VirtualDevice_Success) {
    auto deviceId = GetVirtualInputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual input device available";

    PortAudioInput input;
    auto config = CreateValidConfig();

    // Initial state
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // After Open
    ASSERT_TRUE(input.Open(deviceId.value(), config));
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // After Start
    ASSERT_TRUE(input.Start());
    EXPECT_EQ(input.GetState(), StreamState::Active);

    // After Stop
    input.Stop();
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    // After Close
    input.Close();
}

// =============================================================================
// Test: PortAudioOutput with Virtual Devices
// =============================================================================

TEST_F(VirtualAudioTest, PortAudioOutput_OpenVirtualDevice_Success) {
    auto deviceId = GetVirtualOutputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual output device available";

    PortAudioOutput output;
    auto config = CreateValidConfig();

    bool success = output.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    if (success) {
        EXPECT_EQ(output.GetState(), StreamState::Stopped);
        output.Close();
    }
}

TEST_F(VirtualAudioTest, PortAudioOutput_StartVirtualDevice_Success) {
    auto deviceId = GetVirtualOutputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual output device available";

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    bool started = output.Start();

    EXPECT_TRUE(started);
    if (started) {
        EXPECT_EQ(output.GetState(), StreamState::Active);
        EXPECT_TRUE(output.IsActive());

        // Let it run briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        output.Stop();
        EXPECT_EQ(output.GetState(), StreamState::Stopped);

        output.Close();
    }
}

TEST_F(VirtualAudioTest, PortAudioOutput_WriteVirtualDevice_Success) {
    auto deviceId = GetVirtualOutputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual output device available";

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));
    ASSERT_TRUE(output.Start());

    // Write some silent data
    float buffer[256];
    std::fill_n(buffer, 256, 0.0f);

    AudioBuffer audioBuffer;
    audioBuffer.data = buffer;
    audioBuffer.frameCount = 128;
    audioBuffer.channelCount = 2;

    size_t written = output.Write(audioBuffer);
    EXPECT_GT(written, 0u) << "Should write at least some frames";

    output.Stop();
    output.Close();
}

// =============================================================================
// Test: Audio Pipeline with Virtual Devices
// =============================================================================

TEST_F(VirtualAudioTest, AudioPipeline_VirtualDevicesPassThrough_Success) {
    auto inputId = GetVirtualInputDevice();
    auto outputId = GetVirtualOutputDevice();

    if (!inputId.has_value() || !outputId.has_value()) {
        GTEST_SKIP() << "Need both virtual input and output devices";
    }

    PortAudioInput input;
    PortAudioOutput output;
    AudioPipeline pipeline;

    auto config = CreateValidConfig();

    // Open devices
    ASSERT_TRUE(input.Open(inputId.value(), config));
    ASSERT_TRUE(output.Open(outputId.value(), config));

    // Start devices
    ASSERT_TRUE(input.Start());
    ASSERT_TRUE(output.Start());

    // Connect pipeline
    pipeline.SetInput(&input);
    pipeline.SetOutput(&output);
    pipeline.SetPassThroughEnabled(true);

    // Run pass-through for 200ms
    auto startTime = std::chrono::steady_clock::now();
    size_t iterations = 0;

    while ((std::chrono::steady_clock::now() - startTime) < std::chrono::milliseconds(200)) {
        pipeline.Process();
        iterations++;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    std::cout << "Pass-through: Processed " << iterations << " iterations" << std::endl;

    // Cleanup
    input.Stop();
    input.Close();
    output.Stop();
    output.Close();

    EXPECT_GT(iterations, 0u);
}

// =============================================================================
// Test: Error Handling with Virtual Devices
// =============================================================================

TEST_F(VirtualAudioTest, PortAudioInput_InvalidVirtualDevice_Fails) {
    PortAudioInput input;
    auto config = CreateValidConfig();

    // Try to open with invalid device ID
    bool success = input.Open(-999, config);

    EXPECT_FALSE(success);
    EXPECT_EQ(input.GetState(), StreamState::Error);
}

TEST_F(VirtualAudioTest, PortAudioOutput_InvalidVirtualDevice_Fails) {
    PortAudioOutput output;
    auto config = CreateValidConfig();

    // Try to open with invalid device ID
    bool success = output.Open(-999, config);

    EXPECT_FALSE(success);
    EXPECT_EQ(output.GetState(), StreamState::Error);
}

// =============================================================================
// Test: Device Info with Virtual Devices
// =============================================================================

TEST_F(VirtualAudioTest, PortAudioInput_GetDeviceInfo_VirtualDevice_Success) {
    auto deviceId = GetVirtualInputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual input device available";

    PortAudioInput input;
    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(deviceId.value(), config));

    AudioDeviceInfo info = input.GetDeviceInfo();

    EXPECT_EQ(info.deviceId, deviceId.value());
    EXPECT_GT(info.name.length(), 0u);
    EXPECT_GT(info.maxInputChannels, 0);

    std::cout << "Virtual input device: " << info.name << std::endl;
    std::cout << "  Channels: " << info.maxInputChannels << " in" << std::endl;

    input.Close();
}

TEST_F(VirtualAudioTest, PortAudioOutput_GetDeviceInfo_VirtualDevice_Success) {
    auto deviceId = GetVirtualOutputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual output device available";

    PortAudioOutput output;
    auto config = CreateValidConfig();

    ASSERT_TRUE(output.Open(deviceId.value(), config));

    AudioDeviceInfo info = output.GetDeviceInfo();

    EXPECT_EQ(info.deviceId, deviceId.value());
    EXPECT_GT(info.name.length(), 0u);
    EXPECT_GT(info.maxOutputChannels, 0);

    std::cout << "Virtual output device: " << info.name << std::endl;
    std::cout << "  Channels: " << info.maxOutputChannels << " out" << std::endl;

    output.Close();
}

// =============================================================================
// Test: Multiple Operations with Virtual Devices
// =============================================================================

TEST_F(VirtualAudioTest, PortAudioInput_OpenCloseOpen_VirtualDevice_Success) {
    auto deviceId = GetVirtualInputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual input device available";

    PortAudioInput input;
    auto config = CreateValidConfig();

    // First open
    ASSERT_TRUE(input.Open(deviceId.value(), config));
    input.Close();

    // Second open (should succeed)
    bool success = input.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    if (success) {
        input.Close();
    }
}

TEST_F(VirtualAudioTest, PortAudioOutput_OpenCloseOpen_VirtualDevice_Success) {
    auto deviceId = GetVirtualOutputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual output device available";

    PortAudioOutput output;
    auto config = CreateValidConfig();

    // First open
    ASSERT_TRUE(output.Open(deviceId.value(), config));
    output.Close();

    // Second open (should succeed)
    bool success = output.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    if (success) {
        output.Close();
    }
}

// No main() needed - using gtest_main
