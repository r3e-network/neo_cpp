#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <neo/core/logging.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::monitoring
{
/**
 * @brief Metric types supported
 */
enum class MetricType
{
    Counter,
    Gauge,
    Histogram,
    Summary
};

/**
 * @brief Base class for all metrics
 */
class Metric
{
  protected:
    std::string name_;
    std::string description_;
    std::unordered_map<std::string, std::string> labels_;
    MetricType type_;

  public:
    Metric(const std::string& name, const std::string& description, MetricType type)
        : name_(name), description_(description), type_(type)
    {
    }

    virtual ~Metric() = default;

    const std::string& GetName() const
    {
        return name_;
    }
    const std::string& GetDescription() const
    {
        return description_;
    }
    MetricType GetType() const
    {
        return type_;
    }

    void SetLabel(const std::string& key, const std::string& value)
    {
        labels_[key] = value;
    }

    virtual std::string ToPrometheus() const = 0;
};

/**
 * @brief Counter metric - monotonically increasing value
 */
class Counter : public Metric
{
  private:
    std::atomic<uint64_t> value_{0};

  public:
    Counter(const std::string& name, const std::string& description) : Metric(name, description, MetricType::Counter) {}

    void Increment(uint64_t delta = 1)
    {
        value_.fetch_add(delta, std::memory_order_relaxed);
    }

    uint64_t Get() const
    {
        return value_.load(std::memory_order_relaxed);
    }

    std::string ToPrometheus() const override;
};

/**
 * @brief Gauge metric - value that can go up or down
 */
class Gauge : public Metric
{
  private:
    std::atomic<double> value_{0.0};

  public:
    Gauge(const std::string& name, const std::string& description) : Metric(name, description, MetricType::Gauge) {}

    void Set(double value)
    {
        value_.store(value, std::memory_order_relaxed);
    }

    void Increment(double delta = 1.0)
    {
        double current;
        double desired;
        do
        {
            current = value_.load(std::memory_order_relaxed);
            desired = current + delta;
        } while (!value_.compare_exchange_weak(current, desired));
    }

    void Decrement(double delta = 1.0)
    {
        Increment(-delta);
    }

    double Get() const
    {
        return value_.load(std::memory_order_relaxed);
    }

    std::string ToPrometheus() const override;
};

/**
 * @brief Histogram metric - distribution of values
 */
class Histogram : public Metric
{
  private:
    mutable std::mutex mutex_;
    std::vector<double> buckets_;
    std::vector<uint64_t> bucket_counts_;
    uint64_t count_{0};
    double sum_{0.0};

  public:
    Histogram(const std::string& name, const std::string& description,
              const std::vector<double>& buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10})
        : Metric(name, description, MetricType::Histogram), buckets_(buckets), bucket_counts_(buckets.size() + 1, 0)
    {
    }

    void Observe(double value);
    std::string ToPrometheus() const override;
};

/**
 * @brief Timer for measuring durations
 */
class Timer
{
  private:
    std::shared_ptr<Histogram> histogram_;
    std::chrono::steady_clock::time_point start_;

  public:
    explicit Timer(std::shared_ptr<Histogram> histogram)
        : histogram_(histogram), start_(std::chrono::steady_clock::now())
    {
    }

    ~Timer()
    {
        Stop();
    }

    void Stop()
    {
        if (histogram_)
        {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start_).count();
            histogram_->Observe(duration);
            histogram_.reset();
        }
    }
};

/**
 * @brief Metrics registry for managing all metrics
 */
class MetricsRegistry
{
  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Metric>> metrics_;
    static std::shared_ptr<MetricsRegistry> instance_;
    static std::mutex instance_mutex_;

    MetricsRegistry() = default;

  public:
    static std::shared_ptr<MetricsRegistry> GetInstance();

    template <typename T>
    std::shared_ptr<T> Register(const std::string& name, const std::string& description)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = metrics_.find(name);
        if (it != metrics_.end())
        {
            return std::dynamic_pointer_cast<T>(it->second);
        }

        auto metric = std::make_shared<T>(name, description);
        metrics_[name] = metric;
        return metric;
    }

    std::shared_ptr<Metric> Get(const std::string& name) const;

    /**
     * @brief Export all metrics in Prometheus format
     */
    std::string ExportPrometheus() const;

    /**
     * @brief Export all metrics as JSON
     */
    std::string ExportJson() const;

    /**
     * @brief Clear all metrics
     */
    void Clear();
};

/**
 * @brief Health check status
 */
enum class HealthStatus
{
    Healthy,
    Degraded,
    Unhealthy
};

/**
 * @brief Health check result
 */
struct HealthCheckResult
{
    HealthStatus status;
    std::string message;
    std::chrono::milliseconds duration;
    std::unordered_map<std::string, std::string> details;
};

/**
 * @brief Health check interface
 */
class IHealthCheck
{
  public:
    virtual ~IHealthCheck() = default;
    virtual HealthCheckResult Check() = 0;
    virtual std::string GetName() const = 0;
};

/**
 * @brief Health check registry
 */
class HealthCheckRegistry
{
  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<IHealthCheck>> checks_;
    static std::shared_ptr<HealthCheckRegistry> instance_;
    static std::mutex instance_mutex_;

    HealthCheckRegistry() = default;

  public:
    static std::shared_ptr<HealthCheckRegistry> GetInstance();

    void Register(const std::string& name, std::shared_ptr<IHealthCheck> check);
    void Unregister(const std::string& name);

    /**
     * @brief Run all health checks
     */
    std::unordered_map<std::string, HealthCheckResult> RunAll() const;

    /**
     * @brief Get overall health status
     */
    HealthStatus GetOverallStatus() const;

    /**
     * @brief Export health status as JSON
     */
    std::string ExportJson() const;
};

// Convenience macros for metrics
#define METRICS_COUNTER(name, description)                                                                             \
    neo::monitoring::MetricsRegistry::GetInstance()->Register<neo::monitoring::Counter>(name, description)

#define METRICS_GAUGE(name, description)                                                                               \
    neo::monitoring::MetricsRegistry::GetInstance()->Register<neo::monitoring::Gauge>(name, description)

#define METRICS_HISTOGRAM(name, description)                                                                           \
    neo::monitoring::MetricsRegistry::GetInstance()->Register<neo::monitoring::Histogram>(name, description)

#define METRICS_TIMER(histogram) neo::monitoring::Timer timer(histogram)
}  // namespace neo::monitoring