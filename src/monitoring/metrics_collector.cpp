/**
 * @file metrics_collector.cpp
 * @brief Performance metrics collection
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/monitoring/metrics_collector.h>
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <nlohmann/json.hpp>

namespace neo::monitoring {

// Counter implementation
std::string Counter::ToPrometheus() const {
    std::stringstream ss;
    ss << "# HELP " << name_ << " " << help_ << "\n";
    ss << "# TYPE " << name_ << " counter\n";
    ss << name_ << " " << std::fixed << std::setprecision(6) << GetValue() << "\n";
    return ss.str();
}

// Gauge implementation
std::string Gauge::ToPrometheus() const {
    std::stringstream ss;
    ss << "# HELP " << name_ << " " << help_ << "\n";
    ss << "# TYPE " << name_ << " gauge\n";
    ss << name_ << " " << std::fixed << std::setprecision(6) << GetValue() << "\n";
    return ss.str();
}

// Histogram implementation
Histogram::Histogram(const std::string& name, const std::string& help,
                     const std::vector<double>& buckets)
    : Metric(name, help, MetricType::Histogram),
      buckets_(buckets),
      bucket_counts_(buckets.size() + 1), // +1 for +Inf bucket
      sum_(0),
      count_(0) {
    
    // Ensure buckets are sorted
    std::sort(buckets_.begin(), buckets_.end());
    
    // Initialize all counts to 0
    for (auto& count : bucket_counts_) {
        count.store(0);
    }
}

void Histogram::Observe(double value) {
    // Find the appropriate bucket
    size_t bucket_idx = buckets_.size(); // Default to +Inf bucket
    for (size_t i = 0; i < buckets_.size(); ++i) {
        if (value <= buckets_[i]) {
            bucket_idx = i;
            break;
        }
    }
    
    // Increment all buckets >= this value
    for (size_t i = bucket_idx; i < bucket_counts_.size(); ++i) {
        bucket_counts_[i].fetch_add(1);
    }
    
    // Update sum and count
    sum_.fetch_add(value);
    count_.fetch_add(1);
}

std::string Histogram::ToPrometheus() const {
    std::stringstream ss;
    ss << "# HELP " << name_ << " " << help_ << "\n";
    ss << "# TYPE " << name_ << " histogram\n";
    
    // Output bucket counts
    for (size_t i = 0; i < buckets_.size(); ++i) {
        ss << name_ << "_bucket{le=\"" << buckets_[i] << "\"} " 
           << bucket_counts_[i].load() << "\n";
    }
    
    // Output +Inf bucket
    ss << name_ << "_bucket{le=\"+Inf\"} " 
       << bucket_counts_.back().load() << "\n";
    
    // Output sum and count
    ss << name_ << "_sum " << std::fixed << std::setprecision(6) 
       << sum_.load() << "\n";
    ss << name_ << "_count " << count_.load() << "\n";
    
    return ss.str();
}

// Summary implementation
Summary::Summary(const std::string& name, const std::string& help)
    : Metric(name, help, MetricType::Summary),
      sum_(0),
      count_(0) {}

void Summary::Observe(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    observations_.push_back(value);
    sum_.fetch_add(value);
    count_.fetch_add(1);
    
    // Keep only last 10000 observations for quantile calculation
    if (observations_.size() > 10000) {
        observations_.erase(observations_.begin(), 
                           observations_.begin() + observations_.size() - 10000);
    }
}

std::vector<Summary::Quantile> Summary::CalculateQuantiles() const {
    if (observations_.empty()) {
        return {};
    }
    
    std::vector<double> sorted = observations_;
    std::sort(sorted.begin(), sorted.end());
    
    std::vector<Quantile> quantiles;
    std::vector<double> percentiles = {0.5, 0.9, 0.99};
    
    for (double p : percentiles) {
        size_t idx = static_cast<size_t>(p * sorted.size());
        if (idx >= sorted.size()) idx = sorted.size() - 1;
        quantiles.push_back({p, sorted[idx]});
    }
    
    return quantiles;
}

std::string Summary::ToPrometheus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "# HELP " << name_ << " " << help_ << "\n";
    ss << "# TYPE " << name_ << " summary\n";
    
    // Output quantiles
    auto quantiles = CalculateQuantiles();
    for (const auto& q : quantiles) {
        ss << name_ << "{quantile=\"" << q.quantile << "\"} " 
           << std::fixed << std::setprecision(6) << q.value << "\n";
    }
    
    // Output sum and count
    ss << name_ << "_sum " << std::fixed << std::setprecision(6) 
       << sum_.load() << "\n";
    ss << name_ << "_count " << count_.load() << "\n";
    
    return ss.str();
}

// MetricsCollector implementation
std::shared_ptr<Counter> MetricsCollector::RegisterCounter(const std::string& name, 
                                                          const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return std::dynamic_pointer_cast<Counter>(it->second);
    }
    
    auto counter = std::make_shared<Counter>(name, help);
    metrics_[name] = counter;
    return counter;
}

std::shared_ptr<Gauge> MetricsCollector::RegisterGauge(const std::string& name,
                                                       const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return std::dynamic_pointer_cast<Gauge>(it->second);
    }
    
    auto gauge = std::make_shared<Gauge>(name, help);
    metrics_[name] = gauge;
    return gauge;
}

std::shared_ptr<Histogram> MetricsCollector::RegisterHistogram(const std::string& name,
                                                              const std::string& help,
                                                              const std::vector<double>& buckets) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return std::dynamic_pointer_cast<Histogram>(it->second);
    }
    
    auto histogram = buckets.empty() 
        ? std::make_shared<Histogram>(name, help)
        : std::make_shared<Histogram>(name, help, buckets);
    metrics_[name] = histogram;
    return histogram;
}

std::shared_ptr<Summary> MetricsCollector::RegisterSummary(const std::string& name,
                                                          const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return std::dynamic_pointer_cast<Summary>(it->second);
    }
    
    auto summary = std::make_shared<Summary>(name, help);
    metrics_[name] = summary;
    return summary;
}

std::shared_ptr<Counter> MetricsCollector::GetCounter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return std::dynamic_pointer_cast<Counter>(it->second);
    }
    return nullptr;
}

std::shared_ptr<Gauge> MetricsCollector::GetGauge(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return std::dynamic_pointer_cast<Gauge>(it->second);
    }
    return nullptr;
}

std::shared_ptr<Histogram> MetricsCollector::GetHistogram(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return std::dynamic_pointer_cast<Histogram>(it->second);
    }
    return nullptr;
}

std::shared_ptr<Summary> MetricsCollector::GetSummary(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return std::dynamic_pointer_cast<Summary>(it->second);
    }
    return nullptr;
}

std::string MetricsCollector::ExportPrometheus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    for (const auto& [name, metric] : metrics_) {
        ss << metric->ToPrometheus();
    }
    return ss.str();
}

std::string MetricsCollector::ExportJSON() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json j;
    
    for (const auto& [name, metric] : metrics_) {
        nlohmann::json metric_json;
        metric_json["name"] = metric->GetName();
        metric_json["help"] = metric->GetHelp();
        metric_json["type"] = static_cast<int>(metric->GetType());
        
        // Add type-specific values
        if (auto counter = std::dynamic_pointer_cast<Counter>(metric)) {
            metric_json["value"] = counter->GetValue();
        } else if (auto gauge = std::dynamic_pointer_cast<Gauge>(metric)) {
            metric_json["value"] = gauge->GetValue();
        }
        // Note: Histogram and Summary would need more complex JSON representation
        
        j["metrics"].push_back(metric_json);
    }
    
    return j.dump(2);
}

void MetricsCollector::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.clear();
}

} // namespace neo::monitoring