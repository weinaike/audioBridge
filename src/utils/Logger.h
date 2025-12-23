#ifndef AUDIOBRIDGE_LOGGER_H
#define AUDIOBRIDGE_LOGGER_H

#include <string>
#include <memory>

namespace audiobridge {

/// Logger severity levels
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

/// RT-safe async logger wrapper using spdlog
/// Designed for real-time audio applications
class Logger {
public:
    /// Get singleton instance
    static Logger& GetInstance();

    /// Initialize logger (call before using)
    /// @param logFilePath Optional file path for logging
    /// @param level Minimum log level
    void Initialize(const std::string& logFilePath = "", LogLevel level = LogLevel::Info);

    /// Log a message
    /// @param level Log level
    /// @param message Message to log
    void Log(LogLevel level, const std::string& message);

    /// Convenience methods for different log levels
    void Trace(const std::string& message);
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Critical(const std::string& message);

    /// Flush any pending log messages
    void Flush();

    /// Reset logger state (for testing purposes)
    /// Clears all sinks and resets initialization state
    void Reset();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

}  // namespace audiobridge

// Convenience macros for logging
#define AB_LOG_TRACE(msg)    audiobridge::Logger::GetInstance().Trace(msg)
#define AB_LOG_DEBUG(msg)    audiobridge::Logger::GetInstance().Debug(msg)
#define AB_LOG_INFO(msg)     audiobridge::Logger::GetInstance().Info(msg)
#define AB_LOG_WARNING(msg)  audiobridge::Logger::GetInstance().Warning(msg)
#define AB_LOG_ERROR(msg)    audiobridge::Logger::GetInstance().Error(msg)
#define AB_LOG_CRITICAL(msg) audiobridge::Logger::GetInstance().Critical(msg)

#endif  // AUDIOBRIDGE_LOGGER_H
