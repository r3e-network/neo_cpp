#include <neo/logging/logger.h>
#include <iostream>
#if defined(NEO_HAS_SPDLOG) && !defined(NEO_MINIMAL_LOGGING)
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fmt/core.h>
#include <fmt/format.h>
#endif

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
#ifdef NEO_MINIMAL_LOGGING
        : name_(name)
    {
        // Minimal logging implementation - no initialization needed
    }
#else
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
            (void)ex; // Suppress unused variable warning
            // Fallback to basic console logger
            logger_ = spdlog::stdout_color_mt(name);
        }
    }
#endif

    void Logger::SetLevel(Level level)
    {
#ifdef NEO_MINIMAL_LOGGING
        current_level_ = level;
#else
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
#endif
    }

    void Logger::Trace(const std::string& message)
    {
#ifdef NEO_MINIMAL_LOGGING
        if (current_level_ <= Level::Trace) {
            std::cout << "[TRACE] " << message << std::endl;
        }
#else
        if (logger_) logger_->trace(message);
#endif
    }

    void Logger::Debug(const std::string& message)
    {
#ifdef NEO_MINIMAL_LOGGING
        if (current_level_ <= Level::Debug) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
#else
        if (logger_) logger_->debug(message);
#endif
    }

    void Logger::Info(const std::string& message)
    {
#ifdef NEO_MINIMAL_LOGGING
        if (current_level_ <= Level::Info) {
            std::cout << "[INFO] " << message << std::endl;
        }
#else
        if (logger_) logger_->info(message);
#endif
    }

    void Logger::Warn(const std::string& message)
    {
#ifdef NEO_MINIMAL_LOGGING
        if (current_level_ <= Level::Warn) {
            std::cout << "[WARN] " << message << std::endl;
        }
#else
        if (logger_) logger_->warn(message);
#endif
    }

    void Logger::Error(const std::string& message)
    {
#ifdef NEO_MINIMAL_LOGGING
        if (current_level_ <= Level::Error) {
            std::cout << "[ERROR] " << message << std::endl;
        }
#else
        if (logger_) logger_->error(message);
#endif
    }

    void Logger::Critical(const std::string& message)
    {
#ifdef NEO_MINIMAL_LOGGING
        if (current_level_ <= Level::Critical) {
            std::cout << "[CRITICAL] " << message << std::endl;
        }
#else
        if (logger_) logger_->critical(message);
#endif
    }

    void Logger::LogMinimal(Level level, const std::string& message)
    {
        const char* level_names[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", "OFF"};
        std::cout << "[" << level_names[static_cast<int>(level)] << "] " << message << std::endl;
    }

    // Template instantiations
    template<typename... Args>
    void Logger::Log(Level level, const std::string& format, Args&&... args)
    {
#if defined(NEO_MINIMAL_LOGGING) || !defined(NEO_HAS_SPDLOG)
        // For minimal logging, just use the format string as-is
        LogMinimal(level, format);
#else
        if (!logger_) return;

        // Use fmt::vformat to handle dynamic format strings
        std::string formatted = fmt::vformat(format, fmt::make_format_args(args...));

        switch (level)
        {
            case Level::Trace:
                logger_->trace(formatted);
                break;
            case Level::Debug:
                logger_->debug(formatted);
                break;
            case Level::Info:
                logger_->info(formatted);
                break;
            case Level::Warn:
                logger_->warn(formatted);
                break;
            case Level::Error:
                logger_->error(formatted);
                break;
            case Level::Critical:
                logger_->critical(formatted);
                break;
            default:
                break;
        }
#endif
    }

    // Explicit template instantiations for common use cases
    template void Logger::Log<>(Level level, const std::string& format);
    template void Logger::Log<const char*>(Level level, const std::string& format, const char*&& args);
    template void Logger::Log<std::string>(Level level, const std::string& format, std::string&& args);
}
