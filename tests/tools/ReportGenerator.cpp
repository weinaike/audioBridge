/**
 * ReportGenerator.cpp
 *
 * Implementation of report generation
 */

#include "ReportGenerator.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <iostream>

namespace audioBridge {
namespace testing {

ReportGenerator::ReportGenerator() {
    spdlog::debug("ReportGenerator initialized");
}

ReportGenerator::~ReportGenerator() {
    spdlog::debug("ReportGenerator destroyed");
}

std::string ReportGenerator::generateReport(const TestExecution& execution,
                                             ReportFormat format) {
    switch (format) {
        case ReportFormat::TEXT:
            return generateTextReport(execution);
        case ReportFormat::JSON:
            return generateJsonReport(execution);
        case ReportFormat::JUNIT_XML:
            return generateJUnitReport(execution);
        default:
            return generateTextReport(execution);
    }
}

std::string ReportGenerator::generateTextReport(const TestExecution& execution) {
    std::stringstream ss;

    ss << createDivider() << "\n";
    ss << "AudioBridge Test Report\n";
    ss << createDivider() << "\n\n";

    ss << "Test: " << execution.testAudioFile << "\n";
    ss << "Execution ID: " << execution.executionId << "\n";
    ss << "Timestamp: " << formatTimestamp(execution.timestamp) << "\n\n";

    ss << "Configuration:\n";
    ss << "  Playback Device: " << execution.playbackDevice << "\n";
    ss << "  Capture Device: " << execution.captureDevice << "\n\n";

    ss << "Result: " << formatStatus(execution.status) << "\n\n";

    if (!execution.capturedFile.empty()) {
        ss << "Captured Audio:\n";
        ss << "  File: " << execution.capturedFile << "\n";
        ss << "  Duration: " << execution.duration << " seconds\n";
        ss << "  Frames: " << execution.framesCaptured << "\n\n";
    }

    // T091: Extended validation reporting
    ss << createDivider(40, '-') << "\n";
    ss << "Validation Results\n";
    ss << createDivider(40, '-') << "\n\n";

    // T088: Frequency analysis
    if (execution.hasFrequencyAnalysis) {
        ss << "Frequency Analysis:\n";
        ss << "  Peak Frequency: " << execution.frequencyAnalysis.peakFrequency << " Hz\n";
        if (execution.frequencyAnalysis.withinTolerance) {
            ss << "  Status: ✓ WITHIN TOLERANCE (±5 Hz)\n";
        } else {
            ss << "  Status: ✗ OUT OF TOLERANCE (error: "
               << execution.frequencyAnalysis.frequencyError << " Hz)\n";
        }
        ss << "\n";
    }

    // T089: Signal quality metrics
    if (execution.hasSignalQuality && execution.signalQuality.valid) {
        ss << "Signal Quality:\n";
        ss << "  SNR: " << std::fixed << std::setprecision(2)
           << execution.signalQuality.snr << " dB\n";
        ss << "  THD: " << std::fixed << std::setprecision(4)
           << execution.signalQuality.thd << "%\n";
        ss << "  Peak Amplitude: " << std::fixed << std::setprecision(6)
           << execution.signalQuality.peakAmplitude << "\n";
        ss << "  RMS Level: " << std::fixed << std::setprecision(6)
           << execution.signalQuality.rmsLevel << "\n";
        ss << "  Noise Floor: " << std::fixed << std::setprecision(2)
           << execution.signalQuality.noiseFloor << " dB\n";
        ss << "\n";
    }

    // T090: Latency measurement
    if (execution.hasLatencyMeasurement && execution.latencyMeasurement.success) {
        ss << "Latency Measurement:\n";
        ss << "  Round-trip Latency: " << std::fixed << std::setprecision(2)
           << execution.latencyMeasurement.latencyMs << " ms\n";

        if (execution.latencyMeasurement.measurements.size() > 1) {
            ss << "  Statistics (" << execution.latencyMeasurement.measurements.size()
               << " measurements):\n";
            ss << "    Average: " << execution.latencyMeasurement.averageLatency << " ms\n";
            ss << "    Min: " << execution.latencyMeasurement.minLatency << " ms\n";
            ss << "    Max: " << execution.latencyMeasurement.maxLatency << " ms\n";
            ss << "    Std Dev: " << std::fixed << std::setprecision(3)
               << execution.latencyMeasurement.stdDeviation << " ms\n";
            ss << "  Consistency: " << (execution.latencyMeasurement.withinTolerance ?
                "✓ PASS" : "✗ FAIL") << "\n";
        }
        ss << "\n";
    }

    // T093: Overall validation result
    ss << createDivider(40, '-') << "\n";
    ss << "Overall Validation: "
       << (execution.validationPassed ? "✓ PASSED" : "✗ FAILED") << "\n";
    if (!execution.validationMessage.empty()) {
        ss << "Message: " << execution.validationMessage << "\n";
    }
    ss << createDivider() << "\n";

    if (execution.status == TestState::FAILED) {
        ss << "Error: " << execution.errorMessage << "\n\n";
    }

    return ss.str();
}

std::string ReportGenerator::generateJsonReport(const TestExecution& execution) {
    std::stringstream ss;

    ss << "{\n";
    ss << "  \"reportType\": \"test-execution\",\n";
    ss << "  \"version\": \"2.0\",\n";  // T092: Version bump for extended format
    ss << "  \"executionId\": \"" << execution.executionId << "\",\n";
    ss << "  \"timestamp\": \"" << formatTimestamp(execution.timestamp) << "\",\n";
    ss << "  \"testName\": \"" << execution.testAudioFile << "\",\n";
    ss << "  \"status\": \"" << formatStatus(execution.status) << "\",\n";

    if (!execution.capturedFile.empty()) {
        ss << "  \"capturedFile\": \"" << execution.capturedFile << "\",\n";
        ss << "  \"duration\": " << execution.duration << ",\n";
        ss << "  \"framesCaptured\": " << execution.framesCaptured << ",\n";
    }

    if (execution.status == TestState::FAILED) {
        ss << "  \"error\": \"" << execution.errorMessage << "\",\n";
    }

    ss << "  \"playbackDevice\": \"" << execution.playbackDevice << "\",\n";
    ss << "  \"captureDevice\": \"" << execution.captureDevice << "\",\n";

    // T092: Extended JSON format with validation results
    ss << "  \"validation\": {\n";

    // T088: Frequency analysis
    if (execution.hasFrequencyAnalysis) {
        ss << "    \"frequencyAnalysis\": {\n";
        ss << "      \"peakFrequency\": " << execution.frequencyAnalysis.peakFrequency << ",\n";
        ss << "      \"magnitude\": " << execution.frequencyAnalysis.magnitude << ",\n";
        ss << "      \"withinTolerance\": " << (execution.frequencyAnalysis.withinTolerance ? "true" : "false") << ",\n";
        ss << "      \"frequencyError\": " << execution.frequencyAnalysis.frequencyError << ",\n";
        ss << "      \"matchesExpected\": " << (execution.frequencyAnalysis.matchesExpected ? "true" : "false") << "\n";
        ss << "    }" << (execution.hasSignalQuality || execution.hasLatencyMeasurement ? "," : "") << "\n";
    }

    // T089: Signal quality metrics
    if (execution.hasSignalQuality && execution.signalQuality.valid) {
        ss << "    \"signalQuality\": {\n";
        ss << "      \"snr\": " << std::fixed << std::setprecision(2) << execution.signalQuality.snr << ",\n";
        ss << "      \"thd\": " << std::fixed << std::setprecision(4) << execution.signalQuality.thd << ",\n";
        ss << "      \"peakAmplitude\": " << std::fixed << std::setprecision(6) << execution.signalQuality.peakAmplitude << ",\n";
        ss << "      \"rmsLevel\": " << std::fixed << std::setprecision(6) << execution.signalQuality.rmsLevel << ",\n";
        ss << "      \"noiseFloor\": " << std::fixed << std::setprecision(2) << execution.signalQuality.noiseFloor << ",\n";
        ss << "      \"valid\": " << (execution.signalQuality.valid ? "true" : "false") << "\n";
        ss << "    }" << (execution.hasLatencyMeasurement ? "," : "") << "\n";
    }

    // T090: Latency measurement
    if (execution.hasLatencyMeasurement && execution.latencyMeasurement.success) {
        ss << "    \"latencyMeasurement\": {\n";
        ss << "      \"latencyMs\": " << std::fixed << std::setprecision(2)
           << execution.latencyMeasurement.latencyMs << ",\n";
        ss << "      \"success\": " << (execution.latencyMeasurement.success ? "true" : "false") << ",\n";

        if (execution.latencyMeasurement.measurements.size() > 1) {
            ss << "      \"statistics\": {\n";
            ss << "        \"count\": " << execution.latencyMeasurement.measurements.size() << ",\n";
            ss << "        \"average\": " << execution.latencyMeasurement.averageLatency << ",\n";
            ss << "        \"min\": " << execution.latencyMeasurement.minLatency << ",\n";
            ss << "        \"max\": " << execution.latencyMeasurement.maxLatency << ",\n";
            ss << "        \"stdDeviation\": " << std::fixed << std::setprecision(3)
               << execution.latencyMeasurement.stdDeviation << ",\n";
            ss << "        \"withinTolerance\": " << (execution.latencyMeasurement.withinTolerance ? "true" : "false") << ",\n";
            ss << "        \"consistency\": " << std::fixed << std::setprecision(3)
               << execution.latencyMeasurement.consistency << "\n";
            ss << "      },\n";
        }

        ss << "      \"withinTolerance\": " << (execution.latencyMeasurement.withinTolerance ? "true" : "false") << "\n";
        ss << "    }\n";
    }

    // T093: Overall validation result
    ss << "    \"validationPassed\": " << (execution.validationPassed ? "true" : "false") << ",\n";
    if (!execution.validationMessage.empty()) {
        ss << "    \"validationMessage\": \"" << execution.validationMessage << "\",\n";
    }

    ss << "    \"timestamp\": \"" << formatTimestamp(std::chrono::system_clock::now()) << "\"\n";
    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}

std::string ReportGenerator::generateJUnitReport(const TestExecution& execution) {
    std::stringstream ss;

    // JUnit XML format for single test
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<testsuites>\n";
    ss << "  <testsuite name=\"audioBridge-test\" tests=\"1\" failures=\""
       << (execution.status == TestState::FAILED ? "1" : "0")
       << "\" time=\"" << execution.duration << "\">\n";
    ss << "    <testcase name=\"" << execution.testAudioFile
       << "\" classname=\"audioBridge.loopback\" time=\"" << execution.duration << "\">\n";

    if (execution.status == TestState::FAILED) {
        ss << "      <failure message=\"" << escapeXml(execution.errorMessage) << "\">\n";
        ss << "        Test execution failed: " << escapeXml(execution.errorMessage) << "\n";
        ss << "      </failure>\n";
    }

    // Add system-out with test details
    ss << "      <system-out>\n";
    ss << "        Execution ID: " << execution.executionId << "\n";
    ss << "        Playback Device: " << execution.playbackDevice << "\n";
    ss << "        Capture Device: " << execution.captureDevice << "\n";
    ss << "        Duration: " << execution.duration << " seconds\n";
    if (!execution.capturedFile.empty()) {
        ss << "        Captured File: " << execution.capturedFile << "\n";
        ss << "        Frames Captured: " << execution.framesCaptured << "\n";
    }
    ss << "      </system-out>\n";

    ss << "    </testcase>\n";
    ss << "  </testsuite>\n";
    ss << "</testsuites>\n";

    return ss.str();
}

std::string ReportGenerator::generateJUnitSuiteReport(const std::string& suiteName,
                                                       const std::vector<TestExecution>& executions,
                                                       int totalTests,
                                                       int passedTests,
                                                       int failedTests,
                                                       float duration) {
    std::stringstream ss;

    // JUnit XML format for test suite
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<testsuites>\n";
    ss << "  <testsuite name=\"" << escapeXml(suiteName) << "\" "
       << "tests=\"" << totalTests << "\" "
       << "failures=\"" << failedTests << "\" "
       << "errors=\"0\" "
       << "time=\"" << std::fixed << std::setprecision(3) << duration << "\">\n";

    // Add test suite properties
    ss << "    <properties>\n";
    ss << "      <property name=\"suite\" value=\"" << escapeXml(suiteName) << "\"/>\n";
    ss << "      <property name=\"timestamp\" value=\""
       << formatTimestamp(std::chrono::system_clock::now()) << "\"/>\n";
    ss << "    </properties>\n";

    // Add individual test cases
    for (const auto& execution : executions) {
        std::string testName = execution.testAudioFile;
        // Extract filename from path
        size_t pos = testName.find_last_of('/');
        if (pos != std::string::npos) {
            testName = testName.substr(pos + 1);
        }

        ss << "    <testcase name=\"" << escapeXml(testName)
           << "\" classname=\"audioBridge.loopback\" time=\""
           << std::fixed << std::setprecision(3) << execution.duration << "\">\n";

        if (execution.status == TestState::FAILED) {
            ss << "      <failure message=\"" << escapeXml(execution.errorMessage) << "\">\n";
            ss << "        Test execution failed\n";
            ss << "        Error: " << escapeXml(execution.errorMessage) << "\n";
            ss << "      </failure>\n";
        } else if (execution.status == TestState::INTERRUPTED) {
            ss << "      <error message=\"Test interrupted\"/>\n";
        }

        // Add system-out with test details
        ss << "      <system-out>\n";
        ss << "        Execution ID: " << execution.executionId << "\n";
        ss << "        Playback Device: " << execution.playbackDevice << "\n";
        ss << "        Capture Device: " << execution.captureDevice << "\n";
        ss << "        Duration: " << execution.duration << " seconds\n";

        if (!execution.capturedFile.empty()) {
            ss << "        Captured File: " << execution.capturedFile << "\n";
            ss << "        Frames Captured: " << execution.framesCaptured << "\n";
        }

        if (execution.status == TestState::COMPLETED) {
            ss << "        Result: PASSED\n";
        }

        ss << "      </system-out>\n";

        ss << "    </testcase>\n";
    }

    // Add system-out for overall results
    ss << "    <system-out>\n";
    ss << "      Test Suite: " << escapeXml(suiteName) << "\n";
    ss << "      Total Tests: " << totalTests << "\n";
    ss << "      Passed: " << passedTests << "\n";
    ss << "      Failed: " << failedTests << "\n";
    ss << "      Pass Rate: " << std::fixed << std::setprecision(1)
       << (100.0 * passedTests / totalTests) << "%\n";
    ss << "      Total Duration: " << std::fixed << std::setprecision(3)
       << duration << " seconds\n";
    ss << "    </system-out>\n";

    ss << "  </testsuite>\n";
    ss << "</testsuites>\n";

    return ss.str();
}

bool ReportGenerator::saveReport(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file) {
        lastError_ = "Cannot create report file: " + filepath;
        spdlog::error(lastError_);
        return false;
    }

    file << content;
    spdlog::info("Report saved to {}", filepath);
    return true;
}

std::string ReportGenerator::formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string ReportGenerator::formatStatus(TestState status) {
    switch (status) {
        case TestState::IDLE:
            return "IDLE";
        case TestState::RUNNING:
            return "RUNNING";
        case TestState::COMPLETED:
            return "SUCCESS ✓";
        case TestState::FAILED:
            return "FAILED ✗";
        case TestState::INTERRUPTED:
            return "INTERRUPTED";
        default:
            return "UNKNOWN";
    }
}

std::string ReportGenerator::createDivider(int length, char ch) {
    return std::string(length, ch);
}

std::string ReportGenerator::escapeXml(const std::string& input) {
    std::stringstream ss;
    for (char c : input) {
        switch (c) {
            case '&':
                ss << "&amp;";
                break;
            case '<':
                ss << "&lt;";
                break;
            case '>':
                ss << "&gt;";
                break;
            case '"':
                ss << "&quot;";
                break;
            case '\'':
                ss << "&apos;";
                break;
            default:
                ss << c;
                break;
        }
    }
    return ss.str();
}

// ============================================================================
// T093: Validation Pass/Fail Logic
// ============================================================================

bool ReportGenerator::determineValidationPass(const TestExecution& execution) {
    spdlog::debug("Determining validation pass/fail for execution: {}", execution.executionId);

    bool passed = true;
    std::vector<std::string> failures;

    // Check frequency analysis
    if (execution.hasFrequencyAnalysis) {
        if (!execution.frequencyAnalysis.withinTolerance) {
            passed = false;
            failures.push_back("Frequency out of tolerance");
        }
    }

    // Check signal quality metrics
    if (execution.hasSignalQuality && execution.signalQuality.valid) {
        // Typical thresholds
        if (execution.signalQuality.snr < 40.0f) {
            passed = false;
            failures.push_back("SNR below 40 dB");
        }
        if (execution.signalQuality.thd > 5.0f) {
            passed = false;
            failures.push_back("THD above 5%");
        }
    }

    // Check latency measurement
    if (execution.hasLatencyMeasurement && execution.latencyMeasurement.success) {
        if (execution.latencyMeasurement.latencyMs > 100.0f) {
            passed = false;
            failures.push_back("Latency above 100 ms");
        }
        if (execution.latencyMeasurement.measurements.size() > 1 &&
            !execution.latencyMeasurement.withinTolerance) {
            passed = false;
            failures.push_back("Latency inconsistency detected");
        }
    }

    if (!failures.empty()) {
        spdlog::debug("Validation failed: {}", failures[0]);
        for (const auto& failure : failures) {
            spdlog::debug("  - {}", failure);
        }
    }

    return passed;
}

std::string ReportGenerator::generateValidationMessage(const TestExecution& execution) {
    std::stringstream ss;

    if (execution.validationPassed) {
        ss << "All validation checks passed";
        if (execution.hasFrequencyAnalysis) {
            ss << " (frequency: " << std::fixed << std::setprecision(1)
               << execution.frequencyAnalysis.peakFrequency << " Hz)";
        }
    } else {
        ss << "Validation failed: ";

        std::vector<std::string> failures;

        if (execution.hasFrequencyAnalysis && !execution.frequencyAnalysis.withinTolerance) {
            failures.push_back("frequency error: " +
                             std::to_string(execution.frequencyAnalysis.frequencyError) + " Hz");
        }

        if (execution.hasSignalQuality && execution.signalQuality.valid) {
            if (execution.signalQuality.snr < 40.0f) {
                failures.push_back("SNR too low: " +
                                 std::to_string(execution.signalQuality.snr) + " dB");
            }
            if (execution.signalQuality.thd > 5.0f) {
                failures.push_back("THD too high: " +
                                 std::to_string(execution.signalQuality.thd) + "%");
            }
        }

        if (execution.hasLatencyMeasurement && execution.latencyMeasurement.success) {
            if (execution.latencyMeasurement.latencyMs > 100.0f) {
                failures.push_back("latency too high: " +
                                 std::to_string(execution.latencyMeasurement.latencyMs) + " ms");
            }
        }

        // Join failures with commas
        for (size_t i = 0; i < failures.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << failures[i];
        }
    }

    return ss.str();
}

} // namespace testing
} // namespace audioBridge
