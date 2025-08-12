#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace neo::monitoring {

/**
 * @brief Types of metrics supported
 */
enum class MetricType {
    Counter,    // Monotonically increasing value
    Gauge,      // Value that can go up or down
    Histogram,  // Distribution of values
    Summary     // Statistical distribution over time window
};

/**
 * @brief Base class for all metrics
 */
class Metric {
public:
    Metric(const std::string& name, const std::string& help, MetricType type)
        : name_(name), help_(help), type_(type) {}
    
    virtual ~Metric() = default;
    
    const std::string& GetName() const { return name_; }
    const std::string& GetHelp() const { return help_; }
    MetricType GetType() const { return type_; }
    
    virtual std::string ToPrometheus() const = 0;
    
protected:
    std::string name_;
    std::string help_;
    MetricType type_;
};

/**
 * @brief Counter metric - monotonically increasing value
 */
class Counter : public Metric {
public:
    Counter(const std::string& name, const std::string& help)
        : Metric(name, help, MetricType::Counter), value_(0) {}
    
    void Increment(double v = 1.0) {
        value_.fetch_add(v, std::memory_order_relaxed);
    }
    
    double GetValue() const {
        return value_.load(std::memory_order_relaxed);
    }
    
    void Reset() {
        value_.store(0, std::memory_order_relaxed);
    }
    
    std::string ToPrometheus() const override;
    
private:
    std::atomic<double> value_;
};

/**
 * @brief Gauge metric - value that can go up or down
 */
class Gauge : public Metric {
public:
    Gauge(const std::string& name, const std::string& help)
        : Metric(name, help, MetricType::Gauge), value_(0) {}
    
    void Set(double v) {
        value_.store(v, std::memory_order_relaxed);
    }
    
    void Increment(double v = 1.0) {
        value_.fetch_add(v, std::memory_order_relaxed);
    }
    
    void Decrement(double v = 1.0) {
        value_.fetch_sub(v, std::memory_order_relaxed);
    }
    
    double GetValue() const {
        return value_.load(std::memory_order_relaxed);
    }
    
    std::string ToPrometheus() const override;
    
private:
    std::atomic<double> value_;
};

/**
 * @brief Histogram metric - distribution of values
 */
class Histogram : public Metric {
public:
    Histogram(const std::string& name, const std::string& help,
              const std::vector<double>& buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10});
    
    void Observe(double value);
    
    std::string ToPrometheus() const override;
    
private:
    std::vector<double> buckets_;
    std::vector<std::atomic<uint64_t>> bucket_counts_;
    std::atomic<double> sum_;
    std::atomic<uint64_t> count_;
    mutable std::mutex mutex_;
};

/**
 * @brief Summary metric - statistical distribution over time window
 */
class Summary : public Metric {
public:
    Summary(const std::string& name, const std::string& help);
    
    void Observe(double value);
    
    std::string ToPrometheus() const override;
    
private:
    struct Quantile {
        double quantile;
        double value;
    };
    
    mutable std::mutex mutex_;
    std::vector<double> observations_;
    std::atomic<double> sum_;
    std::atomic<uint64_t> count_;
    
    std::vector<Quantile> CalculateQuantiles() const;
};

/**
 * @brief Central metrics collector and registry
 */
class MetricsCollector {
public:
    static MetricsCollector& GetInstance() {
        static MetricsCollector instance;
        return instance;
    }
    
    // Register metrics
    std::shared_ptr<Counter> RegisterCounter(const std::string& name, const std::string& help);
    std::shared_ptr<Gauge> RegisterGauge(const std::string& name, const std::string& help);
    std::shared_ptr<Histogram> RegisterHistogram(const std::string& name, const std::string& help,
                                                 const std::vector<double>& buckets = {});
    std::shared_ptr<Summary> RegisterSummary(const std::string& name, const std::string& help);
    
    // Get existing metrics
    std::shared_ptr<Counter> GetCounter(const std::string& name);
    std::shared_ptr<Gauge> GetGauge(const std::string& name);
    std::shared_ptr<Histogram> GetHistogram(const std::string& name);
    std::shared_ptr<Summary> GetSummary(const std::string& name);
    
    // Export metrics in Prometheus format
    std::string ExportPrometheus() const;
    
    // Export metrics in JSON format
    std::string ExportJSON() const;
    
    // Clear all metrics
    void Clear();
    
private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<Metric>> metrics_;
};

/**
 * @brief RAII timer for measuring durations
 */
class ScopedTimer {
public:
    ScopedTimer(std::shared_ptr<Histogram> histogram)
        : histogram_(histogram),
          start_(std::chrono::high_resolution_clock::now()) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start_);
        if (histogram_) {
            histogram_->Observe(duration.count());
        }
    }
    
private:
    std::shared_ptr<Histogram> histogram_;
    std::chrono::high_resolution_clock::time_point start_;
};

// Convenience macros for metrics
#define METRIC_INCREMENT(name) \
    if (auto counter = MetricsCollector::GetInstance().GetCounter(name)) { \
        counter->Increment(); \
    }

#define METRIC_GAUGE_SET(name, value) \
    if (auto gauge = MetricsCollector::GetInstance().GetGauge(name)) { \
        gauge->Set(value); \
    }

#define METRIC_OBSERVE(name, value) \
    if (auto histogram = MetricsCollector::GetInstance().GetHistogram(name)) { \
        histogram->Observe(value); \
    }

#define METRIC_TIMER(name) \
    ScopedTimer _timer(MetricsCollector::GetInstance().GetHistogram(name))

} // namespace neo::monitoring