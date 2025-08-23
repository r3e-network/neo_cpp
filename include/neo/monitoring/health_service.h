/**
 * @file health_service.h
 * @brief Health check service for production monitoring
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/core/logging.h>
#include <neo/core/configuration_manager.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

namespace neo
{
namespace monitoring
{

/**
 * @brief Health check status
 */
enum class HealthStatus
{
    Healthy,
    Degraded,
    Unhealthy,
    Critical
};

/**
 * @brief Individual health check result
 */
struct HealthCheckResult
{
    std::string name;
    HealthStatus status;
    std::string message;
    std::chrono::milliseconds duration;
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief Overall system health information
 */
struct SystemHealth
{
    HealthStatus overall_status;
    std::string overall_message;
    std::unordered_map<std::string, HealthCheckResult> checks;
    std::chrono::steady_clock::time_point last_updated;
};

/**
 * @brief Health check function type
 */
using HealthCheckFunction = std::function<HealthCheckResult()>;

/**
 * @brief Production-ready health monitoring service
 */
class HealthService
{
public:
    /**
     * @brief Constructor
     */
    HealthService();

    /**
     * @brief Destructor
     */
    ~HealthService();

    /**
     * @brief Start the health service
     * @return true if started successfully
     */
    bool Start();

    /**
     * @brief Stop the health service
     */
    void Stop();

    /**
     * @brief Check if service is running
     */
    bool IsRunning() const { return running_.load(); }

    /**
     * @brief Register a health check
     * @param name Unique name for the health check
     * @param check_function Function to execute for health check
     * @param interval_seconds Interval between checks in seconds
     */
    void RegisterHealthCheck(const std::string& name, 
                           HealthCheckFunction check_function,
                           uint32_t interval_seconds = 30);

    /**
     * @brief Unregister a health check
     * @param name Name of the health check to remove
     */
    void UnregisterHealthCheck(const std::string& name);

    /**
     * @brief Get current system health
     * @return Current health status
     */
    SystemHealth GetSystemHealth() const;

    /**
     * @brief Get health status as JSON string
     * @return JSON representation of health status
     */
    std::string GetHealthJson() const;

    /**
     * @brief Get health status as Prometheus metrics format
     * @return Prometheus metrics string
     */
    std::string GetHealthMetrics() const;

    /**
     * @brief Force execution of all health checks
     */
    void ForceHealthCheck();

    /**
     * @brief Get singleton instance
     */
    static HealthService& GetInstance();

private:
    struct HealthCheckInfo
    {
        std::string name;
        HealthCheckFunction function;
        uint32_t interval_seconds;
        std::chrono::steady_clock::time_point last_check;
        HealthCheckResult last_result;
    };

    std::atomic<bool> running_{false};
    mutable std::mutex health_mutex_;
    std::unordered_map<std::string, HealthCheckInfo> health_checks_;
    std::thread health_thread_;
    SystemHealth current_health_;
    
    void HealthCheckLoop();
    void ExecuteHealthCheck(HealthCheckInfo& check_info);
    void UpdateOverallHealth();
    HealthStatus DetermineOverallStatus() const;
    std::string HealthStatusToString(HealthStatus status) const;
    
    // Built-in health checks
    static HealthCheckResult CheckSystemMemory();
    static HealthCheckResult CheckSystemDisk();
    static HealthCheckResult CheckSystemCPU();
};

/**
 * @brief HTTP server for health check endpoints
 */
class HealthHttpServer
{
public:
    /**
     * @brief Constructor
     * @param health_service Reference to health service
     * @param bind_address Address to bind to
     * @param port Port to listen on
     */
    HealthHttpServer(HealthService& health_service, 
                     const std::string& bind_address = "127.0.0.1",
                     uint16_t port = 8080);

    /**
     * @brief Destructor
     */
    ~HealthHttpServer();

    /**
     * @brief Start the HTTP server
     * @return true if started successfully
     */
    bool Start();

    /**
     * @brief Stop the HTTP server
     */
    void Stop();

    /**
     * @brief Check if server is running
     */
    bool IsRunning() const { return running_.load(); }

private:
    HealthService& health_service_;
    std::string bind_address_;
    uint16_t port_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;

    void ServerLoop();
};

} // namespace monitoring
} // namespace neo