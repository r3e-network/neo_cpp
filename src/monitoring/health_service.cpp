/**
 * @file health_service.cpp
 * @brief Health check service implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/monitoring/health_service.h>
#include <neo/core/logging.h>

#include <nlohmann/json.hpp>
#include <sstream>
#include <filesystem>
#include <fstream>

#ifdef NEO_HAS_HTTPLIB
#include <httplib.h>
#endif

namespace neo
{
namespace monitoring
{

using json = nlohmann::json;

HealthService::HealthService()
{
    current_health_.overall_status = HealthStatus::Healthy;
    current_health_.overall_message = "System starting up";
    current_health_.last_updated = std::chrono::steady_clock::now();

    // Register built-in health checks
    RegisterHealthCheck("system_memory", CheckSystemMemory, 60);
    RegisterHealthCheck("system_disk", CheckSystemDisk, 120);
    RegisterHealthCheck("system_cpu", CheckSystemCPU, 30);
}

HealthService::~HealthService()
{
    Stop();
}

bool HealthService::Start()
{
    if (running_.exchange(true))
    {
        LOG_WARNING("Health service already running");
        return true;
    }

    LOG_INFO("Starting health service");

    // Start health check thread
    health_thread_ = std::thread(&HealthService::HealthCheckLoop, this);

    LOG_INFO("Health service started successfully");
    return true;
}

void HealthService::Stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    LOG_INFO("Stopping health service");

    if (health_thread_.joinable())
    {
        health_thread_.join();
    }

    LOG_INFO("Health service stopped");
}

void HealthService::RegisterHealthCheck(const std::string& name, 
                                      HealthCheckFunction check_function,
                                      uint32_t interval_seconds)
{
    std::lock_guard<std::mutex> lock(health_mutex_);
    
    HealthCheckInfo info;
    info.name = name;
    info.function = std::move(check_function);
    info.interval_seconds = interval_seconds;
    info.last_check = std::chrono::steady_clock::now();
    info.last_result = {name, HealthStatus::Healthy, "Not yet checked", std::chrono::milliseconds(0), info.last_check};

    health_checks_[name] = std::move(info);
    
    LOG_INFO("Registered health check: {} (interval: {}s)", name, interval_seconds);
}

void HealthService::UnregisterHealthCheck(const std::string& name)
{
    std::lock_guard<std::mutex> lock(health_mutex_);
    
    auto it = health_checks_.find(name);
    if (it != health_checks_.end())
    {
        health_checks_.erase(it);
        current_health_.checks.erase(name);
        LOG_INFO("Unregistered health check: {}", name);
    }
}

SystemHealth HealthService::GetSystemHealth() const
{
    std::lock_guard<std::mutex> lock(health_mutex_);
    return current_health_;
}

std::string HealthService::GetHealthJson() const
{
    std::lock_guard<std::mutex> lock(health_mutex_);
    
    json health_json;
    health_json["status"] = HealthStatusToString(current_health_.overall_status);
    health_json["message"] = current_health_.overall_message;
    health_json["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        current_health_.last_updated.time_since_epoch()).count();

    json checks_json = json::object();
    for (const auto& [name, result] : current_health_.checks)
    {
        checks_json[name] = {
            {"status", HealthStatusToString(result.status)},
            {"message", result.message},
            {"duration_ms", result.duration.count()},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                result.timestamp.time_since_epoch()).count()}
        };
    }
    health_json["checks"] = checks_json;

    return health_json.dump(2);
}

std::string HealthService::GetHealthMetrics() const
{
    std::lock_guard<std::mutex> lock(health_mutex_);
    
    std::ostringstream metrics;
    
    // Overall health metric
    metrics << "# HELP neo_health_status Overall system health status (0=Healthy, 1=Degraded, 2=Unhealthy, 3=Critical)\n";
    metrics << "# TYPE neo_health_status gauge\n";
    metrics << "neo_health_status " << static_cast<int>(current_health_.overall_status) << "\n\n";

    // Individual health check metrics
    metrics << "# HELP neo_health_check_status Individual health check status (0=Healthy, 1=Degraded, 2=Unhealthy, 3=Critical)\n";
    metrics << "# TYPE neo_health_check_status gauge\n";
    
    for (const auto& [name, result] : current_health_.checks)
    {
        metrics << "neo_health_check_status{check=\"" << name << "\"} " 
                << static_cast<int>(result.status) << "\n";
    }
    metrics << "\n";

    // Health check duration metrics
    metrics << "# HELP neo_health_check_duration_ms Duration of health checks in milliseconds\n";
    metrics << "# TYPE neo_health_check_duration_ms gauge\n";
    
    for (const auto& [name, result] : current_health_.checks)
    {
        metrics << "neo_health_check_duration_ms{check=\"" << name << "\"} " 
                << result.duration.count() << "\n";
    }

    return metrics.str();
}

void HealthService::ForceHealthCheck()
{
    std::lock_guard<std::mutex> lock(health_mutex_);
    
    for (auto& [name, check_info] : health_checks_)
    {
        ExecuteHealthCheck(check_info);
    }
    
    UpdateOverallHealth();
}

HealthService& HealthService::GetInstance()
{
    static HealthService instance;
    return instance;
}

void HealthService::HealthCheckLoop()
{
    LOG_INFO("Health check loop started");
    
    while (running_.load())
    {
        try
        {
            auto now = std::chrono::steady_clock::now();
            bool health_updated = false;
            
            {
                std::lock_guard<std::mutex> lock(health_mutex_);
                
                for (auto& [name, check_info] : health_checks_)
                {
                    auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(
                        now - check_info.last_check);
                    
                    if (time_since_last.count() >= check_info.interval_seconds)
                    {
                        ExecuteHealthCheck(check_info);
                        health_updated = true;
                    }
                }
                
                if (health_updated)
                {
                    UpdateOverallHealth();
                }
            }
            
            // Sleep for 1 second between iterations
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error in health check loop: {}", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    LOG_INFO("Health check loop stopped");
}

void HealthService::ExecuteHealthCheck(HealthCheckInfo& check_info)
{
    auto start_time = std::chrono::steady_clock::now();
    
    try
    {
        HealthCheckResult result = check_info.function();
        result.timestamp = start_time;
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        check_info.last_result = result;
        check_info.last_check = start_time;
        current_health_.checks[check_info.name] = result;
        
        LOG_DEBUG("Health check completed: {} - {} ({}ms)", 
                  result.name, HealthStatusToString(result.status), result.duration.count());
    }
    catch (const std::exception& e)
    {
        auto end_time = std::chrono::steady_clock::now();
        HealthCheckResult error_result;
        error_result.name = check_info.name;
        error_result.status = HealthStatus::Critical;
        error_result.message = "Health check failed: " + std::string(e.what());
        error_result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        error_result.timestamp = start_time;
        
        check_info.last_result = error_result;
        check_info.last_check = start_time;
        current_health_.checks[check_info.name] = error_result;
        
        LOG_ERROR("Health check {} failed: {}", check_info.name, e.what());
    }
}

void HealthService::UpdateOverallHealth()
{
    current_health_.overall_status = DetermineOverallStatus();
    current_health_.last_updated = std::chrono::steady_clock::now();
    
    // Update overall message based on status
    size_t healthy_count = 0;
    size_t degraded_count = 0;
    size_t unhealthy_count = 0;
    size_t critical_count = 0;
    
    for (const auto& [name, result] : current_health_.checks)
    {
        switch (result.status)
        {
            case HealthStatus::Healthy: healthy_count++; break;
            case HealthStatus::Degraded: degraded_count++; break;
            case HealthStatus::Unhealthy: unhealthy_count++; break;
            case HealthStatus::Critical: critical_count++; break;
        }
    }
    
    std::ostringstream message;
    message << healthy_count << " healthy, " << degraded_count << " degraded, "
            << unhealthy_count << " unhealthy, " << critical_count << " critical";
    current_health_.overall_message = message.str();
}

HealthStatus HealthService::DetermineOverallStatus() const
{
    if (current_health_.checks.empty())
    {
        return HealthStatus::Degraded;
    }
    
    bool has_critical = false;
    bool has_unhealthy = false;
    bool has_degraded = false;
    
    for (const auto& [name, result] : current_health_.checks)
    {
        switch (result.status)
        {
            case HealthStatus::Critical:
                has_critical = true;
                break;
            case HealthStatus::Unhealthy:
                has_unhealthy = true;
                break;
            case HealthStatus::Degraded:
                has_degraded = true;
                break;
            case HealthStatus::Healthy:
                break;
        }
    }
    
    if (has_critical)
        return HealthStatus::Critical;
    if (has_unhealthy)
        return HealthStatus::Unhealthy;
    if (has_degraded)
        return HealthStatus::Degraded;
    
    return HealthStatus::Healthy;
}

std::string HealthService::HealthStatusToString(HealthStatus status) const
{
    switch (status)
    {
        case HealthStatus::Healthy: return "healthy";
        case HealthStatus::Degraded: return "degraded";
        case HealthStatus::Unhealthy: return "unhealthy";
        case HealthStatus::Critical: return "critical";
        default: return "unknown";
    }
}

// Built-in health checks
HealthCheckResult HealthService::CheckSystemMemory()
{
    HealthCheckResult result;
    result.name = "system_memory";
    
    try
    {
#ifdef __linux__
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo.is_open())
        {
            result.status = HealthStatus::Degraded;
            result.message = "Cannot read /proc/meminfo";
            return result;
        }
        
        std::string line;
        uint64_t mem_total = 0, mem_available = 0;
        
        while (std::getline(meminfo, line))
        {
            if (line.starts_with("MemTotal:"))
            {
                mem_total = std::stoull(line.substr(line.find_first_of("0123456789")));
            }
            else if (line.starts_with("MemAvailable:"))
            {
                mem_available = std::stoull(line.substr(line.find_first_of("0123456789")));
                break;
            }
        }
        
        if (mem_total == 0 || mem_available == 0)
        {
            result.status = HealthStatus::Degraded;
            result.message = "Could not parse memory information";
            return result;
        }
        
        double memory_usage = 1.0 - (static_cast<double>(mem_available) / mem_total);
        
        if (memory_usage > 0.95)
        {
            result.status = HealthStatus::Critical;
            result.message = "Memory usage critical: " + std::to_string(memory_usage * 100) + "%";
        }
        else if (memory_usage > 0.90)
        {
            result.status = HealthStatus::Unhealthy;
            result.message = "Memory usage high: " + std::to_string(memory_usage * 100) + "%";
        }
        else if (memory_usage > 0.80)
        {
            result.status = HealthStatus::Degraded;
            result.message = "Memory usage elevated: " + std::to_string(memory_usage * 100) + "%";
        }
        else
        {
            result.status = HealthStatus::Healthy;
            result.message = "Memory usage normal: " + std::to_string(memory_usage * 100) + "%";
        }
#else
        result.status = HealthStatus::Degraded;
        result.message = "Memory monitoring not implemented for this platform";
#endif
    }
    catch (const std::exception& e)
    {
        result.status = HealthStatus::Critical;
        result.message = "Memory check failed: " + std::string(e.what());
    }
    
    return result;
}

HealthCheckResult HealthService::CheckSystemDisk()
{
    HealthCheckResult result;
    result.name = "system_disk";
    
    try
    {
        // Check current directory disk space
        auto space_info = std::filesystem::space(".");
        
        if (space_info.capacity == 0)
        {
            result.status = HealthStatus::Degraded;
            result.message = "Cannot determine disk space";
            return result;
        }
        
        double disk_usage = 1.0 - (static_cast<double>(space_info.available) / space_info.capacity);
        
        if (disk_usage > 0.95)
        {
            result.status = HealthStatus::Critical;
            result.message = "Disk usage critical: " + std::to_string(disk_usage * 100) + "%";
        }
        else if (disk_usage > 0.90)
        {
            result.status = HealthStatus::Unhealthy;
            result.message = "Disk usage high: " + std::to_string(disk_usage * 100) + "%";
        }
        else if (disk_usage > 0.80)
        {
            result.status = HealthStatus::Degraded;
            result.message = "Disk usage elevated: " + std::to_string(disk_usage * 100) + "%";
        }
        else
        {
            result.status = HealthStatus::Healthy;
            result.message = "Disk usage normal: " + std::to_string(disk_usage * 100) + "%";
        }
    }
    catch (const std::exception& e)
    {
        result.status = HealthStatus::Critical;
        result.message = "Disk check failed: " + std::string(e.what());
    }
    
    return result;
}

HealthCheckResult HealthService::CheckSystemCPU()
{
    HealthCheckResult result;
    result.name = "system_cpu";
    
    try
    {
#ifdef __linux__
        // Simple CPU load check by reading load average
        std::ifstream loadavg("/proc/loadavg");
        if (!loadavg.is_open())
        {
            result.status = HealthStatus::Degraded;
            result.message = "Cannot read /proc/loadavg";
            return result;
        }
        
        double load_1min;
        loadavg >> load_1min;
        
        // Get number of CPU cores
        unsigned int cpu_count = std::thread::hardware_concurrency();
        if (cpu_count == 0) cpu_count = 1;
        
        double load_per_core = load_1min / cpu_count;
        
        if (load_per_core > 2.0)
        {
            result.status = HealthStatus::Critical;
            result.message = "CPU load critical: " + std::to_string(load_per_core) + " per core";
        }
        else if (load_per_core > 1.5)
        {
            result.status = HealthStatus::Unhealthy;
            result.message = "CPU load high: " + std::to_string(load_per_core) + " per core";
        }
        else if (load_per_core > 1.0)
        {
            result.status = HealthStatus::Degraded;
            result.message = "CPU load elevated: " + std::to_string(load_per_core) + " per core";
        }
        else
        {
            result.status = HealthStatus::Healthy;
            result.message = "CPU load normal: " + std::to_string(load_per_core) + " per core";
        }
#else
        result.status = HealthStatus::Degraded;
        result.message = "CPU monitoring not implemented for this platform";
#endif
    }
    catch (const std::exception& e)
    {
        result.status = HealthStatus::Critical;
        result.message = "CPU check failed: " + std::string(e.what());
    }
    
    return result;
}

// HTTP Server Implementation
HealthHttpServer::HealthHttpServer(HealthService& health_service, 
                                   const std::string& bind_address,
                                   uint16_t port)
    : health_service_(health_service), bind_address_(bind_address), port_(port)
{
}

HealthHttpServer::~HealthHttpServer()
{
    Stop();
}

bool HealthHttpServer::Start()
{
    if (running_.exchange(true))
    {
        LOG_WARNING("Health HTTP server already running");
        return true;
    }

    LOG_INFO("Starting health HTTP server on {}:{}", bind_address_, port_);

    server_thread_ = std::thread(&HealthHttpServer::ServerLoop, this);

    LOG_INFO("Health HTTP server started successfully");
    return true;
}

void HealthHttpServer::Stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    LOG_INFO("Stopping health HTTP server");

    if (server_thread_.joinable())
    {
        server_thread_.join();
    }

    LOG_INFO("Health HTTP server stopped");
}

void HealthHttpServer::ServerLoop()
{
#ifdef NEO_HAS_HTTPLIB
    httplib::Server server;

    // Health check endpoint (JSON)
    server.Get("/health",
        [this](const httplib::Request& req, httplib::Response& res)
        {
            try
            {
                auto health = health_service_.GetSystemHealth();
                std::string json_response = health_service_.GetHealthJson();
                
                // Set HTTP status based on health
                switch (health.overall_status)
                {
                    case HealthStatus::Healthy:
                        res.status = 200;
                        break;
                    case HealthStatus::Degraded:
                        res.status = 200; // Still OK, but with warnings
                        break;
                    case HealthStatus::Unhealthy:
                        res.status = 503; // Service Unavailable
                        break;
                    case HealthStatus::Critical:
                        res.status = 503; // Service Unavailable
                        break;
                }
                
                res.set_content(json_response, "application/json");
                res.set_header("Cache-Control", "no-cache");
            }
            catch (const std::exception& e)
            {
                res.status = 500;
                res.set_content("{\"error\":\"" + std::string(e.what()) + "\"}", "application/json");
            }
        });

    // Ready check endpoint (simple)
    server.Get("/ready",
        [this](const httplib::Request& req, httplib::Response& res)
        {
            auto health = health_service_.GetSystemHealth();
            
            if (health.overall_status == HealthStatus::Healthy || 
                health.overall_status == HealthStatus::Degraded)
            {
                res.status = 200;
                res.set_content("OK", "text/plain");
            }
            else
            {
                res.status = 503;
                res.set_content("NOT READY", "text/plain");
            }
            
            res.set_header("Cache-Control", "no-cache");
        });

    // Live check endpoint (simple)
    server.Get("/live",
        [this](const httplib::Request& req, httplib::Response& res)
        {
            // Service is live if the health service is running
            if (health_service_.IsRunning())
            {
                res.status = 200;
                res.set_content("OK", "text/plain");
            }
            else
            {
                res.status = 503;
                res.set_content("NOT LIVE", "text/plain");
            }
            
            res.set_header("Cache-Control", "no-cache");
        });

    // Metrics endpoint (Prometheus format)
    server.Get("/metrics",
        [this](const httplib::Request& req, httplib::Response& res)
        {
            try
            {
                std::string metrics = health_service_.GetHealthMetrics();
                res.status = 200;
                res.set_content(metrics, "text/plain; version=0.0.4");
                res.set_header("Cache-Control", "no-cache");
            }
            catch (const std::exception& e)
            {
                res.status = 500;
                res.set_content("# Error generating metrics: " + std::string(e.what()) + "\n", "text/plain");
            }
        });

    // Start server
    server.listen(bind_address_.c_str(), port_);
#else
    LOG_ERROR("HTTP server not available - health endpoints cannot start");
    running_ = false;
#endif
}

} // namespace monitoring
} // namespace neo