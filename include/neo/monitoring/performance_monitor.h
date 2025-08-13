/**
 * @file performance_monitor.h
 * @brief Comprehensive performance monitoring and metrics collection
 */

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <functional>

namespace neo {
namespace monitoring {

/**
 * @brief Performance metrics for a specific operation
 */
struct OperationMetrics {
    std::atomic<uint64_t> count{0};
    std::atomic<uint64_t> total_duration_ms{0};
    std::atomic<uint64_t> min_duration_ms{UINT64_MAX};
    std::atomic<uint64_t> max_duration_ms{0};
    std::atomic<uint64_t> errors{0};
    std::atomic<uint64_t> last_duration_ms{0};
    std::chrono::steady_clock::time_point last_execution;
    
    OperationMetrics() = default;
    
    // Copy constructor for returning values
    OperationMetrics(const OperationMetrics& other) 
        : count(other.count.load())
        , total_duration_ms(other.total_duration_ms.load())
        , min_duration_ms(other.min_duration_ms.load())
        , max_duration_ms(other.max_duration_ms.load())
        , errors(other.errors.load())
        , last_duration_ms(other.last_duration_ms.load())
        , last_execution(other.last_execution) {}
    
    // Assignment operator
    OperationMetrics& operator=(const OperationMetrics& other) {
        if (this != &other) {
            count.store(other.count.load());
            total_duration_ms.store(other.total_duration_ms.load());
            min_duration_ms.store(other.min_duration_ms.load());
            max_duration_ms.store(other.max_duration_ms.load());
            errors.store(other.errors.load());
            last_duration_ms.store(other.last_duration_ms.load());
            last_execution = other.last_execution;
        }
        return *this;
    }
    
    double GetAverageDurationMs() const {
        uint64_t c = count.load();
        return c > 0 ? static_cast<double>(total_duration_ms.load()) / c : 0.0;
    }
    
    double GetErrorRate() const {
        uint64_t total = count.load() + errors.load();
        return total > 0 ? static_cast<double>(errors.load()) / total : 0.0;
    }
};

/**
 * @brief System-wide performance metrics
 */
struct SystemMetrics {
    // CPU metrics
    double cpu_usage_percent = 0.0;
    uint64_t thread_count = 0;
    
    // Memory metrics
    uint64_t memory_used_bytes = 0;
    uint64_t memory_available_bytes = 0;
    uint64_t heap_allocated_bytes = 0;
    
    // Network metrics
    uint64_t network_bytes_sent = 0;
    uint64_t network_bytes_received = 0;
    uint64_t active_connections = 0;
    uint64_t total_connections = 0;
    
    // Blockchain metrics
    uint32_t blockchain_height = 0;
    uint64_t total_transactions = 0;
    uint64_t blocks_per_second = 0;
    uint64_t transactions_per_second = 0;
    
    // Storage metrics
    uint64_t storage_read_ops = 0;
    uint64_t storage_write_ops = 0;
    uint64_t storage_size_bytes = 0;
    
    double GetMemoryUsagePercent() const {
        uint64_t total = memory_used_bytes + memory_available_bytes;
        return total > 0 ? (static_cast<double>(memory_used_bytes) / total) * 100.0 : 0.0;
    }
};

/**
 * @brief RAII timer for automatic performance measurement
 */
class ScopedTimer {
private:
    std::string operation_name_;
    std::chrono::steady_clock::time_point start_;
    std::function<void(const std::string&, uint64_t)> callback_;
    bool stopped_{false};
    
public:
    ScopedTimer(const std::string& operation_name, 
                std::function<void(const std::string&, uint64_t)> callback);
    ~ScopedTimer();
    
    void Stop();
    uint64_t GetElapsedMs() const;
};

/**
 * @brief Performance monitoring system
 */
class PerformanceMonitor {
public:
    static PerformanceMonitor& GetInstance() {
        static PerformanceMonitor instance;
        return instance;
    }
    
    /**
     * @brief Start monitoring
     */
    void Start();
    
    /**
     * @brief Stop monitoring
     */
    void Stop();
    
    /**
     * @brief Record an operation's performance
     * @param operation_name Name of the operation
     * @param duration_ms Duration in milliseconds
     * @param success Whether the operation succeeded
     */
    void RecordOperation(const std::string& operation_name, 
                        uint64_t duration_ms, 
                        bool success = true);
    
    /**
     * @brief Create a scoped timer for an operation
     * @param operation_name Name of the operation
     * @return Scoped timer that records on destruction
     */
    std::unique_ptr<ScopedTimer> CreateTimer(const std::string& operation_name);
    
    /**
     * @brief Record a custom metric
     * @param metric_name Name of the metric
     * @param value Metric value
     */
    void RecordMetric(const std::string& metric_name, double value);
    
    /**
     * @brief Get metrics for a specific operation
     * @param operation_name Name of the operation
     * @return Operation metrics
     */
    OperationMetrics GetOperationMetrics(const std::string& operation_name) const;
    
    /**
     * @brief Get all operation metrics
     * @return Map of operation names to metrics
     */
    std::unordered_map<std::string, OperationMetrics> GetAllOperationMetrics() const;
    
    /**
     * @brief Get system metrics
     * @return Current system metrics
     */
    SystemMetrics GetSystemMetrics() const;
    
    /**
     * @brief Update system metrics
     * @param metrics New system metrics
     */
    void UpdateSystemMetrics(const SystemMetrics& metrics);
    
    /**
     * @brief Get custom metrics
     * @return Map of metric names to values
     */
    std::unordered_map<std::string, double> GetCustomMetrics() const;
    
    /**
     * @brief Export metrics in Prometheus format
     * @return Metrics string in Prometheus format
     */
    std::string ExportPrometheusMetrics() const;
    
    /**
     * @brief Export metrics in JSON format
     * @return Metrics JSON string
     */
    std::string ExportJsonMetrics() const;
    
    /**
     * @brief Set alert threshold for an operation
     * @param operation_name Name of the operation
     * @param max_duration_ms Maximum allowed duration
     * @param max_error_rate Maximum allowed error rate
     */
    void SetAlertThreshold(const std::string& operation_name,
                          uint64_t max_duration_ms,
                          double max_error_rate);
    
    /**
     * @brief Register alert callback
     * @param callback Function to call when alert triggers
     */
    void RegisterAlertCallback(std::function<void(const std::string&, const std::string&)> callback);
    
    /**
     * @brief Clear all metrics
     */
    void ClearMetrics();
    
    /**
     * @brief Enable/disable detailed tracing
     * @param enabled Whether to enable tracing
     */
    void SetTracingEnabled(bool enabled);
    
    /**
     * @brief Check if tracing is enabled
     * @return True if tracing is enabled
     */
    bool IsTracingEnabled() const { return tracing_enabled_.load(); }
    
private:
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    // Prevent copying
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    
    void MonitoringThread();
    void CollectSystemMetrics();
    void CheckAlerts();
    void TriggerAlert(const std::string& alert_type, const std::string& message);
    
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, OperationMetrics> operation_metrics_;
    std::unordered_map<std::string, double> custom_metrics_;
    SystemMetrics system_metrics_;
    
    struct AlertThreshold {
        uint64_t max_duration_ms;
        double max_error_rate;
    };
    std::unordered_map<std::string, AlertThreshold> alert_thresholds_;
    std::vector<std::function<void(const std::string&, const std::string&)>> alert_callbacks_;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> tracing_enabled_{false};
    std::thread monitoring_thread_;
    
    std::chrono::steady_clock::time_point start_time_;
};

/**
 * @brief Convenience macro for timing operations
 */
#define MONITOR_OPERATION(name) \
    auto _timer = neo::monitoring::PerformanceMonitor::GetInstance().CreateTimer(name)

/**
 * @brief Record a metric value
 */
#define RECORD_METRIC(name, value) \
    neo::monitoring::PerformanceMonitor::GetInstance().RecordMetric(name, value)

/**
 * @brief Mark operation as failed
 */
#define MONITOR_OPERATION_FAILED(name) \
    neo::monitoring::PerformanceMonitor::GetInstance().RecordOperation(name, 0, false)

} // namespace monitoring
} // namespace neo