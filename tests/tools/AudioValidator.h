/**
 * AudioValidator.h
 *
 * Audio validation framework for testing captured audio
 * Phase 1: Basic integrity checks
 * Phase 3: Advanced validation with Gist library (frequency analysis, SNR, THD)
 */

#ifndef AUDIO_VALIDATOR_H
#define AUDIO_VALIDATOR_H

#include <string>
#include <vector>
#include <memory>

namespace audioBridge {
namespace testing {

/**
 * Result of basic integrity check
 */
struct IntegrityCheckResult {
    bool formatValid;
    bool sampleRateMatch;
    bool channelCountMatch;
    bool durationMatch;
    bool fileSizeValid;

    bool allValid() const {
        return formatValid && sampleRateMatch && channelCountMatch &&
               durationMatch && fileSizeValid;
    }
};

/**
 * Frequency analysis result (T075-T078)
 */
struct FrequencyAnalysisResult {
    bool success;
    float peakFrequency;        // Detected peak frequency in Hz
    float magnitude;            // Magnitude at peak frequency
    std::vector<float> spectrum; // Full frequency spectrum magnitudes

    // Frequency matching results
    bool matchesExpected;
    float frequencyError;       // Difference from expected in Hz
    bool withinTolerance;       // Is within specified tolerance

    FrequencyAnalysisResult() : success(false), peakFrequency(0.0f),
                                magnitude(0.0f), matchesExpected(false),
                                frequencyError(0.0f), withinTolerance(false) {}
};

/**
 * Signal quality metrics (T079-T082)
 */
struct SignalQualityMetrics {
    float snr;                  // Signal-to-Noise Ratio in dB
    float thd;                  // Total Harmonic Distortion in percentage
    float peakAmplitude;        // Peak amplitude in linear scale
    float rmsLevel;             // RMS level in linear scale
    float noiseFloor;           // Noise floor in dB

    bool valid;                 // Are all metrics valid?

    SignalQualityMetrics() : snr(0.0f), thd(0.0f), peakAmplitude(0.0f),
                             rmsLevel(0.0f), noiseFloor(-100.0f), valid(false) {}
};

/**
 * Audio validation framework
 *
 * Phase 1: Basic integrity validation
 * Phase 3: Extended with frequency analysis, SNR, THD, latency
 */
class AudioValidator {
public:
    AudioValidator();
    ~AudioValidator();

    // Basic integrity checks (Phase 1)
    IntegrityCheckResult checkIntegrity(const std::string& audioFile,
                                        int expectedSampleRate,
                                        int expectedChannels,
                                        float expectedDuration);

    // Validate file format (WAV, FLAC)
    bool isValidFormat(const std::string& audioFile);

    // Check file size is reasonable for duration
    bool isValidFileSize(const std::string& audioFile,
                        float duration,
                        int sampleRate,
                        int channels);

    // Get last error message
    std::string getLastError() const { return lastError_; }

    // === Phase 3: Advanced Validation with Gist Library ===

    // T075-T078: Frequency Analysis
    FrequencyAnalysisResult analyzeFrequency(const std::string& audioFile,
                                             float expectedFrequency,
                                             float toleranceHz = 5.0f);

    bool analyzeFrequency(const std::string& audioFile,
                         float& outPeakFrequency,
                         float expectedFrequency,
                         float tolerance);

    // T079-T082: Signal Quality Metrics
    SignalQualityMetrics calculateSignalQuality(const std::string& audioFile);

    float calculateSNR(const std::string& audioFile);
    float calculateTHD(const std::string& audioFile);
    float calculatePeakAmplitude(const std::string& audioFile);
    float calculateRMSLevel(const std::string& audioFile);
    float calculateNoiseFloor(const std::string& audioFile);

    // T099: Reference file comparison
    float compareToReference(const std::string& audioFile,
                             const std::string& referenceFile);

    // Calculate correlation coefficient between two audio files
    float calculateCorrelation(const std::vector<float>& audio1,
                               const std::vector<float>& audio2);

private:
    std::string lastError_;

    // Gist library integration
    class GistAnalyzer;
    std::unique_ptr<GistAnalyzer> gistAnalyzer_;

    // Helper: Get file extension
    std::string getFileExtension(const std::string& filepath);

    // Helper: Calculate expected file size
    size_t calculateExpectedSize(float duration, int sampleRate, int channels);

    // Helper: Load audio file into buffer
    std::vector<float> loadAudioFile(const std::string& audioFile,
                                      int& outSampleRate,
                                      int& outChannels);

    // Helper: Convert dB to linear
    float dbToLinear(float db);

    // Helper: Convert linear to dB
    float linearToDb(float linear);
};

} // namespace testing
} // namespace audioBridge

#endif // AUDIO_VALIDATOR_H
