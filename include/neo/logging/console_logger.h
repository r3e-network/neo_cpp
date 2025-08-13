/**
 * @file console_logger.h
 * @brief Console Logger for controlled output
 */

#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <mutex>

namespace neo {
namespace logging {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4,
    NONE = 5
};

class ConsoleLogger {
private:
    static LogLevel current_level_;
    static bool enable_console_;
    static std::mutex mutex_;

public:
    static void SetLevel(LogLevel level) {
        current_level_ = level;
    }

    static void EnableConsole(bool enable) {
        enable_console_ = enable;
    }

    static void Log(LogLevel level, const std::string& message) {
        if (level >= current_level_ && enable_console_) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            const char* prefix = "";
            switch (level) {
                case LogLevel::DEBUG: prefix = "[DEBUG] "; break;
                case LogLevel::INFO: prefix = "[INFO] "; break;
                case LogLevel::WARNING: prefix = "[WARN] "; break;
                case LogLevel::ERROR: prefix = "[ERROR] "; break;
                case LogLevel::CRITICAL: prefix = "[CRITICAL] "; break;
                default: break;
            }
            
            std::cerr << prefix << message << std::endl;
        }
    }

    static void Info(const std::string& message) {
        Log(LogLevel::INFO, message);
    }

    static void Debug(const std::string& message) {
        Log(LogLevel::DEBUG, message);
    }

    static void Warning(const std::string& message) {
        Log(LogLevel::WARNING, message);
    }

    static void Error(const std::string& message) {
        Log(LogLevel::ERROR, message);
    }
};

// Define static members
inline LogLevel ConsoleLogger::current_level_ = LogLevel::INFO;
inline bool ConsoleLogger::enable_console_ = true;
inline std::mutex ConsoleLogger::mutex_;

} // namespace logging
} // namespace neo

// Convenience macros
#define NEO_LOG_INFO(msg) neo::logging::ConsoleLogger::Info(msg)
#define NEO_LOG_DEBUG(msg) neo::logging::ConsoleLogger::Debug(msg)
#define NEO_LOG_WARNING(msg) neo::logging::ConsoleLogger::Warning(msg)
#define NEO_LOG_ERROR(msg) neo::logging::ConsoleLogger::Error(msg)