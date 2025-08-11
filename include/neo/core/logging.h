#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

// Check if spdlog is available
#ifdef NEO_HAS_SPDLOG
#include <fmt/core.h>
#include <fmt/format.h>
#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#else
#include <iostream>
#include <mutex>
#endif

namespace neo::core
{
/**
 * @brief Log levels for the Neo logger
 */
enum class LogLevel
{
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5,
    Off = 6
};

/**
 * @brief Logger configuration
 */
struct LogConfig
{
    LogLevel level = LogLevel::Info;
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%t] %v";
    bool console_output = true;
    bool file_output = true;
    std::string log_file_path = "logs/neo.log";
    size_t max_file_size = 10 * 1024 * 1024;  // 10MB
    size_t max_files = 10;
    bool async_logging = true;
    size_t async_queue_size = 8192;
};

/**
 * @brief Production-ready logger for Neo C++
 */
class Logger
{
   private:
#ifdef NEO_HAS_SPDLOG
    std::shared_ptr<spdlog::logger> logger_;
#else
    LogLevel level_;
    std::mutex mutex_;
#endif
    static std::shared_ptr<Logger> instance_;
    static std::mutex init_mutex_;

    explicit Logger(const std::string& name, const LogConfig& config);

   public:
    /**
     * @brief Initialize the global logger
     * @param name Logger name
     * @param config Logger configuration
     */
    static void Initialize(const std::string& name = "neo", const LogConfig& config = LogConfig{});

    /**
     * @brief Get the global logger instance
     * @return Logger instance
     */
    static std::shared_ptr<Logger> GetInstance();

    /**
     * @brief Set the log level
     * @param level New log level
     */
    void SetLevel(LogLevel level);

    /**
     * @brief Log a trace message
     * @param message Message to log
     */
    template <typename... Args>
    void Trace(const std::string& fmt, Args&&... args);

    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    template <typename... Args>
    void Debug(const std::string& fmt, Args&&... args);

    /**
     * @brief Log an info message
     * @param message Message to log
     */
    template <typename... Args>
    void Info(const std::string& fmt, Args&&... args);

    /**
     * @brief Log a warning message
     * @param message Message to log
     */
    template <typename... Args>
    void Warning(const std::string& fmt, Args&&... args);

    /**
     * @brief Log an error message
     * @param message Message to log
     */
    template <typename... Args>
    void Error(const std::string& fmt, Args&&... args);

    /**
     * @brief Log a critical message
     * @param message Message to log
     */
    template <typename... Args>
    void Critical(const std::string& fmt, Args&&... args);

    /**
     * @brief Flush the logger
     */
    void Flush();
};

// Implementation
#ifdef NEO_HAS_SPDLOG
template <typename... Args>
void Logger::Trace(const std::string& fmt, Args&&... args)
{
    logger_->trace(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args>
void Logger::Debug(const std::string& fmt, Args&&... args)
{
    logger_->debug(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args>
void Logger::Info(const std::string& fmt, Args&&... args)
{
    logger_->info(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args>
void Logger::Warning(const std::string& fmt, Args&&... args)
{
    logger_->warn(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args>
void Logger::Error(const std::string& fmt, Args&&... args)
{
    logger_->error(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args>
void Logger::Critical(const std::string& fmt, Args&&... args)
{
    logger_->critical(fmt::vformat(fmt, fmt::make_format_args(args...)));
}
#else
// Minimal logging implementation when spdlog is not available

// Helper function for simple string formatting
template<typename T>
void FormatString(std::stringstream& ss, const std::string& fmt, T&& value)
{
    size_t pos = fmt.find("{}");
    if (pos != std::string::npos)
    {
        ss << fmt.substr(0, pos) << value << fmt.substr(pos + 2);
    }
    else
    {
        ss << fmt << " " << value;
    }
}

template<typename T, typename... Args>
void FormatString(std::stringstream& ss, const std::string& fmt, T&& value, Args&&... args)
{
    size_t pos = fmt.find("{}");
    if (pos != std::string::npos)
    {
        ss << fmt.substr(0, pos) << value;
        FormatString(ss, fmt.substr(pos + 2), std::forward<Args>(args)...);
    }
    else
    {
        ss << fmt << " " << value;
        ((ss << " " << args), ...);
    }
}

template <typename... Args>
void Logger::Trace(const std::string& fmt, Args&&... args)
{
    if (level_ <= LogLevel::Trace)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if constexpr (sizeof...(args) == 0)
        {
            std::cout << "[TRACE] " << fmt << std::endl;
        }
        else
        {
            std::stringstream ss;
            FormatString(ss, fmt, std::forward<Args>(args)...);
            std::cout << "[TRACE] " << ss.str() << std::endl;
        }
    }
}

template <typename... Args>
void Logger::Debug(const std::string& fmt, Args&&... args)
{
    if (level_ <= LogLevel::Debug)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if constexpr (sizeof...(args) == 0)
        {
            std::cout << "[DEBUG] " << fmt << std::endl;
        }
        else
        {
            std::stringstream ss;
            FormatString(ss, fmt, std::forward<Args>(args)...);
            std::cout << "[DEBUG] " << ss.str() << std::endl;
        }
    }
}

template <typename... Args>
void Logger::Info(const std::string& fmt, Args&&... args)
{
    if (level_ <= LogLevel::Info)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if constexpr (sizeof...(args) == 0)
        {
            std::cout << "[INFO] " << fmt << std::endl;
        }
        else
        {
            // Format string with arguments using a simple approach
            std::stringstream ss;
            FormatString(ss, fmt, std::forward<Args>(args)...);
            std::cout << "[INFO] " << ss.str() << std::endl;
        }
    }
}

template <typename... Args>
void Logger::Warning(const std::string& fmt, Args&&... args)
{
    if (level_ <= LogLevel::Warning)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if constexpr (sizeof...(args) == 0)
        {
            std::cerr << "[WARN] " << fmt << std::endl;
        }
        else
        {
            std::stringstream ss;
            FormatString(ss, fmt, std::forward<Args>(args)...);
            std::cerr << "[WARN] " << ss.str() << std::endl;
        }
    }
}

template <typename... Args>
void Logger::Error(const std::string& fmt, Args&&... args)
{
    if (level_ <= LogLevel::Error)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if constexpr (sizeof...(args) == 0)
        {
            std::cerr << "[ERROR] " << fmt << std::endl;
        }
        else
        {
            std::stringstream ss;
            FormatString(ss, fmt, std::forward<Args>(args)...);
            std::cerr << "[ERROR] " << ss.str() << std::endl;
        }
    }
}

template <typename... Args>
void Logger::Critical(const std::string& fmt, Args&&... args)
{
    if (level_ <= LogLevel::Critical)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if constexpr (sizeof...(args) == 0)
        {
            std::cerr << "[CRITICAL] " << fmt << std::endl;
        }
        else
        {
            std::stringstream ss;
            FormatString(ss, fmt, std::forward<Args>(args)...);
            std::cerr << "[CRITICAL] " << ss.str() << std::endl;
        }
    }
}
#endif

// Convenience macros
#define LOG_TRACE(...) neo::core::Logger::GetInstance()->Trace(__VA_ARGS__)
#define LOG_DEBUG(...) neo::core::Logger::GetInstance()->Debug(__VA_ARGS__)
#define LOG_INFO(...) neo::core::Logger::GetInstance()->Info(__VA_ARGS__)
#define LOG_WARNING(...) neo::core::Logger::GetInstance()->Warning(__VA_ARGS__)
#define LOG_ERROR(...) neo::core::Logger::GetInstance()->Error(__VA_ARGS__)
#define LOG_CRITICAL(...) neo::core::Logger::GetInstance()->Critical(__VA_ARGS__)

/**
 * @brief Performance logger for tracking execution times
 */
class PerfLogger
{
   private:
    std::string operation_;
    std::chrono::steady_clock::time_point start_;
    LogLevel level_;

   public:
    explicit PerfLogger(const std::string& operation, LogLevel level = LogLevel::Debug);
    ~PerfLogger();
};

/**
 * @brief Structured logging helper
 */
class StructuredLog
{
   private:
    std::ostringstream stream_;
    LogLevel level_;
    std::string message_;

   public:
    StructuredLog(LogLevel level, const std::string& message);

    template <typename T>
    StructuredLog& With(const std::string& key, const T& value)
    {
        stream_ << " " << key << "=" << value;
        return *this;
    }

    void Log();
};

/**
 * @brief Factory for creating named loggers
 */
class LoggerFactory
{
   public:
    /**
     * @brief Get a logger by name
     * @param name Logger name
     * @return Shared pointer to logger
     */
    static std::shared_ptr<Logger> GetLogger(const std::string& name)
    {
        // Return the global instance - named loggers share the same backend
        // This maintains consistency across the application
        return Logger::GetInstance();
    }
};
}  // namespace neo::core