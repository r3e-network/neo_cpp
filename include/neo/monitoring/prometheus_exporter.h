#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <sstream>
#include <atomic>
#include <chrono>
#include <functional>
#include <httplib.h>

namespace neo::monitoring {

/**
 * @brief Prometheus metric types
 */
enum class MetricType {
    COUNTER,
    GAUGE,
    HISTOGRAM,
    SUMMARY
};

/**
 * @brief Base class for Prometheus metrics
 */
class Metric {
public:
    Metric(const std::string& name, const std::string& help, MetricType type)
        : name_(name), help_(help), type_(type) {
    }
    
    virtual ~Metric() = default;
    
    /**
     * @brief Serialize metric to Prometheus format
     * @return Prometheus formatted string
     */
    virtual std::string Serialize() const = 0;
    
    const std::string& GetName() const { return name_; }
    const std::string& GetHelp() const { return help_; }
    MetricType GetType() const { return type_; }
    
protected:
    std::string TypeString() const {
        switch (type_) {
            case MetricType::COUNTER: return "counter";
            case MetricType::GAUGE: return "gauge";
            case MetricType::HISTOGRAM: return "histogram";
            case MetricType::SUMMARY: return "summary";
        }
        return "untyped";
    }
    
    std::string name_;
    std::string help_;
    MetricType type_;
};

/**
 * @brief Counter metric (monotonically increasing)
 */
class Counter : public Metric {
public:
    Counter(const std::string& name, const std::string& help)
        : Metric(name, help, MetricType::COUNTER), value_(0) {
    }
    
    void Increment(double value = 1.0) {
        value_ += value;
    }
    
    double Get() const {
        return value_.load();
    }
    
    std::string Serialize() const override {
        std::stringstream ss;
        ss << "# HELP " << name_ << " " << help_ << "\n";
        ss << "# TYPE " << name_ << " " << TypeString() << "\n";
        ss << name_ << " " << value_.load() << "\n";
        return ss.str();
    }
    
private:
    std::atomic<double> value_;
};

/**
 * @brief Gauge metric (can go up and down)
 */
class Gauge : public Metric {
public:
    Gauge(const std::string& name, const std::string& help)
        : Metric(name, help, MetricType::GAUGE), value_(0) {
    }
    
    void Set(double value) {
        value_ = value;
    }
    
    void Increment(double value = 1.0) {
        value_ += value;
    }
    
    void Decrement(double value = 1.0) {
        value_ -= value;
    }
    
    double Get() const {
        return value_.load();
    }
    
    std::string Serialize() const override {
        std::stringstream ss;
        ss << "# HELP " << name_ << " " << help_ << "\n";
        ss << "# TYPE " << name_ << " " << TypeString() << "\n";
        ss << name_ << " " << value_.load() << "\n";
        return ss.str();
    }
    
private:
    std::atomic<double> value_;
};

/**
 * @brief Histogram metric with buckets
 */
class Histogram : public Metric {
public:
    Histogram(const std::string& name, const std::string& help,
              const std::vector<double>& buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10})
        : Metric(name, help, MetricType::HISTOGRAM), buckets_(buckets) {
        
        // Initialize bucket counters
        for (size_t i = 0; i <= buckets.size(); ++i) {
            bucketCounts_.push_back(0);
        }
    }
    
    void Observe(double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        sum_ += value;
        count_++;
        
        // Find bucket
        size_t i = 0;
        for (; i < buckets_.size(); ++i) {
            if (value <= buckets_[i]) {
                break;
            }
        }
        
        // Increment all buckets >= value
        for (size_t j = i; j <= buckets_.size(); ++j) {
            bucketCounts_[j]++;
        }
    }
    
    std::string Serialize() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::stringstream ss;
        ss << "# HELP " << name_ << " " << help_ << "\n";
        ss << "# TYPE " << name_ << " " << TypeString() << "\n";
        
        // Buckets
        for (size_t i = 0; i < buckets_.size(); ++i) {
            ss << name_ << "_bucket{le=\"" << buckets_[i] << "\"} " << bucketCounts_[i] << "\n";
        }
        ss << name_ << "_bucket{le=\"+Inf\"} " << bucketCounts_[buckets_.size()] << "\n";
        
        // Sum and count
        ss << name_ << "_sum " << sum_ << "\n";
        ss << name_ << "_count " << count_ << "\n";
        
        return ss.str();
    }
    
private:
    std::vector<double> buckets_;
    std::vector<uint64_t> bucketCounts_;
    double sum_ = 0;
    uint64_t count_ = 0;
    mutable std::mutex mutex_;
};

/**
 * @brief Labeled metric wrapper
 */
template<typename MetricT>
class LabeledMetric {
public:
    LabeledMetric(const std::string& name, const std::string& help,
                  const std::vector<std::string>& labelNames)
        : name_(name), help_(help), labelNames_(labelNames) {
    }
    
    MetricT& WithLabels(const std::vector<std::string>& labelValues) {
        if (labelValues.size() != labelNames_.size()) {
            throw std::invalid_argument("Label count mismatch");
        }
        
        std::string key = CreateKey(labelValues);
        
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(key);
        if (it == metrics_.end()) {
            auto metric = std::make_unique<MetricT>(name_, help_);
            auto ptr = metric.get();
            metrics_[key] = {std::move(metric), labelValues};
            return *ptr;
        }
        
        return *it->second.metric;
    }
    
    std::string Serialize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (metrics_.empty()) return "";
        
        std::stringstream ss;
        
        // Write help and type once
        bool first = true;
        for (const auto& [key, entry] : metrics_) {
            if (first) {
                ss << "# HELP " << name_ << " " << help_ << "\n";
                ss << "# TYPE " << name_ << " " << entry.metric->TypeString() << "\n";
                first = false;
            }
            
            // Serialize with labels
            std::string labels = CreateLabelString(entry.labelValues);
            std::string serialized = entry.metric->Serialize();
            
            // Extract value from serialized string
            size_t lastNewline = serialized.find_last_of('\n', serialized.length() - 2);
            std::string valueLine = serialized.substr(lastNewline + 1);
            size_t spacePos = valueLine.find(' ');
            
            if (spacePos != std::string::npos) {
                std::string value = valueLine.substr(spacePos + 1);
                ss << name_ << "{" << labels << "} " << value;
            }
        }
        
        return ss.str();
    }
    
private:
    std::string CreateKey(const std::vector<std::string>& labelValues) const {
        std::stringstream ss;
        for (size_t i = 0; i < labelValues.size(); ++i) {
            if (i > 0) ss << ",";
            ss << labelNames_[i] << "=" << labelValues[i];
        }
        return ss.str();
    }
    
    std::string CreateLabelString(const std::vector<std::string>& labelValues) const {
        std::stringstream ss;
        for (size_t i = 0; i < labelNames_.size(); ++i) {
            if (i > 0) ss << ",";
            ss << labelNames_[i] << "=\"" << labelValues[i] << "\"";
        }
        return ss.str();
    }
    
    struct MetricEntry {
        std::unique_ptr<MetricT> metric;
        std::vector<std::string> labelValues;
    };
    
    std::string name_;
    std::string help_;
    std::vector<std::string> labelNames_;
    std::unordered_map<std::string, MetricEntry> metrics_;
    mutable std::mutex mutex_;
};

/**
 * @brief Prometheus metrics exporter
 */
class PrometheusExporter {
public:
    static PrometheusExporter& GetInstance() {
        static PrometheusExporter instance;
        return instance;
    }
    
    /**
     * @brief Register a metric
     * @param metric Metric to register
     */
    void RegisterMetric(std::shared_ptr<Metric> metric) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_[metric->GetName()] = metric;
    }
    
    /**
     * @brief Start HTTP server for metrics export
     * @param port Port to listen on
     */
    void StartServer(uint16_t port = 9090) {
        if (server_) return;
        
        server_ = std::make_unique<httplib::Server>();
        
        server_->Get("/metrics", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(CollectMetrics(), "text/plain; version=0.0.4");
        });
        
        server_->Get("/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("OK", "text/plain");
        });
        
        serverThread_ = std::thread([this, port]() {
            server_->listen("0.0.0.0", port);
        });
    }
    
    /**
     * @brief Stop HTTP server
     */
    void StopServer() {
        if (server_) {
            server_->stop();
            if (serverThread_.joinable()) {
                serverThread_.join();
            }
            server_.reset();
        }
    }
    
    /**
     * @brief Collect all metrics in Prometheus format
     * @return Prometheus formatted metrics
     */
    std::string CollectMetrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::stringstream ss;
        
        // Add standard process metrics
        AddProcessMetrics(ss);
        
        // Add registered metrics
        for (const auto& [name, metric] : metrics_) {
            ss << metric->Serialize() << "\n";
        }
        
        return ss.str();
    }
    
    ~PrometheusExporter() {
        StopServer();
    }
    
private:
    PrometheusExporter() = default;
    
    void AddProcessMetrics(std::stringstream& ss) const {
        // Process start time
        static auto startTime = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        ss << "# HELP process_uptime_seconds Time since process start\n";
        ss << "# TYPE process_uptime_seconds gauge\n";
        ss << "process_uptime_seconds " << uptime << "\n\n";
        
        // Current time
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        ss << "# HELP process_start_time_seconds Unix timestamp of process start\n";
        ss << "# TYPE process_start_time_seconds gauge\n";
        ss << "process_start_time_seconds " << (timestamp / 1000.0 - uptime) << "\n\n";
    }
    
    std::unordered_map<std::string, std::shared_ptr<Metric>> metrics_;
    mutable std::mutex mutex_;
    
    std::unique_ptr<httplib::Server> server_;
    std::thread serverThread_;
};

/**
 * @brief Convenience macros for metric creation
 */
#define PROMETHEUS_COUNTER(name, help) \
    std::make_shared<neo::monitoring::Counter>(name, help)

#define PROMETHEUS_GAUGE(name, help) \
    std::make_shared<neo::monitoring::Gauge>(name, help)

#define PROMETHEUS_HISTOGRAM(name, help, ...) \
    std::make_shared<neo::monitoring::Histogram>(name, help, ##__VA_ARGS__)

#define PROMETHEUS_LABELED_COUNTER(name, help, labels) \
    std::make_shared<neo::monitoring::LabeledMetric<neo::monitoring::Counter>>(name, help, labels)

#define PROMETHEUS_LABELED_GAUGE(name, help, labels) \
    std::make_shared<neo::monitoring::LabeledMetric<neo::monitoring::Gauge>>(name, help, labels)

#define PROMETHEUS_LABELED_HISTOGRAM(name, help, labels, ...) \
    std::make_shared<neo::monitoring::LabeledMetric<neo::monitoring::Histogram>>(name, help, labels, ##__VA_ARGS__)

} // namespace neo::monitoring