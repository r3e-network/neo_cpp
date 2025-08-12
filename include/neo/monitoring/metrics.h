#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace neo {
namespace monitoring {

/**
 * @brief Simple metrics collector for Neo node monitoring
 * Compatible with Prometheus exposition format
 */
class MetricsCollector {
public:
    static MetricsCollector& Instance() {
        static MetricsCollector instance;
        return instance;
    }

    // Counter metrics (only increase)
    void IncrementCounter(const std::string& name, double value = 1.0);
    double GetCounter(const std::string& name) const;

    // Gauge metrics (can increase or decrease)
    void SetGauge(const std::string& name, double value);
    double GetGauge(const std::string& name) const;

    // Histogram metrics (for timing and distributions)
    void RecordHistogram(const std::string& name, double value);
    
    // Summary metrics
    struct Summary {
        size_t count = 0;
        double sum = 0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
    };
    Summary GetSummary(const std::string& name) const;

    // Export metrics in Prometheus format
    std::string ExportPrometheusFormat() const;

    // Clear all metrics
    void Reset();

    // Common Neo metrics
    void RecordBlockHeight(uint32_t height);
    void RecordTransactionCount(size_t count);
    void RecordPeerCount(size_t count);
    void RecordMemoryPoolSize(size_t size);
    void RecordBlockProcessingTime(double milliseconds);
    void RecordRPCRequestDuration(const std::string& method, double milliseconds);
    void IncrementRPCRequestCount(const std::string& method);
    void IncrementConsensusRound();
    void RecordVMExecutionTime(double milliseconds);
    void RecordStorageOperations(const std::string& operation, size_t count);

private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    mutable std::mutex mutex_;
    std::map<std::string, std::atomic<double>> counters_;
    std::map<std::string, std::atomic<double>> gauges_;
    std::map<std::string, std::vector<double>> histograms_;
    std::map<std::string, Summary> summaries_;
};

/**
 * @brief RAII timer for measuring execution time
 */
class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& metric_name)
        : metric_name_(metric_name)
        , start_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count() / 1000.0;
        MetricsCollector::Instance().RecordHistogram(metric_name_, duration);
    }

private:
    std::string metric_name_;
    std::chrono::high_resolution_clock::time_point start_;
};

// Convenience macros for metrics
#define NEO_METRIC_INCREMENT(name) \
    neo::monitoring::MetricsCollector::Instance().IncrementCounter(name)

#define NEO_METRIC_GAUGE(name, value) \
    neo::monitoring::MetricsCollector::Instance().SetGauge(name, value)

#define NEO_METRIC_TIMER(name) \
    neo::monitoring::ScopedTimer _timer_##__LINE__(name)

} // namespace monitoring
} // namespace neo