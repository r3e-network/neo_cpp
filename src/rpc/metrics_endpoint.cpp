/**
 * @file metrics_endpoint.cpp
 * @brief Implementation of metrics endpoint for RPC server
 */

#include <neo/rpc/metrics_endpoint.h>
#include <neo/monitoring/performance_monitor.h>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace neo {
namespace rpc {

std::string MetricsEndpoint::GetPrometheusMetrics() {
    auto& monitor = monitoring::PerformanceMonitor::GetInstance();
    return monitor.ExportPrometheusMetrics();
}

std::string MetricsEndpoint::GetJsonMetrics() {
    auto& monitor = monitoring::PerformanceMonitor::GetInstance();
    return monitor.ExportJsonMetrics();
}

std::string MetricsEndpoint::GetHealthStatus() {
    auto& monitor = monitoring::PerformanceMonitor::GetInstance();
    auto system_metrics = monitor.GetSystemMetrics();
    
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"status\": \"healthy\",\n";
    ss << "  \"timestamp\": " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << ",\n";
    
    // Check various health indicators
    bool is_healthy = true;
    std::vector<std::string> issues;
    
    // Check memory usage
    double memory_usage = system_metrics.GetMemoryUsagePercent();
    if (memory_usage > 90.0) {
        is_healthy = false;
        issues.push_back("High memory usage: " + std::to_string(static_cast<int>(memory_usage)) + "%");
    }
    
    // Check CPU usage
    double cpu_usage = system_metrics.cpu_usage_percent;
    if (cpu_usage > 90.0) {
        is_healthy = false;
        issues.push_back("High CPU usage: " + std::to_string(static_cast<int>(cpu_usage)) + "%");
    }
    
    // Check blockchain sync status
    uint32_t blockchain_height = system_metrics.blockchain_height;
    uint64_t blocks_per_second = system_metrics.blocks_per_second;
    
    ss << "  \"blockchain\": {\n";
    ss << "    \"height\": " << blockchain_height << ",\n";
    ss << "    \"blocks_per_second\": " << blocks_per_second << ",\n";
    ss << "    \"syncing\": " << (blocks_per_second > 0 ? "true" : "false") << "\n";
    ss << "  },\n";
    
    ss << "  \"system\": {\n";
    ss << "    \"cpu_usage_percent\": " << std::fixed << std::setprecision(2) << cpu_usage << ",\n";
    ss << "    \"memory_usage_percent\": " << std::fixed << std::setprecision(2) << memory_usage << ",\n";
    ss << "    \"active_connections\": " << system_metrics.active_connections << ",\n";
    ss << "    \"thread_count\": " << system_metrics.thread_count << "\n";
    ss << "  },\n";
    
    // Add operation statistics
    auto operation_metrics = monitor.GetAllOperationMetrics();
    ss << "  \"operations\": {\n";
    
    // Calculate aggregate statistics
    uint64_t total_operations = 0;
    uint64_t total_errors = 0;
    double total_avg_duration = 0.0;
    int operation_count = 0;
    
    for (const auto& [name, metrics] : operation_metrics) {
        total_operations += metrics.count.load();
        total_errors += metrics.errors.load();
        total_avg_duration += metrics.GetAverageDurationMs();
        operation_count++;
    }
    
    double overall_error_rate = total_operations > 0 ? 
        (static_cast<double>(total_errors) / total_operations) * 100.0 : 0.0;
    double overall_avg_duration = operation_count > 0 ? 
        total_avg_duration / operation_count : 0.0;
    
    ss << "    \"total_count\": " << total_operations << ",\n";
    ss << "    \"error_count\": " << total_errors << ",\n";
    ss << "    \"error_rate_percent\": " << std::fixed << std::setprecision(2) << overall_error_rate << ",\n";
    ss << "    \"average_duration_ms\": " << std::fixed << std::setprecision(2) << overall_avg_duration << "\n";
    ss << "  },\n";
    
    // Add issues if any
    ss << "  \"issues\": [";
    for (size_t i = 0; i < issues.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << issues[i] << "\"";
    }
    ss << "],\n";
    
    ss << "  \"healthy\": " << (is_healthy ? "true" : "false") << "\n";
    ss << "}\n";
    
    return ss.str();
}

} // namespace rpc
} // namespace neo