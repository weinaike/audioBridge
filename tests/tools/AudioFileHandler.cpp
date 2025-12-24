/**
 * AudioFileHandler.cpp
 *
 * Implementation of audio file handling utilities
 */

#include "AudioFileHandler.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cstring>

// WAV header structure
#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Size of fmt chunk
    uint16_t audioFormat;   // Audio format (1 = PCM)
    uint16_t numChannels;   // Number of channels
    uint32_t sampleRate;    // Sample rate
    uint32_t byteRate;      // Byte rate
    uint16_t blockAlign;    // Block align
    uint16_t bitsPerSample; // Bits per sample
};
#pragma pack(pop)

namespace audioBridge {
namespace testing {

AudioFileHandler::AudioFileHandler() {
    spdlog::debug("AudioFileHandler initialized");
}

AudioFileHandler::~AudioFileHandler() {
    spdlog::debug("AudioFileHandler destroyed");
}

bool AudioFileHandler::loadFileInfo(const std::string& filepath, AudioFileInfo& outInfo) {
    if (!fileExists(filepath)) {
        return false;
    }

    std::string ext = getFileExtension(filepath);
    outInfo.filename = filepath;
    outInfo.format = ext;

    if (ext == "wav" || ext == "WAV") {
        return parseWavHeader(filepath, outInfo);
    } else if (ext == "flac" || ext == "FLAC") {
        return parseFlacHeader(filepath, outInfo);
    } else {
        lastError_ = "Unsupported format: " + ext;
        spdlog::error(lastError_);
        return false;
    }
}

bool AudioFileHandler::fileExists(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        lastError_ = "File does not exist or cannot be opened: " + filepath;
        spdlog::error(lastError_);
        return false;
    }
    return true;
}

std::string AudioFileHandler::getFileExtension(const std::string& filepath) {
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos != std::string::npos && dotPos < filepath.length() - 1) {
        return filepath.substr(dotPos + 1);
    }
    return "";
}

bool AudioFileHandler::isValidAudioFormat(const std::string& filepath) {
    std::string ext = getFileExtension(filepath);
    return (ext == "wav" || ext == "WAV" || ext == "flac" || ext == "FLAC");
}

bool AudioFileHandler::parseWavHeader(const std::string& filepath, AudioFileInfo& outInfo) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        lastError_ = "Cannot open WAV file";
        return false;
    }

    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

    if (!file) {
        lastError_ = "Failed to read WAV header";
        return false;
    }

    // Validate RIFF/WAVE format
    if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
        std::strncmp(header.wave, "WAVE", 4) != 0) {
        lastError_ = "Invalid WAV file format";
        return false;
    }

    // Extract metadata
    outInfo.channels = header.numChannels;
    outInfo.sampleRate = header.sampleRate;
    outInfo.bitsPerSample = header.bitsPerSample;

    // Calculate duration
    uint32_t dataSize = header.fileSize - 36; // Subtract header size
    uint32_t bytesPerSample = header.bitsPerSample / 8;
    uint32_t totalSamples = dataSize / (header.numChannels * bytesPerSample);
    outInfo.duration = static_cast<float>(totalSamples) / header.sampleRate;

    outInfo.dataSize = dataSize;

    spdlog::debug("WAV file: {} channels, {} Hz, {} bits, {:.2f} seconds",
                  outInfo.channels, outInfo.sampleRate, outInfo.bitsPerSample, outInfo.duration);

    return true;
}

bool AudioFileHandler::parseFlacHeader(const std::string& filepath, AudioFileInfo& outInfo) {
    // Placeholder: In production, use libsndfile for full FLAC support
    // For now, provide basic info

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        lastError_ = "Cannot open FLAC file";
        return false;
    }

    size_t fileSize = file.tellg();

    // Default FLAC values (would need proper parsing in production)
    outInfo.channels = 2;
    outInfo.sampleRate = 48000;
    outInfo.bitsPerSample = 16;
    outInfo.dataSize = fileSize;

    spdlog::warn("FLAC parsing is placeholder only - use libsndfile for production");
    return true;
}

} // namespace testing
} // namespace audioBridge
