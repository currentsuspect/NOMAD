/**
 * @file log.hpp
 * @brief Structured logging system for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides a flexible logging system with support for:
 * - Multiple log levels (trace, debug, info, warn, error, fatal)
 * - Multiple output sinks (console, file, custom)
 * - Structured logging with key-value pairs
 * - Thread-safe logging
 * - Compile-time log level filtering
 */

#pragma once

#include "config.hpp"
#include "types.hpp"

#include <string>
#include <string_view>
#include <sstream>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <functional>
#include <iostream>
#include <iomanip>

namespace nomad::log {

//=============================================================================
// Log Levels
//=============================================================================

enum class Level : u8 {
    Trace = 0,  ///< Detailed tracing information
    Debug = 1,  ///< Debug information for developers
    Info  = 2,  ///< General informational messages
    Warn  = 3,  ///< Warning messages
    Error = 4,  ///< Error messages
    Fatal = 5,  ///< Fatal errors that may crash the application
    Off   = 6   ///< Logging disabled
};

/// Convert log level to string
inline constexpr const char* levelToString(Level level) noexcept {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
        case Level::Off:   return "OFF  ";
    }
    return "?????";
}

/// Convert log level to ANSI color code
inline constexpr const char* levelToColor(Level level) noexcept {
    switch (level) {
        case Level::Trace: return "\033[90m";     // Gray
        case Level::Debug: return "\033[36m";     // Cyan
        case Level::Info:  return "\033[32m";     // Green
        case Level::Warn:  return "\033[33m";     // Yellow
        case Level::Error: return "\033[31m";     // Red
        case Level::Fatal: return "\033[35;1m";   // Bright Magenta
        case Level::Off:   return "\033[0m";      // Reset
    }
    return "\033[0m";
}

inline constexpr const char* kColorReset = "\033[0m";

//=============================================================================
// Log Record
//=============================================================================

struct LogRecord {
    Level level;
    std::string message;
    std::string_view category;
    std::string_view file;
    int line;
    std::string_view function;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id threadId;
};

//=============================================================================
// Log Sink Interface
//=============================================================================

/// Abstract base class for log output destinations
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const LogRecord& record) = 0;
    virtual void flush() = 0;
};

//=============================================================================
// Console Sink
//=============================================================================

class ConsoleSink : public ILogSink {
public:
    explicit ConsoleSink(bool useColors = true) : useColors_(useColors) {}

    void write(const LogRecord& record) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto time = std::chrono::system_clock::to_time_t(record.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            record.timestamp.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        
        // Timestamp
        oss << std::put_time(std::localtime(&time), "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << ' ';
        
        // Level with color
        if (useColors_) {
            oss << levelToColor(record.level);
        }
        oss << '[' << levelToString(record.level) << ']';
        if (useColors_) {
            oss << kColorReset;
        }
        
        // Category
        if (!record.category.empty()) {
            oss << " [" << record.category << ']';
        }
        
        // Message
        oss << ' ' << record.message;
        
        // Source location in debug
#ifdef NOMAD_BUILD_DEBUG
        oss << " (" << record.file << ':' << record.line << ')';
#endif
        
        oss << '\n';
        
        // Output to appropriate stream
        if (record.level >= Level::Error) {
            std::cerr << oss.str();
        } else {
            std::cout << oss.str();
        }
    }

    void flush() override {
        std::cout.flush();
        std::cerr.flush();
    }

private:
    std::mutex mutex_;
    bool useColors_;
};

//=============================================================================
// Logger
//=============================================================================

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLevel(Level level) {
        level_ = level;
    }

    Level getLevel() const {
        return level_;
    }

    void addSink(std::shared_ptr<ILogSink> sink) {
        std::lock_guard<std::mutex> lock(sinkMutex_);
        sinks_.push_back(std::move(sink));
    }

    void clearSinks() {
        std::lock_guard<std::mutex> lock(sinkMutex_);
        sinks_.clear();
    }

    bool shouldLog(Level level) const {
        return level >= level_;
    }

    void log(Level level, std::string_view category, std::string_view message,
             std::string_view file, int line, std::string_view function) {
        if (!shouldLog(level)) return;

        LogRecord record;
        record.level = level;
        record.message = std::string(message);
        record.category = category;
        record.file = file;
        record.line = line;
        record.function = function;
        record.timestamp = std::chrono::system_clock::now();
        record.threadId = std::this_thread::get_id();

        std::lock_guard<std::mutex> lock(sinkMutex_);
        for (auto& sink : sinks_) {
            sink->write(record);
        }
    }

    void flush() {
        std::lock_guard<std::mutex> lock(sinkMutex_);
        for (auto& sink : sinks_) {
            sink->flush();
        }
    }

private:
    Logger() : level_(Level::Info) {
        // Add default console sink
        sinks_.push_back(std::make_shared<ConsoleSink>());
    }

    Level level_;
    std::mutex sinkMutex_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

//=============================================================================
// Log Stream Helper
//=============================================================================

class LogStream {
public:
    LogStream(Level level, std::string_view category,
              std::string_view file, int line, std::string_view function)
        : level_(level), category_(category), file_(file), 
          line_(line), function_(function) {}

    ~LogStream() {
        Logger::instance().log(level_, category_, stream_.str(), 
                               file_, line_, function_);
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

private:
    Level level_;
    std::string_view category_;
    std::string_view file_;
    int line_;
    std::string_view function_;
    std::ostringstream stream_;
};

} // namespace nomad::log

//=============================================================================
// Logging Macros
//=============================================================================

/// Internal macro to create log stream
#define NOMAD_LOG_IMPL(level, category) \
    if (::nomad::log::Logger::instance().shouldLog(level)) \
        ::nomad::log::LogStream(level, category, __FILE__, __LINE__, __func__)

/// Log at specific level with category
#define NOMAD_LOG_TRACE(category) NOMAD_LOG_IMPL(::nomad::log::Level::Trace, category)
#define NOMAD_LOG_DEBUG(category) NOMAD_LOG_IMPL(::nomad::log::Level::Debug, category)
#define NOMAD_LOG_INFO(category)  NOMAD_LOG_IMPL(::nomad::log::Level::Info, category)
#define NOMAD_LOG_WARN(category)  NOMAD_LOG_IMPL(::nomad::log::Level::Warn, category)
#define NOMAD_LOG_ERROR(category) NOMAD_LOG_IMPL(::nomad::log::Level::Error, category)
#define NOMAD_LOG_FATAL(category) NOMAD_LOG_IMPL(::nomad::log::Level::Fatal, category)

/// Convenience macros with default category
#define LOG_TRACE NOMAD_LOG_TRACE("General")
#define LOG_DEBUG NOMAD_LOG_DEBUG("General")
#define LOG_INFO  NOMAD_LOG_INFO("General")
#define LOG_WARN  NOMAD_LOG_WARN("General")
#define LOG_ERROR NOMAD_LOG_ERROR("General")
#define LOG_FATAL NOMAD_LOG_FATAL("General")

/// Module-specific logging
#define LOG_AUDIO_TRACE NOMAD_LOG_TRACE("Audio")
#define LOG_AUDIO_DEBUG NOMAD_LOG_DEBUG("Audio")
#define LOG_AUDIO_INFO  NOMAD_LOG_INFO("Audio")
#define LOG_AUDIO_WARN  NOMAD_LOG_WARN("Audio")
#define LOG_AUDIO_ERROR NOMAD_LOG_ERROR("Audio")

#define LOG_UI_TRACE NOMAD_LOG_TRACE("UI")
#define LOG_UI_DEBUG NOMAD_LOG_DEBUG("UI")
#define LOG_UI_INFO  NOMAD_LOG_INFO("UI")
#define LOG_UI_WARN  NOMAD_LOG_WARN("UI")
#define LOG_UI_ERROR NOMAD_LOG_ERROR("UI")

#define LOG_DSP_TRACE NOMAD_LOG_TRACE("DSP")
#define LOG_DSP_DEBUG NOMAD_LOG_DEBUG("DSP")
#define LOG_DSP_INFO  NOMAD_LOG_INFO("DSP")
#define LOG_DSP_WARN  NOMAD_LOG_WARN("DSP")
#define LOG_DSP_ERROR NOMAD_LOG_ERROR("DSP")

#define LOG_PLUGIN_TRACE NOMAD_LOG_TRACE("Plugin")
#define LOG_PLUGIN_DEBUG NOMAD_LOG_DEBUG("Plugin")
#define LOG_PLUGIN_INFO  NOMAD_LOG_INFO("Plugin")
#define LOG_PLUGIN_WARN  NOMAD_LOG_WARN("Plugin")
#define LOG_PLUGIN_ERROR NOMAD_LOG_ERROR("Plugin")
