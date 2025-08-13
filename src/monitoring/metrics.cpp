/**
 * @file metrics.cpp
 * @brief Performance metrics collection
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/monitoring/metrics.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace neo {
namespace monitoring {

void MetricsCollector::IncrementCounter(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (counters_.find(name) == counters_.end()) {
        counters_[name] = 0.0;
    }
    counters_[name] += value;
}

double MetricsCollector::GetCounter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = counters_.find(name);
    return (it != counters_.end()) ? it->second.load() : 0.0;
}

void MetricsCollector::SetGauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    gauges_[name] = value;
}

double MetricsCollector::GetGauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    return (it != gauges_.end()) ? it->second.load() : 0.0;
}

void MetricsCollector::RecordHistogram(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    histograms_[name].push_back(value);
    
    // Update summary
    auto& summary = summaries_[name];
    summary.count++;
    summary.sum += value;
    summary.min = std::min(summary.min, value);
    summary.max = std::max(summary.max, value);
}

MetricsCollector::Summary MetricsCollector::GetSummary(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = summaries_.find(name);
    return (it != summaries_.end()) ? it->second : Summary{};
}

std::string MetricsCollector::ExportPrometheusFormat() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    
    // Export counters
    for (const auto& [name, value] : counters_) {
        std::string clean_name = name;
        std::replace(clean_name.begin(), clean_name.end(), '.', '_');
        std::replace(clean_name.begin(), clean_name.end(), '-', '_');
        ss << "# TYPE neo_" << clean_name << " counter\n";
        ss << "neo_" << clean_name << " " << std::fixed << std::setprecision(2) << value.load() << "\n";
    }
    
    // Export gauges
    for (const auto& [name, value] : gauges_) {
        std::string clean_name = name;
        std::replace(clean_name.begin(), clean_name.end(), '.', '_');
        std::replace(clean_name.begin(), clean_name.end(), '-', '_');
        ss << "# TYPE neo_" << clean_name << " gauge\n";
        ss << "neo_" << clean_name << " " << std::fixed << std::setprecision(2) << value.load() << "\n";
    }
    
    // Export summaries
    for (const auto& [name, summary] : summaries_) {
        std::string clean_name = name;
        std::replace(clean_name.begin(), clean_name.end(), '.', '_');
        std::replace(clean_name.begin(), clean_name.end(), '-', '_');
        ss << "# TYPE neo_" << clean_name << " summary\n";
        ss << "neo_" << clean_name << "_count " << summary.count << "\n";
        ss << "neo_" << clean_name << "_sum " << std::fixed << std::setprecision(2) << summary.sum << "\n";
        
        // Calculate percentiles if we have histogram data
        auto hist_it = histograms_.find(name);
        if (hist_it != histograms_.end() && !hist_it->second.empty()) {
            auto values = hist_it->second;
            std::sort(values.begin(), values.end());
            
            // P50 (median)
            size_t p50_idx = values.size() / 2;
            ss << "neo_" << clean_name << "{quantile=\"0.5\"} " << values[p50_idx] << "\n";
            
            // P90
            size_t p90_idx = (values.size() * 90) / 100;
            ss << "neo_" << clean_name << "{quantile=\"0.9\"} " << values[p90_idx] << "\n";
            
            // P99
            size_t p99_idx = (values.size() * 99) / 100;
            ss << "neo_" << clean_name << "{quantile=\"0.99\"} " << values[p99_idx] << "\n";
        }
    }
    
    return ss.str();
}

void MetricsCollector::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    histograms_.clear();
    summaries_.clear();
}

// Common Neo metrics implementations
void MetricsCollector::RecordBlockHeight(uint32_t height) {
    SetGauge("block.height", height);
}

void MetricsCollector::RecordTransactionCount(size_t count) {
    IncrementCounter("transactions.total", count);
}

void MetricsCollector::RecordPeerCount(size_t count) {
    SetGauge("peers.connected", count);
}

void MetricsCollector::RecordMemoryPoolSize(size_t size) {
    SetGauge("mempool.size", size);
}

void MetricsCollector::RecordBlockProcessingTime(double milliseconds) {
    RecordHistogram("block.processing.time", milliseconds);
}

void MetricsCollector::RecordRPCRequestDuration(const std::string& method, double milliseconds) {
    RecordHistogram("rpc.request.duration." + method, milliseconds);
}

void MetricsCollector::IncrementRPCRequestCount(const std::string& method) {
    IncrementCounter("rpc.requests." + method);
}

void MetricsCollector::IncrementConsensusRound() {
    IncrementCounter("consensus.rounds");
}

void MetricsCollector::RecordVMExecutionTime(double milliseconds) {
    RecordHistogram("vm.execution.time", milliseconds);
}

void MetricsCollector::RecordStorageOperations(const std::string& operation, size_t count) {
    IncrementCounter("storage.operations." + operation, count);
}

} // namespace monitoring
} // namespace neo