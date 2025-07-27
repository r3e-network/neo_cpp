#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace neo::common
{
/**
 * @brief Log levels for the logging system.
 */
enum class LogLevel
{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR_LEVEL = 3
};

/**
 * @brief Simple logging class.
 */
class Logger
{
  public:
    /**
     * @brief Logs a message with the specified level.
     * @param level The log level.
     * @param message The message to log.
     */
    static void Log(LogLevel level, const std::string& message);

    /**
     * @brief Gets the string representation of a log level.
     * @param level The log level.
     * @return The string representation.
     */
    static std::string GetLevelString(LogLevel level);

    /**
     * @brief Sets the minimum log level.
     * @param level The minimum log level.
     */
    static void SetMinLevel(LogLevel level);

  private:
    static LogLevel minLevel_;
};
}  // namespace neo::common

// Convenience macros for logging
#define NEO_DEBUG neo::common::LogLevel::DEBUG
#define NEO_INFO neo::common::LogLevel::INFO
#define NEO_WARNING neo::common::LogLevel::WARNING
#define NEO_ERROR neo::common::LogLevel::ERROR_LEVEL

#define NEO_LOG(level, message)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        std::ostringstream oss;                                                                                        \
        oss << message;                                                                                                \
        neo::common::Logger::Log(level, oss.str());                                                                    \
    } while (0)
