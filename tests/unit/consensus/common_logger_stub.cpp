#include <neo/common/logging.h>
#include <iostream>

namespace neo::common
{
LogLevel Logger::minLevel_ = LogLevel::DEBUG;

void Logger::Log(LogLevel level, const std::string& message)
{
    if (static_cast<int>(level) < static_cast<int>(minLevel_))
    {
        return;
    }
    std::cout << "[COMMON LOG] " << message << std::endl;
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

void Logger::SetMinLevel(LogLevel level)
{
    minLevel_ = level;
}
}  // namespace neo::common
