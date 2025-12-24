/**
 * TestRunner.h
 *
 * Test execution state management and lifecycle
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "AudioValidator.h"
#include "LatencyMeasurer.h"

namespace audioBridge {
namespace testing {

// Forward declarations
class VirtualDeviceManager;
class AudioValidator;
class ConfigManager;

/**
 * Test execution state
 */
enum class TestState {
    IDLE,
    RUNNING,
    COMPLETED,
    FAILED,
    INTERRUPTED
};

/**
 * Test execution record
 * T088-T090: Extended with validation results
 */
struct TestExecution {
    // Basic execution info
    std::string executionId;
    std::chrono::system_clock::time_point timestamp;
    std::string testAudioFile;
    std::string configuration;
    std::string playbackDevice;
    std::string captureDevice;
    TestState status;
    std::string errorMessage;
    std::string capturedFile;
    float duration;
    size_t framesCaptured;

    // T088: Frequency analysis results
    FrequencyAnalysisResult frequencyAnalysis;
    bool hasFrequencyAnalysis;

    // T089: Signal quality metrics
    SignalQualityMetrics signalQuality;
    bool hasSignalQuality;

    // T090: Latency measurements
    LatencyMeasurementResult latencyMeasurement;
    bool hasLatencyMeasurement;

    // T093: Overall validation result
    bool validationPassed;
    std::string validationMessage;

    // Constructor
    TestExecution() : duration(0.0f), framesCaptured(0),
                      hasFrequencyAnalysis(false), hasSignalQuality(false),
                      hasLatencyMeasurement(false), validationPassed(false) {}
};

/**
 * TestRunner - Manages test execution lifecycle
 */
class TestRunner {
public:
    TestRunner();
    ~TestRunner();

    // Initialize test run
    bool initialize(const std::string& testAudioFile,
                   const std::string& configFile = "");

    // Execute test
    bool run();

    // Cleanup after test
    void cleanup();

    // Get current state
    TestState getState() const { return state_; }

    // Get execution record
    const TestExecution& getExecutionRecord() const { return execution_; }

    // Interrupt test
    void interrupt();

    // Get last error
    std::string getLastError() const { return lastError_; }

private:
    TestState state_;
    TestExecution execution_;
    std::string lastError_;

    std::unique_ptr<VirtualDeviceManager> deviceManager_;
    std::unique_ptr<AudioValidator> validator_;
    std::unique_ptr<ConfigManager> configManager_;

    // Helper: Generate execution ID
    std::string generateExecutionId();

    // Helper: Get current timestamp as string
    std::string getCurrentTimestamp();

    // Helper: Setup devices
    bool setupDevices();

    // Helper: Execute audio capture
    bool executeCapture();
};

} // namespace testing
} // namespace audioBridge

#endif // TEST_RUNNER_H
