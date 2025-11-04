#include <neo/core/logging.h>

namespace neo::core
{
std::shared_ptr<Logger> Logger::instance_;
std::mutex Logger::init_mutex_;

Logger::Logger(const std::string&, const LogConfig& config)
{
#ifndef NEO_HAS_SPDLOG
    level_ = config.level;
#endif
}

void Logger::Initialize(const std::string& name, const LogConfig& config)
{
    std::lock_guard<std::mutex> lock(init_mutex_);
    if (!instance_)
    {
        instance_ = std::shared_ptr<Logger>(new Logger(name, config));
    }
}

std::shared_ptr<Logger> Logger::GetInstance()
{
    std::lock_guard<std::mutex> lock(init_mutex_);
    if (!instance_)
    {
        instance_ = std::shared_ptr<Logger>(new Logger("neo", LogConfig{}));
    }
    return instance_;
}

void Logger::SetLevel(LogLevel level)
{
#ifndef NEO_HAS_SPDLOG
    level_ = level;
#else
    (void)level;
#endif
}

void Logger::Flush()
{
    // no-op for stub
}
}  // namespace neo::core
