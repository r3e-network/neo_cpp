#include <filesystem>
#include <iostream>
#include <neo/core/logging.h>

namespace neo::core
{
std::shared_ptr<Logger> Logger::instance_;
std::mutex Logger::init_mutex_;

Logger::Logger(const std::string& name, const LogConfig& config)
{
#ifdef HAS_SPDLOG
    std::vector<spdlog::sink_ptr> sinks;

    // Console sink
    if (config.console_output)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(static_cast<spdlog::level::level_enum>(config.level));
        sinks.push_back(console_sink);
    }

    // File sink
    if (config.file_output)
    {
        // Create log directory if it doesn't exist
        std::filesystem::path log_path(config.log_file_path);
        std::filesystem::create_directories(log_path.parent_path());

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config.log_file_path,
                                                                                config.max_file_size, config.max_files);
        file_sink->set_level(static_cast<spdlog::level::level_enum>(config.level));
        sinks.push_back(file_sink);
    }

    // Create logger
    if (config.async_logging)
    {
        spdlog::init_thread_pool(config.async_queue_size, 1);
        logger_ = std::make_shared<spdlog::async_logger>(name, sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                         spdlog::async_overflow_policy::block);
    }
    else
    {
        logger_ = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    }

    logger_->set_level(static_cast<spdlog::level::level_enum>(config.level));
    logger_->set_pattern(config.pattern);
    logger_->flush_on(spdlog::level::warn);

    // Register it globally
    spdlog::register_logger(logger_);
#else
    level_ = config.level;
    std::cout << "Neo logger initialized (minimal mode - spdlog not available)" << std::endl;
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
#ifdef HAS_SPDLOG
    logger_->set_level(static_cast<spdlog::level::level_enum>(level));
#else
    level_ = level;
#endif
}

void Logger::Flush()
{
#ifdef HAS_SPDLOG
    logger_->flush();
#else
    std::cout.flush();
    std::cerr.flush();
#endif
}

// PerfLogger implementation
PerfLogger::PerfLogger(const std::string& operation, LogLevel level)
    : operation_(operation), start_(std::chrono::steady_clock::now()), level_(level)
{
    if (level_ <= LogLevel::Debug)
    {
        LOG_DEBUG("Starting operation: {}", operation_);
    }
}

PerfLogger::~PerfLogger()
{
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();

    if (level_ <= LogLevel::Debug)
    {
        LOG_DEBUG("Operation '{}' completed in {} µs", operation_, duration);
    }
}

// StructuredLog implementation
StructuredLog::StructuredLog(LogLevel level, const std::string& message) : level_(level), message_(message) {}

void StructuredLog::Log()
{
    std::string full_message = message_ + stream_.str();

    switch (level_)
    {
        case LogLevel::Trace:
            LOG_TRACE("{}", full_message);
            break;
        case LogLevel::Debug:
            LOG_DEBUG("{}", full_message);
            break;
        case LogLevel::Info:
            LOG_INFO("{}", full_message);
            break;
        case LogLevel::Warning:
            LOG_WARNING("{}", full_message);
            break;
        case LogLevel::Error:
            LOG_ERROR("{}", full_message);
            break;
        case LogLevel::Critical:
            LOG_CRITICAL("{}", full_message);
            break;
        default:
            break;
    }
}
}  // namespace neo::core