#include <neo/core/logging.h>
#include <iostream>

namespace neo::core
{
std::shared_ptr<Logger> Logger::instance_;
std::mutex Logger::init_mutex_;

Logger::Logger(const std::string&, const LogConfig& config)
#ifdef NEO_HAS_SPDLOG
    : logger_(nullptr)
#else
    : level_(config.level)
#endif
{
    (void)config;
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
#ifdef NEO_HAS_SPDLOG
    (void)level;
#else
    level_ = level;
#endif
}

void Logger::Flush()
{
#ifdef NEO_HAS_SPDLOG
    if (logger_)
    {
        logger_->flush();
    }
#else
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << std::flush;
#endif
}
}  // namespace neo::core
