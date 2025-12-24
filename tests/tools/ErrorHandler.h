/**
 * ErrorHandler.h
 *
 * Centralized error handling and user-friendly error messages
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <sstream>

namespace audioBridge {
namespace testing {

/**
 * Error categories
 */
enum class ErrorCategory {
    DEVICE_UNAVAILABLE,
    DEVICE_CONFIGURATION,
    FILE_NOT_FOUND,
    INVALID_AUDIO_FORMAT,
    CAPTURE_ERROR,
    PLAYBACK_ERROR,
    VALIDATION_ERROR,
    CONFIGURATION_ERROR,
    PERMISSION_DENIED,
    INTERRUPTED
};

/**
 * Error handler for consistent error reporting
 */
class ErrorHandler {
public:
    ErrorHandler();
    ~ErrorHandler();

    // Set error with category and message
    void setError(ErrorCategory category, const std::string& message);

    // Get formatted error message
    std::string getError() const;

    // Get last error category
    ErrorCategory getCategory() const { return category_; }

    // Get suggestion for fixing the error
    std::string getSuggestion() const;

    // Clear error
    void clear();

    // Check if has error
    bool hasError() const { return hasError_; }

    // Static helper: Format error with suggestion
    static std::string formatError(ErrorCategory category,
                                   const std::string& message,
                                   const std::string& suggestion = "");

private:
    ErrorCategory category_;
    std::string message_;
    std::string suggestion_;
    bool hasError_;

    // Get default suggestion for error category
    static std::string getDefaultSuggestion(ErrorCategory category);
};

} // namespace testing
} // namespace audioBridge

#endif // ERROR_HANDLER_H
