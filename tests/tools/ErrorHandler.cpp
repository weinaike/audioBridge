/**
 * ErrorHandler.cpp
 *
 * Implementation of error handling
 */

#include "ErrorHandler.h"
#include <spdlog/spdlog.h>

namespace audioBridge {
namespace testing {

ErrorHandler::ErrorHandler()
    : category_(ErrorCategory::DEVICE_CONFIGURATION),
      hasError_(false) {
}

ErrorHandler::~ErrorHandler() {
}

void ErrorHandler::setError(ErrorCategory category, const std::string& message) {
    category_ = category;
    message_ = message;
    suggestion_ = getDefaultSuggestion(category);
    hasError_ = true;

    spdlog::error("Error [{}]: {}", static_cast<int>(category), message_);
}

std::string ErrorHandler::getError() const {
    if (!hasError_) {
        return "";
    }

    std::stringstream ss;
    ss << "Error: " << message_;

    if (!suggestion_.empty()) {
        ss << "\n\nSuggestion: " << suggestion_;
    }

    return ss.str();
}

std::string ErrorHandler::getSuggestion() const {
    return suggestion_;
}

void ErrorHandler::clear() {
    hasError_ = false;
    message_.clear();
    suggestion_.clear();
}

std::string ErrorHandler::formatError(ErrorCategory category,
                                       const std::string& message,
                                       const std::string& suggestion) {
    std::string defaultSuggestion = getDefaultSuggestion(category);
    std::string finalSuggestion = suggestion.empty() ? defaultSuggestion : suggestion;

    std::stringstream ss;
    ss << "Error: " << message;

    if (!finalSuggestion.empty()) {
        ss << "\n\nSuggestion: " << finalSuggestion;
    }

    return ss.str();
}

std::string ErrorHandler::getDefaultSuggestion(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::DEVICE_UNAVAILABLE:
            return "Ensure virtual audio device is configured. Run: sudo modprobe snd-aloop";

        case ErrorCategory::DEVICE_CONFIGURATION:
            return "Check device settings and ensure sample rate and channel count are supported";

        case ErrorCategory::FILE_NOT_FOUND:
            return "Verify the file path and ensure the file exists";

        case ErrorCategory::INVALID_AUDIO_FORMAT:
            return "Use supported audio formats: WAV, FLAC";

        case ErrorCategory::CAPTURE_ERROR:
            return "Check device permissions and ensure no other application is using the device";

        case ErrorCategory::PLAYBACK_ERROR:
            return "Verify the audio file is not corrupted and device is available";

        case ErrorCategory::VALIDATION_ERROR:
            return "Check test audio file integrity and expected parameters";

        case ErrorCategory::CONFIGURATION_ERROR:
            return "Verify configuration file format and required fields";

        case ErrorCategory::PERMISSION_DENIED:
            return "Ensure proper permissions for device access. May need to add user to 'audio' group";

        case ErrorCategory::INTERRUPTED:
            return "Test was interrupted by user";

        default:
            return "Check system logs for more details";
    }
}

} // namespace testing
} // namespace audioBridge
