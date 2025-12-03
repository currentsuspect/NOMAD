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

/**
 * @brief Returns the uppercase short name for a log Level.
 *
 * @param level Log level to convert.
 * @return const char* Uppercase, fixed-width textual representation of the level (e.g. "TRACE", "INFO ", "ERROR").
 */
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

/**
 * @brief Maps a log level to its ANSI color escape code.
 *
 * @returns const char* ANSI escape sequence corresponding to the provided log level; returns the reset code for Level::Off or unknown values.
 */
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
    /**
 * @brief Ensures derived log sink objects are destroyed correctly through the interface.
 *
 * Provides a virtual destructor so implementations of ILogSink can be deleted via
 * a pointer or smart pointer to the base type without resource leaks.
 */
virtual ~ILogSink() = default;
    virtual void write(const LogRecord& record) = 0;
    virtual void flush() = 0;
};

//=============================================================================
// Console Sink
//=============================================================================

class ConsoleSink : public ILogSink {
public:
    /**
 * @brief Constructs a ConsoleSink with optional ANSI color output.
 *
 * @param useColors If `true`, console output (level labels and related text) is colorized using ANSI escape codes; if `false`, output is plain text without color codes.
 */
explicit ConsoleSink(bool useColors = true) : useColors_(useColors) {}

    /**
     * @brief Writes a formatted log record to the console sink.
     *
     * Formats the record with a time stamp (hours:minutes:seconds.milliseconds), level, optional category, and message, optionally including source location in debug builds, and outputs error-level records to stderr and others to stdout.
     *
     * @param record The log record to write; provides level, message, category, timestamp, and optional source location.
     *
     * @note Output may be colorized when the sink was constructed with colors enabled.
     */
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

    /**
     * @brief Flushes the console output streams used by the sink.
     *
     * Ensures both standard output (stdout) and standard error (stderr) are flushed.
     */
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
    /**
     * @brief Access the global Logger singleton.
     *
     * @return Logger& Reference to the process-wide Logger singleton.
     */
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    /**
     * @brief Sets the logger's minimum level; messages below this level will be ignored.
     *
     * @param level The new minimum log level; messages with a lower priority than this will not be emitted.
     */
    void setLevel(Level level) {
        level_ = level;
    }

    /**
     * @brief Retrieve the logger's current minimum log level.
     *
     * @return Level The current minimum log level used for filtering logged messages; messages with lower severity than this level are not emitted.
     */
    Level getLevel() const {
        return level_;
    }

    /**
     * @brief Adds a log sink to the logger and makes it available for future log dispatch.
     *
     * This operation is thread-safe and stores the provided sink in the logger's collection so
     * subsequent log records will be forwarded to it.
     *
     * @param sink Shared pointer to an ILogSink to register; ownership is shared with the logger.
     */
    void addSink(std::shared_ptr<ILogSink> sink) {
        std::lock_guard<std::mutex> lock(sinkMutex_);
        sinks_.push_back(std::move(sink));
    }

    /**
     * @brief Removes all registered log sinks from the global logger.
     *
     * This operation is thread-safe and acquires the sink mutex before clearing the
     * internal list of sinks.
     */
    void clearSinks() {
        std::lock_guard<std::mutex> lock(sinkMutex_);
        sinks_.clear();
    }

    /**
     * @brief Check whether messages at the given level will be emitted.
     *
     * @param level Log level to test against the logger's current minimum level.
     * @return `true` if `level` is greater than or equal to the logger's current minimum level, `false` otherwise.
     */
    bool shouldLog(Level level) const {
        return level >= level_;
    }

    /**
     * @brief Emits a log record at the specified level to all registered sinks.
     *
     * If the level is enabled, constructs a record containing level, category, message,
     * source location, timestamp, and thread id, then forwards it to every configured sink.
     *
     * @param level Log level for the message.
     * @param category Logical category or subsystem for the message.
     * @param message Log message text.
     * @param file Source file name to associate with the record.
     * @param line Source line number to associate with the record.
     * @param function Source function name to associate with the record.
     */
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

    /**
     * @brief Flushes all registered log sinks.
     *
     * Ensures any buffered log output held by each sink is written out. This operation
     * is performed under the logger's sink mutex to synchronize with concurrent sink
     * modifications and log dispatch.
     */
    void flush() {
        std::lock_guard<std::mutex> lock(sinkMutex_);
        for (auto& sink : sinks_) {
            sink->flush();
        }
    }

private:
    /**
     * @brief Initializes the Logger singleton with default settings.
     *
     * Constructs the Logger with the default minimum log level set to Info and registers
     * a default ConsoleSink so logging is available immediately.
     */
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
    /**
           * @brief Create a LogStream that collects a log message and its metadata for later emission.
           *
           * @param level Log level to associate with the emitted record.
           * @param category Semantic category for the message (e.g., "Audio", "UI", "General").
           * @param file Source file path to record with the message.
           * @param line Source line number to record with the message.
           * @param function Source function name to record with the message.
           */
          LogStream(Level level, std::string_view category,
              std::string_view file, int line, std::string_view function)
        : level_(level), category_(category), file_(file), 
          line_(line), function_(function) {}

    /**
     * @brief Emits the accumulated log message to the global Logger using the stored metadata.
     *
     * On destruction, forwards the stream contents along with the captured level, category,
     * and source location (file, line, function) to Logger::instance().log().
     */
    ~LogStream() {
        Logger::instance().log(level_, category_, stream_.str(), 
                               file_, line_, function_);
    }

    template<typename T>
    /**
     * @brief Appends a value to the internal message stream.
     *
     * @tparam T Type of the value to append; must be streamable to an output stream.
     * @param value The value to append to the accumulated log message.
     * @return LogStream& Reference to this LogStream to allow chained streaming.
     */
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