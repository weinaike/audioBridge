/**
 * ReportGenerator.h
 *
 * Generates test reports in multiple formats (text, JSON, JUnit XML)
 */

#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include "TestRunner.h"
#include "AudioValidator.h"
#include <string>
#include <vector>

namespace audioBridge {
namespace testing {

/**
 * Report format types
 */
enum class ReportFormat {
    TEXT,
    JSON,
    JUNIT_XML
};

/**
 * Test report summary
 */
struct TestReport {
    std::string executionId;
    std::string timestamp;
    std::string testName;
    TestState status;
    int passedChecks;
    int failedChecks;
    int warnings;
    std::string capturedFile;

    // Validation results
    IntegrityCheckResult integrityCheck;

    // Duration
    float duration;
};

/**
 * Report generator for test results
 */
class ReportGenerator {
public:
    ReportGenerator();
    ~ReportGenerator();

    // Generate report from execution record
    std::string generateReport(const TestExecution& execution,
                               ReportFormat format = ReportFormat::TEXT);

    // Generate text report
    std::string generateTextReport(const TestExecution& execution);

    // Generate JSON report
    std::string generateJsonReport(const TestExecution& execution);

    // Generate JUnit XML report for single test
    std::string generateJUnitReport(const TestExecution& execution);

    // Generate JUnit XML report for test suite
    std::string generateJUnitSuiteReport(const std::string& suiteName,
                                         const std::vector<TestExecution>& executions,
                                         int totalTests,
                                         int passedTests,
                                         int failedTests,
                                         float duration);

    // Save report to file
    bool saveReport(const std::string& filepath, const std::string& content);

    // Get last error
    std::string getLastError() const { return lastError_; }

private:
    std::string lastError_;

    // Helper: Format timestamp
    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp);

    // Helper: Format status string
    std::string formatStatus(TestState status);

    // Helper: Create divider line
    std::string createDivider(int length = 60, char ch = '=');

    // Helper: Escape XML special characters
    std::string escapeXml(const std::string& input);

    // T093: Validation pass/fail determination
    bool determineValidationPass(const TestExecution& execution);

    // T093: Generate validation message
    std::string generateValidationMessage(const TestExecution& execution);
};

} // namespace testing
} // namespace audioBridge

#endif // REPORT_GENERATOR_H
