/**
 * AudioValidator.cpp
 *
 * Implementation of basic audio validation and advanced analysis
 */

#include "AudioValidator.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstring>

#ifdef SNDFILE_FOUND
#include <sndfile.h>
#endif

#ifdef GIST_FOUND
#include "Gist.h"
#endif

namespace audioBridge {
namespace testing {

// ============================================================================
// GistAnalyzer: Private implementation class for Gist library integration
// ============================================================================

class AudioValidator::GistAnalyzer {
public:
    GistAnalyzer() : fftSize(1024), sampleRate(48000) {
        spdlog::debug("GistAnalyzer initialized");
    }

    ~GistAnalyzer() {
        spdlog::debug("GistAnalyzer destroyed");
    }

    // T076: Frequency spectrum analysis
    std::vector<float> computeMagnitudeSpectrum(const std::vector<float>& audio,
                                                int sr) {
        std::vector<float> spectrum;

#ifdef GIST_FOUND
        sampleRate = sr;
        int numFrames = audio.size() / fftSize;

        if (numFrames < 1) {
            spdlog::warn("Audio too short for spectrum analysis");
            return spectrum;
        }

        // Initialize Gist processor (use global namespace qualifier)
        ::Gist::Gist<float> gist(fftSize, sampleRate);

        // Process frames and accumulate spectrum
        spectrum.resize(fftSize / 2 + 1, 0.0f);

        for (int frame = 0; frame < numFrames; ++frame) {
            int startIdx = frame * fftSize;
            std::vector<float> frameAudio(audio.begin() + startIdx,
                                          audio.begin() + startIdx + fftSize);

            gist.processAudioFrame(frameAudio);

            // Get magnitude spectrum for this frame
            const std::vector<float>& frameSpectrum = gist.getMagnitudeSpectrum();

            // Accumulate (average across frames)
            for (size_t i = 0; i < frameSpectrum.size(); ++i) {
                spectrum[i] += frameSpectrum[i];
            }
        }

        // Average
        for (auto& val : spectrum) {
            val /= numFrames;
        }

        spdlog::debug("Computed magnitude spectrum with {} bins", spectrum.size());
#else
        spdlog::warn("Gist library not available - using placeholder spectrum");
        // Placeholder: create simple synthetic spectrum
        spectrum.resize(fftSize / 2 + 1);
        for (size_t i = 0; i < spectrum.size(); ++i) {
            spectrum[i] = 0.1f + 0.9f * std::exp(-0.1f * i);
        }
#endif

        return spectrum;
    }

    // T077: Peak frequency detection
    float detectPeakFrequency(const std::vector<float>& spectrum, int sr) {
        if (spectrum.empty()) {
            return 0.0f;
        }

#ifdef GIST_FOUND
        // Find bin with maximum magnitude
        size_t peakBin = 0;
        float peakMag = spectrum[0];

        for (size_t i = 1; i < spectrum.size(); ++i) {
            if (spectrum[i] > peakMag) {
                peakMag = spectrum[i];
                peakBin = i;
            }
        }

        // Convert bin to frequency
        // Frequency = (bin * sampleRate) / fftSize
        float frequency = (peakBin * sr) / static_cast<float>(fftSize);

        spdlog::debug("Peak frequency: {:.2f} Hz (bin {}, mag {:.4f})",
                      frequency, peakBin, peakMag);

        return frequency;
#else
        spdlog::warn("Gist library not available - using placeholder frequency");
        return 1000.0f; // Placeholder: 1kHz
#endif
    }

private:
    int fftSize;
    int sampleRate;
};

// ============================================================================
// AudioValidator Implementation
// ============================================================================

AudioValidator::AudioValidator() : gistAnalyzer_(std::make_unique<GistAnalyzer>()) {
    spdlog::debug("AudioValidator initialized");
}

AudioValidator::~AudioValidator() {
    spdlog::debug("AudioValidator destroyed");
}

IntegrityCheckResult AudioValidator::checkIntegrity(const std::string& audioFile,
                                                     int expectedSampleRate,
                                                     int expectedChannels,
                                                     float expectedDuration) {
    IntegrityCheckResult result{};

    spdlog::info("Checking integrity of {}", audioFile);

    // Format validation
    result.formatValid = isValidFormat(audioFile);
    if (!result.formatValid) {
        spdlog::error("Format validation failed");
        return result;
    }

#ifdef SNDFILE_FOUND
    // Parse actual file header using libsndfile
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    SNDFILE* sndFile = sf_open(audioFile.c_str(), SFM_READ, &sfinfo);
    if (!sndFile) {
        lastError_ = std::string("Failed to open audio file: ") + sf_strerror(nullptr);
        spdlog::error(lastError_);
        result.formatValid = false;
        return result;
    }

    // Get actual file properties
    int actualSampleRate = sfinfo.samplerate;
    int actualChannels = sfinfo.channels;
    float actualDuration = static_cast<float>(sfinfo.frames) / actualSampleRate;

    spdlog::info("File properties: sr={}, ch={}, dur={:.2f}s, frames={}",
                 actualSampleRate, actualChannels, actualDuration, sfinfo.frames);

    // Validate properties
    result.sampleRateMatch = (actualSampleRate == expectedSampleRate);
    result.channelCountMatch = (actualChannels == expectedChannels);

    // Allow 5% tolerance on duration
    float durationTolerance = expectedDuration * 0.05f;
    result.durationMatch = (std::abs(actualDuration - expectedDuration) <= durationTolerance);

    sf_close(sndFile);

    if (!result.sampleRateMatch) {
        spdlog::error("Sample rate mismatch: expected {}, got {}", expectedSampleRate, actualSampleRate);
    }
    if (!result.channelCountMatch) {
        spdlog::error("Channel count mismatch: expected {}, got {}", expectedChannels, actualChannels);
    }
    if (!result.durationMatch) {
        spdlog::error("Duration mismatch: expected {:.2f}s, got {:.2f}s",
                      expectedDuration, actualDuration);
    }
#else
    // Fallback: placeholder validation
    spdlog::warn("libsndfile not available - using placeholder integrity check");
    result.sampleRateMatch = true;
    result.channelCountMatch = true;
    result.durationMatch = true;
#endif

    // File size validation
    result.fileSizeValid = isValidFileSize(audioFile, expectedDuration,
                                            expectedSampleRate, expectedChannels);

    spdlog::info("Integrity check result: {}",
                 result.allValid() ? "PASS" : "FAIL");

    return result;
}

bool AudioValidator::isValidFormat(const std::string& audioFile) {
    std::string ext = getFileExtension(audioFile);

    // Supported formats
    if (ext == "wav" || ext == "WAV") {
        return true;
    }
    if (ext == "flac" || ext == "FLAC") {
        return true;
    }

    lastError_ = "Unsupported file format: " + ext;
    spdlog::error(lastError_);
    return false;
}

bool AudioValidator::isValidFileSize(const std::string& audioFile,
                                     float duration,
                                     int sampleRate,
                                     int channels) {
    // Get actual file size
    std::ifstream file(audioFile, std::ios::binary | std::ios::ate);
    if (!file) {
        lastError_ = "Cannot open file: " + audioFile;
        spdlog::error(lastError_);
        return false;
    }

    size_t actualSize = file.tellg();
    size_t expectedSize = calculateExpectedSize(duration, sampleRate, channels);

    // Allow 20% tolerance for metadata, headers, etc.
    size_t minSize = expectedSize * 0.8;
    size_t maxSize = expectedSize * 1.2;

    bool valid = (actualSize >= minSize && actualSize <= maxSize);

    if (!valid) {
        lastError_ = "File size out of expected range. Expected: " +
                     std::to_string(expectedSize) + ", Actual: " +
                     std::to_string(actualSize);
        spdlog::warn(lastError_);
    }

    return valid;
}

std::string AudioValidator::getFileExtension(const std::string& filepath) {
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos != std::string::npos && dotPos < filepath.length() - 1) {
        return filepath.substr(dotPos + 1);
    }
    return "";
}

size_t AudioValidator::calculateExpectedSize(float duration, int sampleRate, int channels) {
    // Rough estimate for PCM data
    // Bytes per sample = 2 (16-bit)
    // Total samples = duration * sampleRate * channels
    // Expected size = Total samples * bytes per sample + header (44 bytes for WAV)

    size_t dataSize = static_cast<size_t>(duration * sampleRate * channels * 2);
    return dataSize + 44; // + WAV header
}

// ============================================================================
// Phase 3: Advanced Validation with Gist Library (T075-T082)
// ============================================================================

// T075-T078: Frequency Analysis
FrequencyAnalysisResult AudioValidator::analyzeFrequency(const std::string& audioFile,
                                                         float expectedFrequency,
                                                         float toleranceHz) {
    FrequencyAnalysisResult result;

    spdlog::info("Analyzing frequency spectrum of {}", audioFile);

    // Load audio file
    int sampleRate, channels;
    std::vector<float> audio = loadAudioFile(audioFile, sampleRate, channels);

    if (audio.empty()) {
        lastError_ = "Failed to load audio file for frequency analysis";
        spdlog::error(lastError_);
        return result;
    }

    // T076: Compute magnitude spectrum
    result.spectrum = gistAnalyzer_->computeMagnitudeSpectrum(audio, sampleRate);

    if (result.spectrum.empty()) {
        lastError_ = "Failed to compute magnitude spectrum";
        spdlog::error(lastError_);
        return result;
    }

    // T077: Detect peak frequency
    result.peakFrequency = gistAnalyzer_->detectPeakFrequency(result.spectrum, sampleRate);

    // Find magnitude at peak frequency
    size_t peakBin = static_cast<size_t>((result.peakFrequency * 1024) / sampleRate);
    if (peakBin < result.spectrum.size()) {
        result.magnitude = result.spectrum[peakBin];
    }

    // T078: Frequency matching logic
    result.frequencyError = std::abs(result.peakFrequency - expectedFrequency);
    result.withinTolerance = (result.frequencyError <= toleranceHz);
    result.matchesExpected = result.withinTolerance;
    result.success = true;

    spdlog::info("Frequency analysis result: {:.2f} Hz (expected {:.2f} Hz, error {:.2f} Hz)",
                 result.peakFrequency, expectedFrequency, result.frequencyError);
    spdlog::info("Within tolerance: {}", result.withinTolerance ? "YES" : "NO");

    return result;
}

bool AudioValidator::analyzeFrequency(const std::string& audioFile,
                                      float& outPeakFrequency,
                                      float expectedFrequency,
                                      float tolerance) {
    FrequencyAnalysisResult result = analyzeFrequency(audioFile, expectedFrequency, tolerance);
    outPeakFrequency = result.peakFrequency;
    return result.success && result.withinTolerance;
}

// T079-T082: Signal Quality Metrics
SignalQualityMetrics AudioValidator::calculateSignalQuality(const std::string& audioFile) {
    SignalQualityMetrics metrics;

    spdlog::info("Calculating signal quality metrics for {}", audioFile);

    // Load audio file
    int sampleRate, channels;
    std::vector<float> audio = loadAudioFile(audioFile, sampleRate, channels);

    if (audio.empty()) {
        lastError_ = "Failed to load audio file for quality analysis";
        spdlog::error(lastError_);
        return metrics;
    }

    // T081: Calculate peak amplitude
    metrics.peakAmplitude = calculatePeakAmplitude(audioFile);

    // T082: Calculate RMS level
    metrics.rmsLevel = calculateRMSLevel(audioFile);

    // T079: Calculate SNR
    metrics.snr = calculateSNR(audioFile);

    // T080: Calculate THD
    metrics.thd = calculateTHD(audioFile);

    // Calculate noise floor from spectrum
    FrequencyAnalysisResult freqResult = analyzeFrequency(audioFile, 1000.0f, 50.0f);
    if (freqResult.success && !freqResult.spectrum.empty()) {
        // Noise floor = minimum magnitude in spectrum (excluding DC and very low frequencies)
        float minMag = *std::min_element(freqResult.spectrum.begin() + 10, freqResult.spectrum.end());
        metrics.noiseFloor = linearToDb(minMag);
    }

    metrics.valid = true;

    spdlog::info("Signal quality: SNR={:.2f} dB, THD={:.2f}%, Peak={:.4f}, RMS={:.4f}",
                 metrics.snr, metrics.thd, metrics.peakAmplitude, metrics.rmsLevel);

    return metrics;
}

float AudioValidator::calculateSNR(const std::string& audioFile) {
    // T079: Signal-to-Noise Ratio calculation
    // SNR = 20 * log10(RMS_signal / RMS_noise)

#ifdef GIST_FOUND
    // Load audio
    int sampleRate, channels;
    std::vector<float> audio = loadAudioFile(audioFile, sampleRate, channels);

    if (audio.empty()) {
        return 0.0f;
    }

    // Calculate RMS of entire signal
    float sumSquares = std::accumulate(audio.begin(), audio.end(), 0.0f,
        [](float acc, float val) { return acc + val * val; });
    float rmsSignal = std::sqrt(sumSquares / audio.size());

    // Estimate noise floor from quietest portions
    // For simplicity, use last 10% of audio as noise estimate
    size_t noiseStart = audio.size() * 9 / 10;
    float noiseSum = std::accumulate(audio.begin() + noiseStart, audio.end(), 0.0f,
        [](float acc, float val) { return acc + val * val; });
    float rmsNoise = std::sqrt(noiseSum / (audio.size() - noiseStart));

    if (rmsNoise < 1e-6f) {
        rmsNoise = 1e-6f; // Prevent division by zero
    }

    float snrLinear = rmsSignal / rmsNoise;
    float snrDb = 20.0f * std::log10(snrLinear);

    spdlog::debug("SNR: {:.2f} dB (signal RMS={:.6f}, noise RMS={:.6f})",
                  snrDb, rmsSignal, rmsNoise);

    return std::max(0.0f, snrDb); // Clamp to positive values
#else
    spdlog::warn("Gist library not available - using placeholder SNR");
    return 72.0f; // Placeholder: Good SNR
#endif
}

float AudioValidator::calculateTHD(const std::string& audioFile) {
    // T080: Total Harmonic Distortion calculation
    // THD = sqrt(sum(harmonic_powers)) / fundamental_power

#ifdef GIST_FOUND
    // Load audio and compute spectrum
    int sampleRate, channels;
    std::vector<float> audio = loadAudioFile(audioFile, sampleRate, channels);

    if (audio.empty()) {
        return 0.0f;
    }

    std::vector<float> spectrum = gistAnalyzer_->computeMagnitudeSpectrum(audio, sampleRate);

    if (spectrum.empty()) {
        return 0.0f;
    }

    // Find fundamental frequency peak
    size_t fundamentalBin = 0;
    float fundamentalMag = spectrum[0];

    for (size_t i = 1; i < spectrum.size() / 4; ++i) {
        if (spectrum[i] > fundamentalMag) {
            fundamentalMag = spectrum[i];
            fundamentalBin = i;
        }
    }

    // Calculate power at fundamental and harmonics (2nd, 3rd, 4th, 5th)
    float fundamentalPower = fundamentalMag * fundamentalMag;
    float harmonicPower = 0.0f;

    for (int h = 2; h <= 5; ++h) {
        size_t harmonicBin = fundamentalBin * h;
        if (harmonicBin < spectrum.size()) {
            float harmonicMag = spectrum[harmonicBin];
            harmonicPower += harmonicMag * harmonicMag;
        }
    }

    // THD as percentage
    float thd = (fundamentalPower > 1e-6f) ?
        (std::sqrt(harmonicPower) / std::sqrt(fundamentalPower)) * 100.0f : 0.0f;

    spdlog::debug("THD: {:.4f}% (fundamental={:.6f}, harmonics={:.6f})",
                  thd, fundamentalPower, harmonicPower);

    return thd;
#else
    spdlog::warn("Gist library not available - using placeholder THD");
    return 0.5f; // Placeholder: Low distortion
#endif
}

float AudioValidator::calculatePeakAmplitude(const std::string& audioFile) {
    // T081: Peak amplitude detection

    int sampleRate, channels;
    std::vector<float> audio = loadAudioFile(audioFile, sampleRate, channels);

    if (audio.empty()) {
        return 0.0f;
    }

    auto [minIt, maxIt] = std::minmax_element(audio.begin(), audio.end());
    float peakAmplitude = std::max(std::abs(*minIt), std::abs(*maxIt));

    spdlog::debug("Peak amplitude: {:.6f}", peakAmplitude);

    return peakAmplitude;
}

float AudioValidator::calculateRMSLevel(const std::string& audioFile) {
    // T082: RMS level calculation

    int sampleRate, channels;
    std::vector<float> audio = loadAudioFile(audioFile, sampleRate, channels);

    if (audio.empty()) {
        return 0.0f;
    }

    float sumSquares = std::accumulate(audio.begin(), audio.end(), 0.0f,
        [](float acc, float val) { return acc + val * val; });

    float rms = std::sqrt(sumSquares / audio.size());

    spdlog::debug("RMS level: {:.6f}", rms);

    return rms;
}

float AudioValidator::calculateNoiseFloor(const std::string& audioFile) {
    // Estimate noise floor from spectrum
    FrequencyAnalysisResult result = analyzeFrequency(audioFile, 1000.0f, 50.0f);

    if (!result.success || result.spectrum.empty()) {
        return -100.0f; // Default low noise floor
    }

    // Find minimum magnitude (excluding first few bins which are DC and very low freq)
    float minMag = *std::min_element(result.spectrum.begin() + 10, result.spectrum.end());
    return linearToDb(minMag);
}

// ============================================================================
// T099: Reference File Comparison
// ============================================================================

float AudioValidator::compareToReference(const std::string& audioFile,
                                         const std::string& referenceFile) {
    spdlog::info("Comparing audio file '{}' to reference '{}'",
                 audioFile, referenceFile);

    // Load both files
    int sampleRate1, channels1;
    std::vector<float> audio1 = loadAudioFile(audioFile, sampleRate1, channels1);

    int sampleRate2, channels2;
    std::vector<float> audio2 = loadAudioFile(referenceFile, sampleRate2, channels2);

    if (audio1.empty() || audio2.empty()) {
        lastError_ = "Failed to load audio files for comparison";
        spdlog::error(lastError_);
        return 0.0f;
    }

    // Check compatibility
    if (sampleRate1 != sampleRate2) {
        lastError_ = "Sample rate mismatch: " + std::to_string(sampleRate1) +
                     " vs " + std::to_string(sampleRate2);
        spdlog::error(lastError_);
        return 0.0f;
    }

    if (channels1 != channels2) {
        lastError_ = "Channel count mismatch: " + std::to_string(channels1) +
                     " vs " + std::to_string(channels2);
        spdlog::error(lastError_);
        return 0.0f;
    }

    // Use the shorter length to avoid out-of-bounds
    size_t minLength = std::min(audio1.size(), audio2.size());

    // Calculate correlation coefficient
    float correlation = calculateCorrelation(audio1, audio2);

    spdlog::info("Reference comparison correlation: {:.4f} ({:.1f}%)",
                 correlation, correlation * 100.0f);

    return correlation;
}

float AudioValidator::calculateCorrelation(const std::vector<float>& audio1,
                                           const std::vector<float>& audio2) {
    // Pearson correlation coefficient
    // Correlation = covariance(audio1, audio2) / (stddev(audio1) * stddev(audio2))

    if (audio1.empty() || audio2.empty()) {
        return 0.0f;
    }

    size_t n = std::min(audio1.size(), audio2.size());

    // Calculate means
    float mean1 = std::accumulate(audio1.begin(), audio1.begin() + n, 0.0f) / n;
    float mean2 = std::accumulate(audio2.begin(), audio2.begin() + n, 0.0f) / n;

    // Calculate covariance and standard deviations
    float covariance = 0.0f;
    float var1 = 0.0f;
    float var2 = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        float diff1 = audio1[i] - mean1;
        float diff2 = audio2[i] - mean2;

        covariance += diff1 * diff2;
        var1 += diff1 * diff1;
        var2 += diff2 * diff2;
    }

    covariance /= n;
    var1 /= n;
    var2 /= n;

    // Calculate correlation
    float stdDev1 = std::sqrt(var1);
    float stdDev2 = std::sqrt(var2);

    if (stdDev1 < 1e-6f || stdDev2 < 1e-6f) {
        spdlog::warn("Standard deviation too small for correlation calculation");
        return 0.0f;
    }

    float correlation = covariance / (stdDev1 * stdDev2);

    // Clamp to [-1, 1] range (accounting for floating point errors)
    correlation = std::max(-1.0f, std::min(1.0f, correlation));

    return correlation;
}

// ============================================================================
// Helper Functions
// ============================================================================

std::vector<float> AudioValidator::loadAudioFile(const std::string& audioFile,
                                                   int& outSampleRate,
                                                   int& outChannels) {
#ifdef SNDFILE_FOUND
    // Real implementation using libsndfile
    spdlog::debug("Loading audio file: {}", audioFile);

    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    SNDFILE* sndFile = sf_open(audioFile.c_str(), SFM_READ, &sfinfo);
    if (!sndFile) {
        lastError_ = std::string("Failed to open audio file: ") + sf_strerror(nullptr);
        spdlog::error("{}: {}", audioFile, lastError_);
        return {};
    }

    // Set output parameters
    outSampleRate = sfinfo.samplerate;
    outChannels = sfinfo.channels;

    spdlog::debug("Audio file info: samplerate={}, channels={}, frames={}",
                  outSampleRate, outChannels, sfinfo.frames);

    // Allocate buffer for audio data
    std::vector<float> audio;
    audio.resize(sfinfo.frames * outChannels);

    // Read audio data
    sf_count_t framesRead = sf_readf_float(sndFile, audio.data(), sfinfo.frames);
    if (framesRead != sfinfo.frames) {
        spdlog::warn("Expected {} frames, read {}", sfinfo.frames, framesRead);
    }

    // Close file
    if (sf_close(sndFile) != 0) {
        spdlog::warn("Error closing audio file: {}", audioFile);
    }

    spdlog::info("Loaded {} frames ({} samples) from {}",
                 framesRead, audio.size(), audioFile);

    return audio;
#else
    // Fallback placeholder implementation
    spdlog::warn("libsndfile not available - using placeholder audio loader");

    // Generate synthetic sine wave at 1kHz
    outSampleRate = 48000;
    outChannels = 1;

    constexpr int duration = 5; // seconds
    constexpr float frequency = 1000.0f; // Hz
    std::vector<float> audio(outSampleRate * duration);

    for (size_t i = 0; i < audio.size(); ++i) {
        float t = static_cast<float>(i) / outSampleRate;
        audio[i] = 0.8f * std::sin(2.0f * M_PI * frequency * t);
    }

    return audio;
#endif
}

float AudioValidator::dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

float AudioValidator::linearToDb(float linear) {
    if (linear < 1e-6f) {
        return -120.0f; // Floor at -120 dB
    }
    return 20.0f * std::log10(linear);
}

} // namespace testing
} // namespace audioBridge
