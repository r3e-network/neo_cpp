#pragma once

/**
 * @file logger.h
 * @brief Simple logging interface for Neo C++ implementation
 *
 * This header provides a basic logging interface compatible with
 * the existing codebase requirements.
 */

#include <memory>
#include <string>
#include <iosfwd>

#if defined(NEO_HAS_SPDLOG) && !defined(NEO_MINIMAL_LOGGING)
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#endif

namespace neo::logging
{
    /**
     * @brief Logger class providing structured logging functionality
     */
    class Logger
    {
    public:
        /**
         * @brief Log levels
         */
        enum class Level
        {
            Trace = 0,
            Debug = 1,
            Info = 2,
            Warn = 3,
            Error = 4,
            Critical = 5,
            Off = 6
        };

        /**
         * @brief Get the default logger instance
         * @return Reference to the default logger
         */
        static Logger& GetDefault();

        /**
         * @brief Get the singleton logger instance (alias for GetDefault)
         * @return Reference to the singleton logger
         */
        static Logger& Instance();

        /**
         * @brief Create a named logger
         * @param name Logger name
         * @return Shared pointer to the logger
         */
        static std::shared_ptr<Logger> Create(const std::string& name);

        /**
         * @brief Constructor
         * @param name Logger name
         */
        explicit Logger(const std::string& name);

        /**
         * @brief Set the log level
         * @param level Log level
         */
        void SetLevel(Level level);

        /**
         * @brief Log a trace message
         * @param message Message to log
         */
        void Trace(const std::string& message);

        /**
         * @brief Log a debug message
         * @param message Message to log
         */
        void Debug(const std::string& message);

        /**
         * @brief Log a debug message with category
         * @param category Message category
         * @param message Message to log
         */
        void Debug(const std::string& category, const std::string& message);

        /**
         * @brief Log an info message
         * @param message Message to log
         */
        void Info(const std::string& message);

        /**
         * @brief Log an info message with category
         * @param category Message category
         * @param message Message to log
         */
        void Info(const std::string& category, const std::string& message);

        /**
         * @brief Log a warning message
         * @param message Message to log
         */
        void Warn(const std::string& message);

        /**
         * @brief Log a warning message with category
         * @param category Message category
         * @param message Message to log
         */
        void Warn(const std::string& category, const std::string& message);

        /**
         * @brief Log a warning message (alias for Warn)
         * @param message Message to log
         */
        void Warning(const std::string& message);

        /**
         * @brief Log a warning message with category (alias for Warn)
         * @param category Message category
         * @param message Message to log
         */
        void Warning(const std::string& category, const std::string& message);

        /**
         * @brief Log an error message
         * @param message Message to log
         */
        void Error(const std::string& message);

        /**
         * @brief Log an error message with category
         * @param category Message category
         * @param message Message to log
         */
        void Error(const std::string& category, const std::string& message);

        /**
         * @brief Log a critical message
         * @param message Message to log
         */
        void Critical(const std::string& message);

        /**
         * @brief Template method for formatted logging
         * @tparam Args Argument types
         * @param level Log level
         * @param format Format string
         * @param args Arguments
         */
        template<typename... Args>
        void Log(Level level, const std::string& format, Args&&... args);

        // Helper method for minimal logging
        void LogMinimal(Level level, const std::string& message);

    private:
#if defined(NEO_MINIMAL_LOGGING) || !defined(NEO_HAS_SPDLOG)
        Level current_level_ = Level::Info;
        std::string name_;
#else
        std::shared_ptr<spdlog::logger> logger_;
#endif
    };

    // Convenience macros for logging
    #define NEO_LOG_TRACE(msg) neo::logging::Logger::GetDefault().Trace(msg)
    #define NEO_LOG_DEBUG(msg) neo::logging::Logger::GetDefault().Debug(msg)
    #define NEO_LOG_INFO(msg) neo::logging::Logger::GetDefault().Info(msg)
    #define NEO_LOG_WARN(msg) neo::logging::Logger::GetDefault().Warn(msg)
    #define NEO_LOG_ERROR(msg) neo::logging::Logger::GetDefault().Error(msg)
    #define NEO_LOG_CRITICAL(msg) neo::logging::Logger::GetDefault().Critical(msg)
}
