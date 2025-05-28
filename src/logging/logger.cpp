#include <neo/logging/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace neo::logging
{
    Logger& Logger::GetDefault()
    {
        static Logger defaultLogger("neo");
        return defaultLogger;
    }

    Logger& Logger::Instance()
    {
        return GetDefault();
    }

    void Logger::Debug(const std::string& category, const std::string& message)
    {
        Debug("[" + category + "] " + message);
    }

    void Logger::Info(const std::string& category, const std::string& message)
    {
        Info("[" + category + "] " + message);
    }

    void Logger::Warn(const std::string& category, const std::string& message)
    {
        Warn("[" + category + "] " + message);
    }

    void Logger::Warning(const std::string& message)
    {
        Warn(message);
    }

    void Logger::Warning(const std::string& category, const std::string& message)
    {
        Warn(category, message);
    }

    void Logger::Error(const std::string& category, const std::string& message)
    {
        Error("[" + category + "] " + message);
    }

    std::shared_ptr<Logger> Logger::Create(const std::string& name)
    {
        return std::make_shared<Logger>(name);
    }

    Logger::Logger(const std::string& name)
    {
        try
        {
            // Create console sink
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

            // Create file sink
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("neo.log", true);
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

            // Create logger with both sinks
            logger_ = std::make_shared<spdlog::logger>(name,
                spdlog::sinks_init_list{console_sink, file_sink});

            logger_->set_level(spdlog::level::info);
            logger_->flush_on(spdlog::level::warn);

            // Register the logger
            spdlog::register_logger(logger_);
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            // Fallback to basic console logger
            logger_ = spdlog::stdout_color_mt(name);
        }
    }

    void Logger::SetLevel(Level level)
    {
        if (!logger_) return;

        spdlog::level::level_enum spdlog_level;
        switch (level)
        {
            case Level::Trace:
                spdlog_level = spdlog::level::trace;
                break;
            case Level::Debug:
                spdlog_level = spdlog::level::debug;
                break;
            case Level::Info:
                spdlog_level = spdlog::level::info;
                break;
            case Level::Warn:
                spdlog_level = spdlog::level::warn;
                break;
            case Level::Error:
                spdlog_level = spdlog::level::err;
                break;
            case Level::Critical:
                spdlog_level = spdlog::level::critical;
                break;
            case Level::Off:
                spdlog_level = spdlog::level::off;
                break;
            default:
                spdlog_level = spdlog::level::info;
                break;
        }

        logger_->set_level(spdlog_level);
    }

    void Logger::Trace(const std::string& message)
    {
        if (logger_) logger_->trace(message);
    }

    void Logger::Debug(const std::string& message)
    {
        if (logger_) logger_->debug(message);
    }

    void Logger::Info(const std::string& message)
    {
        if (logger_) logger_->info(message);
    }

    void Logger::Warn(const std::string& message)
    {
        if (logger_) logger_->warn(message);
    }

    void Logger::Error(const std::string& message)
    {
        if (logger_) logger_->error(message);
    }

    void Logger::Critical(const std::string& message)
    {
        if (logger_) logger_->critical(message);
    }
}
