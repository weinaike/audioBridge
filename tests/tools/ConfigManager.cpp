/**
 * ConfigManager.cpp
 *
 * Implementation of configuration management
 */

#include "ConfigManager.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

namespace audioBridge {
namespace testing {

ConfigManager::ConfigManager() {
    spdlog::debug("ConfigManager initialized");
}

ConfigManager::~ConfigManager() {
    spdlog::debug("ConfigManager destroyed");
}

bool ConfigManager::loadConfig(const std::string& filepath, TestConfiguration& outConfig) {
    spdlog::info("Loading configuration from {}", filepath);

    std::string content;
    if (!readFile(filepath, content)) {
        return false;
    }

    // TODO: Parse JSON using nlohmann/json library
    // For now, provide framework structure
    spdlog::warn("JSON parsing is placeholder - use nlohmann/json in production");

    // Set default values for now
    outConfig.configId = "default";
    outConfig.name = "Default Configuration";
    outConfig.autoDetectDevices = true;
    outConfig.sampleRate = 48000;
    outConfig.framesPerBuffer = 128;
    outConfig.channelCount = 1;
    outConfig.defaultDuration = 5.0f;
    outConfig.validationEnabled = true;
    outConfig.latencyMeasurementEnabled = true;

    spdlog::info("Configuration loaded (using defaults)");
    return true;
}

bool ConfigManager::saveConfig(const std::string& filepath, const TestConfiguration& config) {
    spdlog::info("Saving configuration to {}", filepath);

    // TODO: Serialize to JSON using nlohmann/json
    spdlog::warn("JSON serialization is placeholder - use nlohmann/json in production");

    return true;
}

bool ConfigManager::loadSuiteConfig(const std::string& filepath) {
    spdlog::info("Loading test suite from {}", filepath);

    std::string content;
    if (!readFile(filepath, content)) {
        return false;
    }

    // TODO: Parse suite configuration
    spdlog::warn("Suite config parsing is placeholder");
    return true;
}

bool ConfigManager::validateConfig(const std::string& filepath) {
    spdlog::info("Validating configuration {}", filepath);

    // TODO: Validate against JSON schema
    spdlog::warn("Schema validation is placeholder - use json-schema-validator in production");
    return true;
}

bool ConfigManager::readFile(const std::string& filepath, std::string& outContent) {
    std::ifstream file(filepath);
    if (!file) {
        lastError_ = "Cannot open file: " + filepath;
        spdlog::error(lastError_);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    outContent = buffer.str();

    return true;
}

bool ConfigManager::writeFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file) {
        lastError_ = "Cannot create file: " + filepath;
        spdlog::error(lastError_);
        return false;
    }

    file << content;
    return true;
}

bool ConfigManager::parseJson(const std::string& json) {
    // TODO: Implement proper JSON parsing
    return true;
}

// T101: Load validation thresholds from config file
bool ConfigManager::loadValidationThresholds(const std::string& filepath,
                                              ValidationThresholds& outThresholds) {
    spdlog::info("Loading validation thresholds from {}", filepath);

    std::string content;
    if (!readFile(filepath, content)) {
        return false;
    }

    // Simple JSON parsing for validationThresholds section
    // This is a lightweight parser that extracts the validationThresholds object
    // without requiring a full JSON library

    // Find the validationThresholds section
    std::string key = "\"validationThresholds\"";
    size_t keyPos = content.find(key);
    if (keyPos == std::string::npos) {
        lastError_ = "validationThresholds section not found in config";
        spdlog::warn(lastError_ + ", using defaults");
        // Return success with defaults (already set by constructor)
        return true;
    }

    // Find the opening brace after the key
    size_t braceStart = content.find('{', keyPos);
    if (braceStart == std::string::npos) {
        lastError_ = "Invalid validationThresholds format (missing opening brace)";
        spdlog::warn(lastError_ + ", using defaults");
        return true;
    }

    // Find the matching closing brace
    int braceCount = 1;
    size_t braceEnd = braceStart + 1;
    for (; braceEnd < content.length() && braceCount > 0; ++braceEnd) {
        if (content[braceEnd] == '{') braceCount++;
        else if (content[braceEnd] == '}') braceCount--;
    }

    if (braceCount != 0) {
        lastError_ = "Invalid validationThresholds format (unbalanced braces)";
        spdlog::warn(lastError_ + ", using defaults");
        return true;
    }

    // Extract the validationThresholds content
    std::string thresholdsJson = content.substr(braceStart, braceEnd - braceStart);

    // Helper lambda to extract numeric values from JSON
    auto extractNumber = [&thresholdsJson, this](const std::string& key, float defaultValue) -> float {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = thresholdsJson.find(searchKey);
        if (pos == std::string::npos) return defaultValue;

        size_t colonPos = thresholdsJson.find(':', pos);
        if (colonPos == std::string::npos) return defaultValue;

        // Skip whitespace after colon
        size_t valueStart = colonPos + 1;
        while (valueStart < thresholdsJson.length() &&
               (thresholdsJson[valueStart] == ' ' || thresholdsJson[valueStart] == '\t' ||
                thresholdsJson[valueStart] == '\n' || thresholdsJson[valueStart] == '\r')) {
            valueStart++;
        }

        // Parse number (handle both integers and floats)
        size_t valueEnd = valueStart;
        bool hasDecimal = false;
        while (valueEnd < thresholdsJson.length() &&
               (std::isdigit(thresholdsJson[valueEnd]) ||
                thresholdsJson[valueEnd] == '.' ||
                thresholdsJson[valueEnd] == '-' ||
                thresholdsJson[valueEnd] == 'e' ||
                thresholdsJson[valueEnd] == 'E' ||
                thresholdsJson[valueEnd] == '+')) {
            if (thresholdsJson[valueEnd] == '.') hasDecimal = true;
            valueEnd++;
        }

        if (valueStart == valueEnd) return defaultValue;

        std::string valueStr = thresholdsJson.substr(valueStart, valueEnd - valueStart);
        try {
            return std::stof(valueStr);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to parse {} value: {}", key, valueStr);
            return defaultValue;
        }
    };

    // Extract all threshold values
    outThresholds.minFileSize = static_cast<size_t>(extractNumber("minFileSize", 100000.0f));
    outThresholds.maxFileSizeDeviation = extractNumber("maxFileSizeDeviation", 0.1f);
    outThresholds.minDuration = extractNumber("minDuration", 4.5f);
    outThresholds.maxDuration = extractNumber("maxDuration", 6.0f);
    outThresholds.minSampleRate = static_cast<int>(extractNumber("minSampleRate", 47000.0f));
    outThresholds.maxSampleRate = static_cast<int>(extractNumber("maxSampleRate", 49000.0f));

    // Phase 5: Signal quality thresholds
    outThresholds.minSNR = extractNumber("minSNR", 40.0f);
    outThresholds.maxTHD = extractNumber("maxTHD", 1.0f);

    // Latency thresholds
    outThresholds.maxLatency = extractNumber("maxLatency", 100.0f);
    outThresholds.latencyTolerance = extractNumber("latencyTolerance", 10.0f);

    // Frequency analysis thresholds
    outThresholds.frequencyTolerance = extractNumber("frequencyTolerance", 5.0f);
    outThresholds.expectedFrequency = extractNumber("expectedFrequency", 1000.0f);

    spdlog::info("Validation thresholds loaded successfully:");
    spdlog::info("  - minSNR: {} dB", outThresholds.minSNR);
    spdlog::info("  - maxTHD: {} %", outThresholds.maxTHD);
    spdlog::info("  - maxLatency: {} ms", outThresholds.maxLatency);
    spdlog::info("  - frequencyTolerance: {} Hz", outThresholds.frequencyTolerance);
    spdlog::info("  - expectedFrequency: {} Hz", outThresholds.expectedFrequency);

    return true;
}

} // namespace testing
} // namespace audioBridge
