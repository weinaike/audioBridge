/**
 * LatencyMeasurer.h
 *
 * Audio latency measurement for virtual loopback testing
 * Phase 5: Advanced validation - timestamp-based latency measurement
 *
 * Measures round-trip latency by capturing timestamps at:
 * - T0: Playback start (before audio sent to output device)
 * - T1: Capture start (first audio callback on input device)
 *
 * Latency = T1 - T0 (in milliseconds)
 */

#ifndef LATENCY_MEASURER_H
#define LATENCY_MEASURER_H

#include <chrono>
#include <string>
#include <vector>

namespace audioBridge {
namespace testing {

/**
 * Latency measurement result (T083-T087)
 */
struct LatencyMeasurementResult {
    bool success;
    float latencyMs;              // Round-trip latency in milliseconds
    std::chrono::system_clock::time_point playbackStartTime;
    std::chrono::system_clock::time_point captureStartTime;

    // Statistics across multiple measurements
    std::vector<float> measurements;  // All individual measurements
    float averageLatency;             // Average of all measurements
    float minLatency;                 // Minimum latency observed
    float maxLatency;                 // Maximum latency observed
    float stdDeviation;               // Standard deviation

    // Validation (T087)
    bool withinTolerance;             // Is latency within ±10ms tolerance?
    float consistency;                // How consistent are measurements (0-1 scale)

    LatencyMeasurementResult() : success(false), latencyMs(0.0f),
                                 averageLatency(0.0f), minLatency(0.0f),
                                 maxLatency(0.0f), stdDeviation(0.0f),
                                 withinTolerance(false), consistency(0.0f) {}
};

/**
 * Latency measurement state
 */
enum class LatencyState {
    IDLE,
    PLAYBACK_STARTED,    // T0 captured
    CAPTURE_STARTED,     // T1 captured, latency calculated
    COMPLETED,
    FAILED
};

/**
 * LatencyMeasurer
 *
 * Measures round-trip audio latency in virtual loopback testing
 * Uses high-resolution timestamps for accurate measurement
 */
class LatencyMeasurer {
public:
    LatencyMeasurer();
    ~LatencyMeasurer();

    // T084: Capture playback start timestamp (before PortAudio write)
    void markPlaybackStart();

    // T085: Capture playback stop timestamp
    void markPlaybackStop();

    // T086: Capture start timestamp (on first audio callback)
    void markCaptureStart();

    // T086: Calculate round-trip latency (T1 - T0 in milliseconds)
    float calculateLatency() const;

    // Get current measurement result
    LatencyMeasurementResult getMeasurement() const;

    // Reset for next measurement
    void reset();

    // Multiple measurement support
    void addMeasurement(float latencyMs);
    LatencyMeasurementResult getStatistics() const;

    // T087: Validate latency consistency (±10ms tolerance across runs)
    bool validateConsistency(float toleranceMs = 10.0f) const;

    // Get state
    LatencyState getState() const { return state_; }

    // Check if measurement is complete
    bool isComplete() const { return state_ == LatencyState::CAPTURE_STARTED; }

    // Get last error
    std::string getLastError() const { return lastError_; }

    // Convert to human-readable string
    std::string toString() const;

private:
    LatencyState state_;

    // Timestamps
    std::chrono::system_clock::time_point playbackStartTime_;
    std::chrono::system_clock::time_point playbackStopTime_;
    std::chrono::system_clock::time_point captureStartTime_;

    // Multiple measurements
    std::vector<float> measurements_;

    // Current latency value
    float currentLatency_;

    std::string lastError_;

    // Helper: Calculate statistics
    void calculateStatistics(LatencyMeasurementResult& result) const;

    // Helper: Calculate standard deviation
    float calculateStdDev(float mean) const;
};

} // namespace testing
} // namespace audioBridge

#endif // LATENCY_MEASURER_H
