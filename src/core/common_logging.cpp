/**
 * @file common_logging.cpp
 * @brief Common Logging
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/common/logging.h>
#include <neo/logging/logger.h>

#include <chrono>
#include <iomanip>

namespace neo::common
{
LogLevel Logger::minLevel_ = LogLevel::INFO;

void Logger::Log(LogLevel level, const std::string& message)
{
    if (level < minLevel_) return;

    // Use the neo::logging::Logger
    switch (level)
    {
        case LogLevel::DEBUG:
            neo::logging::Logger::Instance().Debug("Common", message);
            break;
        case LogLevel::INFO:
            neo::logging::Logger::Instance().Info("Common", message);
            break;
        case LogLevel::WARNING:
            neo::logging::Logger::Instance().Warning("Common", message);
            break;
        case LogLevel::ERROR_LEVEL:
            neo::logging::Logger::Instance().Error("Common", message);
            break;
    }
}

std::string Logger::GetLevelString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR_LEVEL:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

void Logger::SetMinLevel(LogLevel level) { minLevel_ = level; }
}  // namespace neo::common