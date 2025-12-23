#include "utils/Logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>

namespace audiobridge {

// PIMPL implementation class
class Logger::Impl {
public:
    std::shared_ptr<spdlog::logger> logger_;
    bool initialized_ = false;

    ~Impl() = default;
};

Logger::~Logger() = default;

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::Initialize(const std::string& logFilePath, LogLevel level) {
    if (pImpl_ && pImpl_->initialized_) {
        return;  // Already initialized
    }

    if (!pImpl_) {
        pImpl_ = std::make_unique<Impl>();
    }

    try {
        std::vector<spdlog::sink_ptr> sinks;

        // Console sink with colors
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        sinks.push_back(console_sink);

        // File sink (optional)
        if (!logFilePath.empty()) {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                logFilePath, true);
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            sinks.push_back(file_sink);
        }

        // Create multi-sink logger
        pImpl_->logger_ = std::make_shared<spdlog::logger>("audioBridge",
            begin(sinks), end(sinks));
        pImpl_->logger_->set_level(spdlog::level::trace);
        pImpl_->logger_->flush_on(spdlog::level::warn);

        // Set user-defined log level
        switch (level) {
            case LogLevel::Trace:
                pImpl_->logger_->set_level(spdlog::level::trace);
                break;
            case LogLevel::Debug:
                pImpl_->logger_->set_level(spdlog::level::debug);
                break;
            case LogLevel::Info:
                pImpl_->logger_->set_level(spdlog::level::info);
                break;
            case LogLevel::Warning:
                pImpl_->logger_->set_level(spdlog::level::warn);
                break;
            case LogLevel::Error:
                pImpl_->logger_->set_level(spdlog::level::err);
                break;
            case LogLevel::Critical:
                pImpl_->logger_->set_level(spdlog::level::critical);
                break;
        }

        pImpl_->initialized_ = true;

    } catch (const spdlog::spdlog_ex& ex) {
        // Fallback to stdout if spdlog fails
        fprintf(stderr, "Logger initialization failed: %s\n", ex.what());
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (!pImpl_ || !pImpl_->initialized_) {
        // Fallback to stderr if not initialized
        fprintf(stderr, "[NOT INITIALIZED] %s\n", message.c_str());
        return;
    }

    try {
        switch (level) {
            case LogLevel::Trace:
                pImpl_->logger_->trace(message);
                break;
            case LogLevel::Debug:
                pImpl_->logger_->debug(message);
                break;
            case LogLevel::Info:
                pImpl_->logger_->info(message);
                break;
            case LogLevel::Warning:
                pImpl_->logger_->warn(message);
                break;
            case LogLevel::Error:
                pImpl_->logger_->error(message);
                break;
            case LogLevel::Critical:
                pImpl_->logger_->critical(message);
                break;
        }
    } catch (const std::exception& ex) {
        fprintf(stderr, "Logger error: %s\n", ex.what());
    }
}

void Logger::Trace(const std::string& message) { Log(LogLevel::Trace, message); }
void Logger::Debug(const std::string& message) { Log(LogLevel::Debug, message); }
void Logger::Info(const std::string& message) { Log(LogLevel::Info, message); }
void Logger::Warning(const std::string& message) { Log(LogLevel::Warning, message); }
void Logger::Error(const std::string& message) { Log(LogLevel::Error, message); }
void Logger::Critical(const std::string& message) { Log(LogLevel::Critical, message); }

void Logger::Flush() {
    if (pImpl_ && pImpl_->initialized_) {
        pImpl_->logger_->flush();
    }
}

}  // namespace audiobridge
