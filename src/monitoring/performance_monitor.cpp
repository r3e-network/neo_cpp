/**
 * @file performance_monitor.cpp
 * @brief Implementation of performance monitoring system
 */

#include <neo/monitoring/performance_monitor.h>
#include <neo/logging/console_logger.h>
#include <sstream>
#include <iomanip>
#include <cstring>

#ifdef __linux__
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>
#elif __APPLE__
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <mach/mach_init.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#endif

namespace neo {
namespace monitoring {

// ScopedTimer implementation
ScopedTimer::ScopedTimer(const std::string& operation_name,
                        std::function<void(const std::string&, uint64_t)> callback)
    : operation_name_(operation_name)
    , callback_(callback)
    , start_(std::chrono::steady_clock::now()) {
}

ScopedTimer::~ScopedTimer() {
    if (!stopped_) {
        Stop();
    }
}

void ScopedTimer::Stop() {
    if (!stopped_) {
        stopped_ = true;
        uint64_t elapsed = GetElapsedMs();
        if (callback_) {
            callback_(operation_name_, elapsed);
        }
    }
}

uint64_t ScopedTimer::GetElapsedMs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
}

// PerformanceMonitor implementation
PerformanceMonitor::PerformanceMonitor() 
    : start_time_(std::chrono::steady_clock::now()) {
}

PerformanceMonitor::~PerformanceMonitor() {
    Stop();
}

void PerformanceMonitor::Start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    start_time_ = std::chrono::steady_clock::now();
    monitoring_thread_ = std::thread(&PerformanceMonitor::MonitoringThread, this);
    
    logging::ConsoleLogger::Info("Performance monitoring started");
}

void PerformanceMonitor::Stop() {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }
    
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
    
    logging::ConsoleLogger::Info("Performance monitoring stopped");
}

void PerformanceMonitor::RecordOperation(const std::string& operation_name,
                                        uint64_t duration_ms,
                                        bool success) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto& metrics = operation_metrics_[operation_name];
    
    if (success) {
        metrics.count.fetch_add(1);
        metrics.total_duration_ms.fetch_add(duration_ms);
        
        // Update min
        uint64_t current_min = metrics.min_duration_ms.load();
        while (duration_ms < current_min && 
               !metrics.min_duration_ms.compare_exchange_weak(current_min, duration_ms)) {
            // Retry
        }
        
        // Update max
        uint64_t current_max = metrics.max_duration_ms.load();
        while (duration_ms > current_max && 
               !metrics.max_duration_ms.compare_exchange_weak(current_max, duration_ms)) {
            // Retry
        }
        
        metrics.last_duration_ms.store(duration_ms);
        metrics.last_execution = std::chrono::steady_clock::now();
    } else {
        metrics.errors.fetch_add(1);
    }
    
    if (tracing_enabled_) {
        logging::ConsoleLogger::Debug("Operation " + operation_name + 
                                    " completed in " + std::to_string(duration_ms) + 
                                    "ms, success=" + (success ? "true" : "false"));
    }
}

std::unique_ptr<ScopedTimer> PerformanceMonitor::CreateTimer(const std::string& operation_name) {
    return std::make_unique<ScopedTimer>(operation_name,
        [this](const std::string& name, uint64_t duration) {
            RecordOperation(name, duration, true);
        });
}

void PerformanceMonitor::RecordMetric(const std::string& metric_name, double value) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    custom_metrics_[metric_name] = value;
}

OperationMetrics PerformanceMonitor::GetOperationMetrics(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = operation_metrics_.find(operation_name);
    if (it != operation_metrics_.end()) {
        OperationMetrics result;
        result.count = it->second.count.load();
        result.total_duration_ms = it->second.total_duration_ms.load();
        result.min_duration_ms = it->second.min_duration_ms.load();
        result.max_duration_ms = it->second.max_duration_ms.load();
        result.errors = it->second.errors.load();
        result.last_duration_ms = it->second.last_duration_ms.load();
        result.last_execution = it->second.last_execution;
        return result;
    }
    return OperationMetrics();
}

std::unordered_map<std::string, OperationMetrics> PerformanceMonitor::GetAllOperationMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    std::unordered_map<std::string, OperationMetrics> result;
    for (const auto& [name, metrics] : operation_metrics_) {
        OperationMetrics copy;
        copy.count = metrics.count.load();
        copy.total_duration_ms = metrics.total_duration_ms.load();
        copy.min_duration_ms = metrics.min_duration_ms.load();
        copy.max_duration_ms = metrics.max_duration_ms.load();
        copy.errors = metrics.errors.load();
        copy.last_duration_ms = metrics.last_duration_ms.load();
        copy.last_execution = metrics.last_execution;
        result[name] = copy;
    }
    return result;
}

SystemMetrics PerformanceMonitor::GetSystemMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return system_metrics_;
}

void PerformanceMonitor::UpdateSystemMetrics(const SystemMetrics& metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    system_metrics_ = metrics;
}

std::unordered_map<std::string, double> PerformanceMonitor::GetCustomMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return custom_metrics_;
}

std::string PerformanceMonitor::ExportPrometheusMetrics() const {
    std::stringstream ss;
    
    // System metrics
    SystemMetrics sys_metrics = GetSystemMetrics();
    
    ss << "# HELP neo_cpu_usage_percent CPU usage percentage\n";
    ss << "# TYPE neo_cpu_usage_percent gauge\n";
    ss << "neo_cpu_usage_percent " << sys_metrics.cpu_usage_percent << "\n\n";
    
    ss << "# HELP neo_memory_used_bytes Memory used in bytes\n";
    ss << "# TYPE neo_memory_used_bytes gauge\n";
    ss << "neo_memory_used_bytes " << sys_metrics.memory_used_bytes << "\n\n";
    
    ss << "# HELP neo_blockchain_height Current blockchain height\n";
    ss << "# TYPE neo_blockchain_height counter\n";
    ss << "neo_blockchain_height " << sys_metrics.blockchain_height << "\n\n";
    
    ss << "# HELP neo_active_connections Number of active connections\n";
    ss << "# TYPE neo_active_connections gauge\n";
    ss << "neo_active_connections " << sys_metrics.active_connections << "\n\n";
    
    // Operation metrics
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        for (const auto& [name, metrics] : operation_metrics_) {
            std::string safe_name = name;
            std::replace(safe_name.begin(), safe_name.end(), '.', '_');
            std::replace(safe_name.begin(), safe_name.end(), '-', '_');
            
            ss << "# HELP neo_operation_count_total Total operation count\n";
            ss << "# TYPE neo_operation_count_total counter\n";
            ss << "neo_operation_count_total{operation=\"" << safe_name << "\"} " 
               << metrics.count.load() << "\n\n";
            
            ss << "# HELP neo_operation_duration_ms Operation duration in milliseconds\n";
            ss << "# TYPE neo_operation_duration_ms histogram\n";
            ss << "neo_operation_duration_ms{operation=\"" << safe_name << "\",quantile=\"0.0\"} " 
               << metrics.min_duration_ms.load() << "\n";
            ss << "neo_operation_duration_ms{operation=\"" << safe_name << "\",quantile=\"0.5\"} " 
               << metrics.GetAverageDurationMs() << "\n";
            ss << "neo_operation_duration_ms{operation=\"" << safe_name << "\",quantile=\"1.0\"} " 
               << metrics.max_duration_ms.load() << "\n\n";
            
            ss << "# HELP neo_operation_errors_total Total operation errors\n";
            ss << "# TYPE neo_operation_errors_total counter\n";
            ss << "neo_operation_errors_total{operation=\"" << safe_name << "\"} " 
               << metrics.errors.load() << "\n\n";
        }
        
        // Custom metrics
        for (const auto& [name, value] : custom_metrics_) {
            std::string safe_name = name;
            std::replace(safe_name.begin(), safe_name.end(), '.', '_');
            std::replace(safe_name.begin(), safe_name.end(), '-', '_');
            
            ss << "# HELP neo_custom_" << safe_name << " Custom metric\n";
            ss << "# TYPE neo_custom_" << safe_name << " gauge\n";
            ss << "neo_custom_" << safe_name << " " << value << "\n\n";
        }
    }
    
    return ss.str();
}

std::string PerformanceMonitor::ExportJsonMetrics() const {
    std::stringstream ss;
    ss << "{\n";
    
    // System metrics
    SystemMetrics sys_metrics = GetSystemMetrics();
    
    ss << "  \"system\": {\n";
    ss << "    \"cpu_usage_percent\": " << sys_metrics.cpu_usage_percent << ",\n";
    ss << "    \"thread_count\": " << sys_metrics.thread_count << ",\n";
    ss << "    \"memory_used_bytes\": " << sys_metrics.memory_used_bytes << ",\n";
    ss << "    \"memory_available_bytes\": " << sys_metrics.memory_available_bytes << ",\n";
    ss << "    \"memory_usage_percent\": " << sys_metrics.GetMemoryUsagePercent() << ",\n";
    ss << "    \"blockchain_height\": " << sys_metrics.blockchain_height << ",\n";
    ss << "    \"active_connections\": " << sys_metrics.active_connections << ",\n";
    ss << "    \"total_connections\": " << sys_metrics.total_connections << ",\n";
    ss << "    \"blocks_per_second\": " << sys_metrics.blocks_per_second << ",\n";
    ss << "    \"transactions_per_second\": " << sys_metrics.transactions_per_second << "\n";
    ss << "  },\n";
    
    // Operation metrics
    ss << "  \"operations\": {\n";
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        bool first = true;
        for (const auto& [name, metrics] : operation_metrics_) {
            if (!first) ss << ",\n";
            first = false;
            
            ss << "    \"" << name << "\": {\n";
            ss << "      \"count\": " << metrics.count.load() << ",\n";
            ss << "      \"errors\": " << metrics.errors.load() << ",\n";
            ss << "      \"error_rate\": " << std::fixed << std::setprecision(4) 
               << metrics.GetErrorRate() << ",\n";
            ss << "      \"avg_duration_ms\": " << std::fixed << std::setprecision(2) 
               << metrics.GetAverageDurationMs() << ",\n";
            ss << "      \"min_duration_ms\": " << metrics.min_duration_ms.load() << ",\n";
            ss << "      \"max_duration_ms\": " << metrics.max_duration_ms.load() << ",\n";
            ss << "      \"last_duration_ms\": " << metrics.last_duration_ms.load() << "\n";
            ss << "    }";
        }
    }
    ss << "\n  },\n";
    
    // Custom metrics
    ss << "  \"custom\": {\n";
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        bool first = true;
        for (const auto& [name, value] : custom_metrics_) {
            if (!first) ss << ",\n";
            first = false;
            ss << "    \"" << name << "\": " << value;
        }
    }
    ss << "\n  },\n";
    
    // Uptime
    auto now = std::chrono::steady_clock::now();
    auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    ss << "  \"uptime_seconds\": " << uptime_seconds << "\n";
    
    ss << "}\n";
    return ss.str();
}

void PerformanceMonitor::SetAlertThreshold(const std::string& operation_name,
                                          uint64_t max_duration_ms,
                                          double max_error_rate) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    alert_thresholds_[operation_name] = {max_duration_ms, max_error_rate};
}

void PerformanceMonitor::RegisterAlertCallback(
    std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    alert_callbacks_.push_back(callback);
}

void PerformanceMonitor::ClearMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    operation_metrics_.clear();
    custom_metrics_.clear();
}

void PerformanceMonitor::SetTracingEnabled(bool enabled) {
    tracing_enabled_.store(enabled);
}

void PerformanceMonitor::MonitoringThread() {
    while (running_.load()) {
        CollectSystemMetrics();
        CheckAlerts();
        
        // Sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void PerformanceMonitor::CollectSystemMetrics() {
#ifdef __linux__
    // Linux implementation
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        system_metrics_.memory_used_bytes = info.totalram - info.freeram;
        system_metrics_.memory_available_bytes = info.freeram;
    }
    
    // Get CPU usage using /proc/stat
    static uint64_t prev_idle = 0, prev_total = 0;
    std::ifstream stat_file("/proc/stat");
    if (stat_file.is_open()) {
        std::string cpu;
        uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
        stat_file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        
        uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
        uint64_t diff_idle = idle - prev_idle;
        uint64_t diff_total = total - prev_total;
        
        if (diff_total > 0) {
            double cpu_usage = 100.0 * (1.0 - static_cast<double>(diff_idle) / diff_total);
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            system_metrics_.cpu_usage_percent = cpu_usage;
        }
        
        prev_idle = idle;
        prev_total = total;
    }
#elif __APPLE__
    // macOS implementation
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_stat) / sizeof(natural_t);
    
    if (host_page_size(mach_host_self(), &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_host_self(), HOST_VM_INFO64, 
                         (host_info64_t)&vm_stat, &host_size) == KERN_SUCCESS) {
        uint64_t total_pages = vm_stat.free_count + vm_stat.active_count + 
                              vm_stat.inactive_count + vm_stat.wire_count;
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        system_metrics_.memory_used_bytes = (total_pages - vm_stat.free_count) * page_size;
        system_metrics_.memory_available_bytes = vm_stat.free_count * page_size;
    }
#endif
    
    // Thread count (portable)
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        system_metrics_.thread_count = std::thread::hardware_concurrency();
    }
}

void PerformanceMonitor::CheckAlerts() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    for (const auto& [operation_name, threshold] : alert_thresholds_) {
        auto it = operation_metrics_.find(operation_name);
        if (it != operation_metrics_.end()) {
            const auto& metrics = it->second;
            
            // Check duration threshold
            if (metrics.last_duration_ms.load() > threshold.max_duration_ms) {
                std::stringstream msg;
                msg << "Operation '" << operation_name << "' exceeded duration threshold: "
                    << metrics.last_duration_ms.load() << "ms > " 
                    << threshold.max_duration_ms << "ms";
                TriggerAlert("SLOW_OPERATION", msg.str());
            }
            
            // Check error rate threshold
            double error_rate = metrics.GetErrorRate();
            if (error_rate > threshold.max_error_rate) {
                std::stringstream msg;
                msg << "Operation '" << operation_name << "' exceeded error rate threshold: "
                    << std::fixed << std::setprecision(2) << (error_rate * 100) << "% > "
                    << (threshold.max_error_rate * 100) << "%";
                TriggerAlert("HIGH_ERROR_RATE", msg.str());
            }
        }
    }
}

void PerformanceMonitor::TriggerAlert(const std::string& alert_type, const std::string& message) {
    logging::ConsoleLogger::Warning("[ALERT] " + alert_type + ": " + message);
    
    for (const auto& callback : alert_callbacks_) {
        if (callback) {
            callback(alert_type, message);
        }
    }
}

} // namespace monitoring
} // namespace neo