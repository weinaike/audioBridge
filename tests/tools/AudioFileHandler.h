/**
 * AudioFileHandler.h
 *
 * Utilities for loading and handling audio files
 */

#ifndef AUDIO_FILE_HANDLER_H
#define AUDIO_FILE_HANDLER_H

#include <string>
#include <vector>

namespace audioBridge {
namespace testing {

/**
 * Audio file metadata
 */
struct AudioFileInfo {
    std::string filename;
    std::string format;
    int sampleRate;
    int channels;
    int bitsPerSample;
    float duration;
    size_t dataSize;
};

/**
 * Audio file I/O handler
 */
class AudioFileHandler {
public:
    AudioFileHandler();
    ~AudioFileHandler();

    // Load audio file metadata
    bool loadFileInfo(const std::string& filepath, AudioFileInfo& outInfo);

    // Check if file exists and is readable
    bool fileExists(const std::string& filepath);

    // Get file extension
    static std::string getFileExtension(const std::string& filepath);

    // Validate audio file format
    static bool isValidAudioFormat(const std::string& filepath);

    // Get last error
    std::string getLastError() const { return lastError_; }

private:
    std::string lastError_;

    // Parse WAV header (basic implementation)
    bool parseWavHeader(const std::string& filepath, AudioFileInfo& outInfo);

    // Parse FLAC header (placeholder - use libsndfile in production)
    bool parseFlacHeader(const std::string& filepath, AudioFileInfo& outInfo);
};

} // namespace testing
} // namespace audioBridge

#endif // AUDIO_FILE_HANDLER_H
