/**
 * TestRunner.cpp
 *
 * Implementation of test execution lifecycle management
 */

#include "TestRunner.h"
#include "VirtualDeviceManager.h"
#include "AudioValidator.h"
#include "ConfigManager.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>

namespace audioBridge {
namespace testing {

TestRunner::TestRunner()
    : state_(TestState::IDLE) {
    deviceManager_ = std::make_unique<VirtualDeviceManager>();
    validator_ = std::make_unique<AudioValidator>();
    configManager_ = std::make_unique<ConfigManager>();
    spdlog::debug("TestRunner initialized");
}

TestRunner::~TestRunner() {
    cleanup();
    spdlog::debug("TestRunner destroyed");
}

bool TestRunner::initialize(const std::string& testAudioFile,
                             const std::string& configFile) {
    spdlog::info("Initializing test run for audio file: {}", testAudioFile);

    execution_.executionId = generateExecutionId();
    execution_.timestamp = std::chrono::system_clock::now();
    execution_.testAudioFile = testAudioFile;
    execution_.status = TestState::IDLE;
    execution_.framesCaptured = 0;
    execution_.duration = 0.0f;

    state_ = TestState::IDLE;
    return true;
}

bool TestRunner::run() {
    if (state_ != TestState::IDLE) {
        lastError_ = "Test runner not in IDLE state";
        spdlog::error(lastError_);
        return false;
    }

    spdlog::info("Starting test execution: {}", execution_.executionId);
    state_ = TestState::RUNNING;
    execution_.status = TestState::RUNNING;

    // TODO: Implement full test execution in Phase 3 (User Story 1)
    // For now, provide framework structure

    // Setup devices
    if (!setupDevices()) {
        state_ = TestState::FAILED;
        execution_.status = TestState::FAILED;
        return false;
    }

    // Execute capture (placeholder)
    if (!executeCapture()) {
        state_ = TestState::FAILED;
        execution_.status = TestState::FAILED;
        return false;
    }

    state_ = TestState::COMPLETED;
    execution_.status = TestState::COMPLETED;

    spdlog::info("Test execution completed: {}", execution_.executionId);
    return true;
}

void TestRunner::cleanup() {
    spdlog::debug("Cleaning up test runner");
    state_ = TestState::IDLE;
}

void TestRunner::interrupt() {
    spdlog::warn("Test interrupted by user");
    state_ = TestState::INTERRUPTED;
    execution_.status = TestState::INTERRUPTED;
}

std::string TestRunner::generateExecutionId() {
    // Generate unique ID: exec-YYYYMMDD-HHMMSS-random
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    std::stringstream ss;
    ss << "exec-" << std::put_time(std::localtime(&time), "%Y%m%d-%H%M%S")
       << "-" << dis(gen);

    return ss.str();
}

std::string TestRunner::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

bool TestRunner::setupDevices() {
    spdlog::info("Setting up virtual audio devices");

    // Check if loopback module is loaded
    if (!deviceManager_->isLoopbackModuleLoaded()) {
        lastError_ = "snd-aloop module not loaded. Run: sudo modprobe snd-aloop";
        spdlog::error(lastError_);
        return false;
    }

    // Auto-detect loopback pair
    VirtualAudioDevice playback, capture;
    if (!deviceManager_->autoDetectLoopbackPair(playback, capture)) {
        lastError_ = deviceManager_->getLastError();
        spdlog::error(lastError_);
        return false;
    }

    execution_.playbackDevice = playback.deviceName;
    execution_.captureDevice = capture.deviceName;

    spdlog::info("Devices configured: {} -> {}", playback.deviceName, capture.deviceName);
    return true;
}

bool TestRunner::executeCapture() {
    spdlog::info("Executing audio capture (placeholder implementation)");

    // TODO: Implement actual audio capture in Phase 3
    // This will involve:
    // 1. Opening PortAudio streams for playback and capture
    // 2. Playing test audio file
    // 3. Capturing loopback audio
    // 4. Writing captured audio to file
    // 5. Validating captured audio

    spdlog::warn("Audio capture not yet implemented - framework ready for Phase 3");

    // Simulate successful capture for now
    execution_.capturedFile = "/tmp/captured-" + execution_.executionId + ".wav";
    execution_.duration = 5.0f;
    execution_.framesCaptured = 240000; // 5 seconds * 48000 Hz

    return true;
}

} // namespace testing
} // namespace audioBridge
