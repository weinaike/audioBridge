/**
 * ConfigManager.h
 *
 * Configuration file loading and saving for JSON test configs
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>

namespace audioBridge {
namespace testing {

/**
 * Validation thresholds for audio testing (T100, T102)
 */
struct ValidationThresholds {
    // Basic integrity thresholds
    size_t minFileSize;
    float maxFileSizeDeviation;
    float minDuration;
    float maxDuration;
    int minSampleRate;
    int maxSampleRate;

    // Signal quality thresholds (Phase 5)
    float minSNR;                  // Minimum Signal-to-Noise Ratio in dB
    float maxTHD;                  // Maximum Total Harmonic Distortion in percentage

    // Latency thresholds
    float maxLatency;              // Maximum acceptable latency in ms
    float latencyTolerance;        // Consistency tolerance in ms

    // Frequency analysis thresholds
    float frequencyTolerance;      // Frequency matching tolerance in Hz
    float expectedFrequency;       // Expected test frequency in Hz

    // Constructor with defaults
    ValidationThresholds()
        : minFileSize(100000)
        , maxFileSizeDeviation(0.1f)
        , minDuration(4.5f)
        , maxDuration(6.0f)
        , minSampleRate(47000)
        , maxSampleRate(49000)
        , minSNR(40.0f)
        , maxTHD(1.0f)
        , maxLatency(100.0f)
        , latencyTolerance(10.0f)
        , frequencyTolerance(5.0f)
        , expectedFrequency(1000.0f) {}
};

/**
 * Test configuration data structure (simplified for Phase 1)
 */
struct TestConfiguration {
    std::string configId;
    std::string name;
    std::string description;

    // Device settings
    int playbackDeviceId;
    int captureDeviceId;
    bool autoDetectDevices;

    // Audio settings
    int sampleRate;
    int framesPerBuffer;
    int channelCount;

    // Test settings
    float defaultDuration;
    std::string outputDirectory;
    std::string outputFormat;
    bool validationEnabled;
    bool latencyMeasurementEnabled;

    // T101: Validation thresholds from config
    ValidationThresholds validationThresholds;
    bool hasValidationThresholds;
};

/**
 * Configuration manager for loading/saving JSON configs
 */
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    // Load configuration from JSON file
    bool loadConfig(const std::string& filepath, TestConfiguration& outConfig);

    // Save configuration to JSON file
    bool saveConfig(const std::string& filepath, const TestConfiguration& config);

    // Load test suite configuration
    bool loadSuiteConfig(const std::string& filepath);

    // Validate configuration against schema
    bool validateConfig(const std::string& filepath);

    // Get last error
    std::string getLastError() const { return lastError_; }

    // T101: Load validation thresholds from config file
    bool loadValidationThresholds(const std::string& filepath, ValidationThresholds& outThresholds);

private:
    std::string lastError_;

    // Helper: Read file to string
    bool readFile(const std::string& filepath, std::string& outContent);

    // Helper: Write string to file
    bool writeFile(const std::string& filepath, const std::string& content);

    // Helper: Parse JSON (simplified - use nlohmann/json in production)
    bool parseJson(const std::string& json);
};

} // namespace testing
} // namespace audioBridge

#endif // CONFIG_MANAGER_H
