#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>

namespace neo {
namespace common {

/**
 * Common configuration structure used across the system
 * Consolidates multiple Config structs to avoid duplication
 */
struct Configuration {
    // Common configuration fields
    std::string name;
    bool enabled = true;
    uint32_t version = 1;
    
    // Network configuration
    struct Network {
        std::string host = "127.0.0.1";
        uint16_t port = 10333;
        uint32_t max_connections = 100;
        uint32_t timeout_ms = 30000;
    } network;
    
    // Storage configuration
    struct Storage {
        std::string path = "./data";
        uint64_t max_size_bytes = 1073741824; // 1GB
        bool enable_compression = true;
    } storage;
    
    // Performance configuration
    struct Performance {
        uint32_t thread_pool_size = 4;
        uint32_t cache_size_mb = 256;
        bool enable_metrics = true;
    } performance;
};

/**
 * Common statistics structure
 * Consolidates multiple Stats structs to avoid duplication
 */
struct Statistics {
    // Timing statistics
    struct Timing {
        uint64_t total_requests = 0;
        uint64_t total_time_ms = 0;
        uint64_t min_time_ms = UINT64_MAX;
        uint64_t max_time_ms = 0;
        double average_time_ms = 0.0;
    } timing;
    
    // Resource statistics
    struct Resources {
        uint64_t memory_bytes = 0;
        uint64_t disk_bytes = 0;
        double cpu_percent = 0.0;
        uint32_t thread_count = 0;
    } resources;
    
    // Error statistics
    struct Errors {
        uint64_t total_errors = 0;
        uint64_t connection_errors = 0;
        uint64_t timeout_errors = 0;
        uint64_t validation_errors = 0;
    } errors;
    
    // Update statistics
    void Update(uint64_t time_ms, bool success = true) {
        timing.total_requests++;
        timing.total_time_ms += time_ms;
        timing.min_time_ms = std::min(timing.min_time_ms, time_ms);
        timing.max_time_ms = std::max(timing.max_time_ms, time_ms);
        timing.average_time_ms = static_cast<double>(timing.total_time_ms) / timing.total_requests;
        
        if (!success) {
            errors.total_errors++;
        }
    }
    
    void Reset() {
        *this = Statistics();
    }
};

/**
 * Common peer information structure
 * Consolidates multiple PeerInfo structs
 */
struct PeerInfo {
    std::string address;
    uint16_t port;
    std::string node_id;
    uint32_t version;
    uint64_t last_seen_timestamp;
    bool is_connected;
    
    // Network statistics
    uint64_t bytes_sent = 0;
    uint64_t bytes_received = 0;
    uint32_t latency_ms = 0;
    
    // Capabilities
    std::vector<std::string> capabilities;
    
    std::string GetEndpoint() const {
        return address + ":" + std::to_string(port);
    }
};

/**
 * Common result type for operations
 */
template<typename T>
class Result {
public:
    Result(T value) : success_(true), value_(std::move(value)) {}
    Result(std::string error) : success_(false), error_(std::move(error)) {}
    
    bool IsSuccess() const { return success_; }
    bool IsError() const { return !success_; }
    
    const T& Value() const { 
        if (!success_) throw std::runtime_error("Result is error: " + error_);
        return value_; 
    }
    
    T& Value() { 
        if (!success_) throw std::runtime_error("Result is error: " + error_);
        return value_; 
    }
    
    const std::string& Error() const { return error_; }
    
private:
    bool success_;
    T value_;
    std::string error_;
};

/**
 * Common metrics structure
 */
struct Metrics {
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::milliseconds;
    
    struct Counter {
        std::string name;
        uint64_t value = 0;
        
        void Increment(uint64_t amount = 1) { value += amount; }
        void Reset() { value = 0; }
    };
    
    struct Gauge {
        std::string name;
        double value = 0.0;
        
        void Set(double val) { value = val; }
        void Increment(double amount = 1.0) { value += amount; }
        void Decrement(double amount = 1.0) { value -= amount; }
    };
    
    struct Histogram {
        std::string name;
        std::vector<double> values;
        std::vector<double> buckets;
        
        void Observe(double value) { values.push_back(value); }
        void Reset() { values.clear(); }
        
        double GetPercentile(double p) const {
            if (values.empty()) return 0.0;
            auto sorted = values;
            std::sort(sorted.begin(), sorted.end());
            size_t index = static_cast<size_t>(p * sorted.size());
            return sorted[std::min(index, sorted.size() - 1)];
        }
    };
    
    std::vector<Counter> counters;
    std::vector<Gauge> gauges;
    std::vector<Histogram> histograms;
};

} // namespace common
} // namespace neo