/**
 * LatencyMeasurer.cpp
 *
 * Implementation of latency measurement functionality
 */

#include "LatencyMeasurer.h"
#include <spdlog/spdlog.h>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace audioBridge {
namespace testing {

LatencyMeasurer::LatencyMeasurer()
    : state_(LatencyState::IDLE), currentLatency_(0.0f) {
    spdlog::debug("LatencyMeasurer initialized");
}

LatencyMeasurer::~LatencyMeasurer() {
    spdlog::debug("LatencyMeasurer destroyed");
}

// T084: Capture playback start timestamp
void LatencyMeasurer::markPlaybackStart() {
    playbackStartTime_ = std::chrono::system_clock::now();
    state_ = LatencyState::PLAYBACK_STARTED;

    spdlog::debug("Playback start timestamp captured");
}

void LatencyMeasurer::markPlaybackStop() {
    playbackStopTime_ = std::chrono::system_clock::now();

    spdlog::debug("Playback stop timestamp captured");
}

// T085: Capture start timestamp (on first audio callback)
void LatencyMeasurer::markCaptureStart() {
    if (state_ != LatencyState::PLAYBACK_STARTED) {
        lastError_ = "Cannot mark capture start before playback start";
        spdlog::error(lastError_);
        state_ = LatencyState::FAILED;
        return;
    }

    captureStartTime_ = std::chrono::system_clock::now();
    state_ = LatencyState::CAPTURE_STARTED;

    // T086: Calculate latency immediately
    currentLatency_ = calculateLatency();

    spdlog::debug("Capture start timestamp captured, latency: {:.2f} ms", currentLatency_);
}

// T086: Calculate round-trip latency (T1 - T0 in milliseconds)
float LatencyMeasurer::calculateLatency() const {
    if (state_ != LatencyState::CAPTURE_STARTED) {
        spdlog::warn("Cannot calculate latency - measurement not complete");
        return 0.0f;
    }

    // Calculate duration in milliseconds
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        captureStartTime_ - playbackStartTime_
    );

    float latencyMs = static_cast<float>(duration.count()) / 1000.0f;

    spdlog::debug("Calculated latency: {:.2f} ms ({:.2f} us)",
                  latencyMs, static_cast<float>(duration.count()));

    return latencyMs;
}

LatencyMeasurementResult LatencyMeasurer::getMeasurement() const {
    LatencyMeasurementResult result;

    if (state_ != LatencyState::CAPTURE_STARTED) {
        result.success = false;
        return result;
    }

    result.success = true;
    result.playbackStartTime = playbackStartTime_;
    result.captureStartTime = captureStartTime_;
    result.latencyMs = currentLatency_;

    // Copy measurements for statistics
    result.measurements = measurements_;
    calculateStatistics(result);

    // T087: Validate consistency
    result.withinTolerance = validateConsistency(10.0f);

    return result;
}

void LatencyMeasurer::reset() {
    state_ = LatencyState::IDLE;
    currentLatency_ = 0.0f;
    measurements_.clear();

    spdlog::debug("LatencyMeasurer reset");
}

void LatencyMeasurer::addMeasurement(float latencyMs) {
    measurements_.push_back(latencyMs);
    spdlog::debug("Added latency measurement: {:.2f} ms (total: {})",
                  latencyMs, measurements_.size());
}

LatencyMeasurementResult LatencyMeasurer::getStatistics() const {
    LatencyMeasurementResult result;

    if (measurements_.empty()) {
        result.success = false;
        return result;
    }

    result.success = true;
    result.measurements = measurements_;
    calculateStatistics(result);

    // T087: Validate consistency across all measurements
    result.withinTolerance = validateConsistency(10.0f);

    return result;
}

// T087: Validate latency consistency (±10ms tolerance across runs)
bool LatencyMeasurer::validateConsistency(float toleranceMs) const {
    if (measurements_.size() < 2) {
        // Not enough measurements to validate consistency
        return true;
    }

    float maxDiff = maxLatency_ - minLatency_;
    bool consistent = (maxDiff <= toleranceMs);

    spdlog::debug("Latency consistency: {:.2f} ms range (tolerance: {:.2f} ms) - {}",
                  maxDiff, toleranceMs, consistent ? "PASS" : "FAIL");

    return consistent;
}

std::string LatencyMeasurer::toString() const {
    std::stringstream ss;

    if (state_ != LatencyState::CAPTURE_STARTED) {
        ss << "Latency measurement not complete";
        return ss.str();
    }

    ss << std::fixed << std::setprecision(2);
    ss << "Latency: " << currentLatency_ << " ms\n";

    if (!measurements_.empty()) {
        LatencyMeasurementResult stats = getStatistics();
        ss << "Statistics (" << measurements_.size() << " measurements):\n";
        ss << "  Average: " << stats.averageLatency << " ms\n";
        ss << "  Min: " << stats.minLatency << " ms\n";
        ss << "  Max: " << stats.maxLatency << " ms\n";
        ss << "  Std Dev: " << stats.stdDeviation << " ms\n";
        ss << "  Consistency: " << (stats.withinTolerance ? "PASS" : "FAIL");
    }

    return ss.str();
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void LatencyMeasurer::calculateStatistics(LatencyMeasurementResult& result) const {
    if (result.measurements.empty()) {
        return;
    }

    // Calculate average
    float sum = std::accumulate(result.measurements.begin(), result.measurements.end(), 0.0f);
    result.averageLatency = sum / result.measurements.size();

    // Find min and max
    auto [minIt, maxIt] = std::minmax_element(result.measurements.begin(),
                                                result.measurements.end());
    result.minLatency = *minIt;
    result.maxLatency = *maxIt;

    // Calculate standard deviation
    result.stdDeviation = calculateStdDev(result.averageLatency);

    // Calculate consistency (1 - normalized_std_dev)
    if (result.averageLatency > 0.0f) {
        float normalizedStdDev = result.stdDeviation / result.averageLatency;
        result.consistency = std::max(0.0f, 1.0f - normalizedStdDev);
    } else {
        result.consistency = 0.0f;
    }
}

float LatencyMeasurer::calculateStdDev(float mean) const {
    if (measurements_.size() < 2) {
        return 0.0f;
    }

    float sumSquaredDiff = 0.0f;
    for (float measurement : measurements_) {
        float diff = measurement - mean;
        sumSquaredDiff += diff * diff;
    }

    float variance = sumSquaredDiff / measurements_.size();
    return std::sqrt(variance);
}

} // namespace testing
} // namespace audioBridge
