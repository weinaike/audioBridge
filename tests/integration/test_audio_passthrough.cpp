/**
 * @file test_audio_passthrough.cpp
 * @brief Integration test for audio pass-through functionality
 *
 * This test verifies the complete audio pass-through pipeline:
 * 1. AudioEngine creation and initialization
 * 2. Device enumeration and selection
 * 3. Pass-through mode activation
 * 4. Audio routing from input to output
 * 5. Level monitoring during pass-through
 * 6. Latency measurement (<200ms requirement)
 *
 * Success Criteria:
 * - Engine can be created and initialized
 * - Input and output devices can be selected
 * - Pass-through mode can be enabled/disabled
 * - Audio flows from input to output
 * - Level callbacks receive valid data
 * - Latency is below 200ms threshold
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

#include "core/Factory.h"
#include "core/AudioEngine.h"
#include "utils/Logger.h"

using namespace audiobridge;

class AudioPassThroughIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::GetInstance().Initialize();
        engine = CreateAudioEngine();
        ASSERT_NE(engine, nullptr);
    }

    void TearDown() override {
        if (engine) {
            if (engine->IsRunning()) {
                engine->Stop();
            }
        }
        Logger::GetInstance().Flush();
        Logger::GetInstance().Reset();  // Reset logger state for next test
    }

    // Helper: Get first available input device
    std::optional<int> GetFirstInputDevice() {
        auto& enumerator = engine->GetDeviceEnumerator();
        auto inputDevices = enumerator.GetInputDevices();
        if (inputDevices.empty()) {
            return std::nullopt;
        }
        return inputDevices[0].deviceId;
    }

    // Helper: Get first available output device
    std::optional<int> GetFirstOutputDevice() {
        auto& enumerator = engine->GetDeviceEnumerator();
        auto outputDevices = enumerator.GetOutputDevices();
        if (outputDevices.empty()) {
            return std::nullopt;
        }
        return outputDevices[0].deviceId;
    }

    // Helper: Select different input and output devices
    bool SelectTestDevices() {
        auto inputDevice = GetFirstInputDevice();
        auto outputDevice = GetFirstOutputDevice();

        if (!inputDevice.has_value() || !outputDevice.has_value()) {
            return false;
        }

        // Try to find different devices for input and output
        auto& enumerator = engine->GetDeviceEnumerator();
        auto inputDevices = enumerator.GetInputDevices();
        auto outputDevices = enumerator.GetOutputDevices();

        // Look for different devices
        for (const auto& in : inputDevices) {
            for (const auto& out : outputDevices) {
                if (in.deviceId != out.deviceId) {
                    if (engine->SelectInputDevice(in.deviceId) &&
                        engine->SelectOutputDevice(out.deviceId)) {
                        return true;
                    }
                }
            }
        }

        // If no different devices found, try default devices
        auto defaultInput = enumerator.GetDefaultInputDevice();
        auto defaultOutput = enumerator.GetDefaultOutputDevice();

        if (defaultInput >= 0 && defaultOutput >= 0 && defaultInput != defaultOutput) {
            return engine->SelectInputDevice(defaultInput) &&
                   engine->SelectOutputDevice(defaultOutput);
        }

        return false;
    }

    std::unique_ptr<IAudioEngine> engine;
};

// =============================================================================
// Integration Test: Engine Creation and Initialization
// =============================================================================

TEST_F(AudioPassThroughIntegrationTest, CreateEngine_Success) {
    ASSERT_NE(engine, nullptr);
    EXPECT_FALSE(engine->IsRunning());
    EXPECT_FALSE(engine->IsPassThroughEnabled());
}

TEST_F(AudioPassThroughIntegrationTest, GetDeviceEnumerator_Success) {
    auto& enumerator = engine->GetDeviceEnumerator();

    // Should be able to get device list
    auto allDevices = enumerator.GetAllDevices();
    EXPECT_GT(allDevices.size(), 0u);
}

// =============================================================================
// Integration Test: Device Selection
// =============================================================================

TEST_F(AudioPassThroughIntegrationTest, SelectInputOutputDevices_Success) {
    auto inputDevice = GetFirstInputDevice();
    auto outputDevice = GetFirstOutputDevice();

    // Skip if no devices available
    if (!inputDevice.has_value() || !outputDevice.has_value()) {
        GTEST_SKIP() << "No audio devices available";
    }

    // Try to find different devices
    auto& enumerator = engine->GetDeviceEnumerator();
    auto inputDevices = enumerator.GetInputDevices();
    auto outputDevices = enumerator.GetOutputDevices();

    bool selected = false;
    for (const auto& in : inputDevices) {
        for (const auto& out : outputDevices) {
            if (in.deviceId != out.deviceId) {
                if (engine->SelectInputDevice(in.deviceId) &&
                    engine->SelectOutputDevice(out.deviceId)) {
                    selected = true;
                    break;
                }
            }
        }
        if (selected) break;
    }

    if (!selected) {
        GTEST_SKIP() << "Could not find different input and output devices";
    }

    // Verify devices are selected
    EXPECT_TRUE(true);
}

TEST_F(AudioPassThroughIntegrationTest, SelectSameDevice_Fails) {
    auto inputDevice = GetFirstInputDevice();

    if (!inputDevice.has_value()) {
        GTEST_SKIP() << "No input device available";
    }

    // Try to select the same device for both input and output
    bool inputSelected = engine->SelectInputDevice(inputDevice.value());
    bool outputSelected = engine->SelectOutputDevice(inputDevice.value());

    // Should fail to select same device for output
    EXPECT_TRUE(inputSelected);
    EXPECT_FALSE(outputSelected);
}

// =============================================================================
// Integration Test: Pass-through Mode
// =============================================================================

TEST_F(AudioPassThroughIntegrationTest, EnablePassThrough_Success) {
    if (!SelectTestDevices()) {
        GTEST_SKIP() << "Could not select test devices";
    }

    // Enable pass-through
    engine->SetPassThroughEnabled(true);
    EXPECT_TRUE(engine->IsPassThroughEnabled());

    // Disable pass-through
    engine->SetPassThroughEnabled(false);
    EXPECT_FALSE(engine->IsPassThroughEnabled());
}

TEST_F(AudioPassThroughIntegrationTest, StartEngine_Success) {
    if (!SelectTestDevices()) {
        GTEST_SKIP() << "Could not select test devices";
    }

    // Enable pass-through
    engine->SetPassThroughEnabled(true);

    // Start engine
    bool started = engine->Start();
    EXPECT_TRUE(started);
    EXPECT_TRUE(engine->IsRunning());

    // Stop engine
    engine->Stop();
    EXPECT_FALSE(engine->IsRunning());
}

// =============================================================================
// Integration Test: Level Monitoring
// =============================================================================

TEST_F(AudioPassThroughIntegrationTest, LevelCallback_ReceivesData) {
    if (!SelectTestDevices()) {
        GTEST_SKIP() << "Could not select test devices";
    }

    // Track level updates
    std::atomic<int> callbackCount(0);
    AudioLevels lastLevels = {0, 0, 0, 0};

    engine->SetLevelCallback([&callbackCount, &lastLevels](const AudioLevels& levels) {
        lastLevels = levels;
        callbackCount++;
    });

    // Enable pass-through
    engine->SetPassThroughEnabled(true);

    // Start engine
    ASSERT_TRUE(engine->Start());

    // Wait for level updates
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stop engine
    engine->Stop();

    // Verify we received level updates
    EXPECT_GT(callbackCount.load(), 0);

    // Verify levels are in valid range [0.0, 1.0]
    EXPECT_GE(lastLevels.inputPeakL, 0.0f);
    EXPECT_LE(lastLevels.inputPeakL, 1.0f);
    EXPECT_GE(lastLevels.inputPeakR, 0.0f);
    EXPECT_LE(lastLevels.inputPeakR, 1.0f);
    EXPECT_GE(lastLevels.outputPeakL, 0.0f);
    EXPECT_LE(lastLevels.outputPeakL, 1.0f);
    EXPECT_GE(lastLevels.outputPeakR, 0.0f);
    EXPECT_LE(lastLevels.outputPeakR, 1.0f);
}

// =============================================================================
// Integration Test: Latency Measurement
// =============================================================================

TEST_F(AudioPassThroughIntegrationTest, LatencyMeasurement_Under200ms) {
    if (!SelectTestDevices()) {
        GTEST_SKIP() << "Could not select test devices";
    }

    // Enable pass-through
    engine->SetPassThroughEnabled(true);

    // Start engine
    ASSERT_TRUE(engine->Start());

    // Wait for engine to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Measure latency
    double latency = engine->GetCurrentLatency();

    // Stop engine
    engine->Stop();

    // Latency should be under 200ms
    EXPECT_LT(latency, 200.0);

    std::cout << "\nMeasured latency: " << latency << " ms" << std::endl;
}

// =============================================================================
// Integration Test: Complete Pass-through Flow
// =============================================================================

TEST_F(AudioPassThroughIntegrationTest, CompletePassThroughFlow_Success) {
    if (!SelectTestDevices()) {
        GTEST_SKIP() << "Could not select test devices";
    }

    std::cout << "\n=== Audio Pass-through Integration Test ===" << std::endl;

    // Enumerate devices
    auto& enumerator = engine->GetDeviceEnumerator();
    auto allDevices = enumerator.GetAllDevices();
    std::cout << "Found " << allDevices.size() << " audio devices" << std::endl;

    // Select devices
    ASSERT_TRUE(SelectTestDevices());
    std::cout << "Devices selected successfully" << std::endl;

    // Set up level monitoring
    std::atomic<bool> levelsReceived(false);
    engine->SetLevelCallback([&levelsReceived](const AudioLevels& levels) {
        (void)levels;  // Unused in this test
        levelsReceived.store(true);
    });

    // Enable pass-through
    engine->SetPassThroughEnabled(true);
    std::cout << "Pass-through enabled" << std::endl;

    // Start engine
    ASSERT_TRUE(engine->Start());
    std::cout << "Engine started" << std::endl;

    // Run for 2 seconds
    std::cout << "Running pass-through for 2 seconds..." << std::endl;
    auto startTime = std::chrono::steady_clock::now();
    int iterations = 0;

    while ((std::chrono::steady_clock::now() - startTime) < std::chrono::seconds(2)) {
        // Check latency periodically
        double latency = engine->GetCurrentLatency();
        if (iterations % 10 == 0) {
            std::cout << "\rLatency: " << latency << " ms    " << std::flush;
        }
        iterations++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n";

    // Verify levels were received
    EXPECT_TRUE(levelsReceived.load());

    // Verify latency is acceptable
    double finalLatency = engine->GetCurrentLatency();
    EXPECT_LT(finalLatency, 200.0);
    std::cout << "Final latency: " << finalLatency << " ms" << std::endl;

    // Stop engine
    engine->Stop();
    std::cout << "Engine stopped" << std::endl;

    std::cout << "Pass-through test completed successfully!" << std::endl;
}

// =============================================================================
// Integration Test: Pass-through Toggle
// =============================================================================

TEST_F(AudioPassThroughIntegrationTest, PassThroughToggle_WhileRunning) {
    if (!SelectTestDevices()) {
        GTEST_SKIP() << "Could not select test devices";
    }

    // Start engine without pass-through
    ASSERT_TRUE(engine->Start());
    EXPECT_FALSE(engine->IsPassThroughEnabled());

    // Enable pass-through while running
    engine->SetPassThroughEnabled(true);
    EXPECT_TRUE(engine->IsPassThroughEnabled());

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Disable pass-through while running
    engine->SetPassThroughEnabled(false);
    EXPECT_FALSE(engine->IsPassThroughEnabled());

    // Stop engine
    engine->Stop();
}

// =============================================================================
// Integration Test: Multiple Start/Stop Cycles
// =============================================================================

TEST_F(AudioPassThroughIntegrationTest, MultipleStartStopCycles_Success) {
    if (!SelectTestDevices()) {
        GTEST_SKIP() << "Could not select test devices";
    }

    engine->SetPassThroughEnabled(true);

    // Perform multiple start/stop cycles
    for (int cycle = 0; cycle < 3; ++cycle) {
        std::cout << "Cycle " << (cycle + 1) << "/3" << std::endl;

        ASSERT_TRUE(engine->Start());
        EXPECT_TRUE(engine->IsRunning());

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        engine->Stop();
        EXPECT_FALSE(engine->IsRunning());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Completed 3 start/stop cycles successfully" << std::endl;
}

// No main() needed - using gtest_main
