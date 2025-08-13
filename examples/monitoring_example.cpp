/**
 * @file monitoring_example.cpp
 * @brief Example demonstrating performance monitoring capabilities
 */

#include <neo/monitoring/performance_monitor.h>
#include <neo/network/connection_pool.h>
#include <neo/ledger/blockchain_cache.h>
#include <neo/logging/console_logger.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>

using namespace neo;
using namespace neo::monitoring;
using namespace neo::network;
using namespace neo::ledger;

// Simulate blockchain operations
void SimulateBlockProcessing() {
    auto& monitor = PerformanceMonitor::GetInstance();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> duration_dist(10, 100);
    std::uniform_real_distribution<> success_dist(0.0, 1.0);
    
    for (int i = 0; i < 10; ++i) {
        // Simulate block validation
        {
            auto timer = monitor.CreateTimer("block_validation");
            std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(gen)));
            
            // Simulate occasional failures
            if (success_dist(gen) > 0.95) {
                monitor.RecordOperation("block_validation", 0, false);
            }
        }
        
        // Simulate transaction processing
        {
            MONITOR_OPERATION("transaction_processing");
            std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(gen) / 2));
        }
        
        // Record custom metrics
        RECORD_METRIC("pending_transactions", 100 + i * 10);
        RECORD_METRIC("memory_pool_size", 50 + i * 5);
    }
}

// Simulate network operations
void SimulateNetworkOperations() {
    auto& monitor = PerformanceMonitor::GetInstance();
    ConnectionPool pool;
    pool.Start();
    
    for (int i = 0; i < 5; ++i) {
        auto timer = monitor.CreateTimer("network_request");
        
        // Simulate connection establishment
        std::this_thread::sleep_for(std::chrono::milliseconds(20 + i * 5));
        
        // Record network metrics
        RECORD_METRIC("active_connections", 5 + i);
        RECORD_METRIC("bytes_sent", 1024 * (i + 1));
        RECORD_METRIC("bytes_received", 2048 * (i + 1));
    }
    
    pool.Stop();
}

// Simulate cache operations
void SimulateCacheOperations() {
    auto& monitor = PerformanceMonitor::GetInstance();
    BlockchainCache cache;
    
    for (int i = 0; i < 20; ++i) {
        // Simulate cache lookup
        {
            auto timer = monitor.CreateTimer("cache_lookup");
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        
        // Simulate cache hit/miss
        bool is_hit = (i % 3) != 0;  // 66% hit rate
        if (is_hit) {
            RECORD_METRIC("cache_hits", 1);
        } else {
            RECORD_METRIC("cache_misses", 1);
            
            // Simulate cache load
            auto timer = monitor.CreateTimer("cache_load");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    // Get and display cache stats
    auto stats = cache.GetStats();
    std::cout << "Cache Statistics:\n";
    std::cout << "  Hit Rate: " << (stats.hit_rate * 100) << "%\n";
    std::cout << "  Total Blocks: " << stats.block_stats.size << "\n";
    std::cout << "  Total Transactions: " << stats.tx_stats.size << "\n";
}

// Display monitoring results
void DisplayMetrics() {
    auto& monitor = PerformanceMonitor::GetInstance();
    
    // Get system metrics
    auto system_metrics = monitor.GetSystemMetrics();
    std::cout << "\n=== System Metrics ===\n";
    std::cout << "CPU Usage: " << system_metrics.cpu_usage_percent << "%\n";
    std::cout << "Memory Usage: " << system_metrics.GetMemoryUsagePercent() << "%\n";
    std::cout << "Thread Count: " << system_metrics.thread_count << "\n";
    std::cout << "Active Connections: " << system_metrics.active_connections << "\n";
    
    // Get operation metrics
    auto operation_metrics = monitor.GetAllOperationMetrics();
    std::cout << "\n=== Operation Metrics ===\n";
    for (const auto& [name, metrics] : operation_metrics) {
        std::cout << name << ":\n";
        std::cout << "  Count: " << metrics.count.load() << "\n";
        std::cout << "  Errors: " << metrics.errors.load() << "\n";
        std::cout << "  Avg Duration: " << metrics.GetAverageDurationMs() << "ms\n";
        std::cout << "  Min Duration: " << metrics.min_duration_ms.load() << "ms\n";
        std::cout << "  Max Duration: " << metrics.max_duration_ms.load() << "ms\n";
        std::cout << "  Error Rate: " << (metrics.GetErrorRate() * 100) << "%\n";
    }
    
    // Get custom metrics
    auto custom_metrics = monitor.GetCustomMetrics();
    std::cout << "\n=== Custom Metrics ===\n";
    for (const auto& [name, value] : custom_metrics) {
        std::cout << name << ": " << value << "\n";
    }
}

// Export metrics in different formats
void ExportMetrics() {
    auto& monitor = PerformanceMonitor::GetInstance();
    
    // Export as Prometheus format
    std::cout << "\n=== Prometheus Format ===\n";
    std::cout << monitor.ExportPrometheusMetrics().substr(0, 500) << "...\n";
    
    // Export as JSON
    std::cout << "\n=== JSON Format ===\n";
    std::cout << monitor.ExportJsonMetrics().substr(0, 500) << "...\n";
}

// Set up alerting
void SetupAlerts() {
    auto& monitor = PerformanceMonitor::GetInstance();
    
    // Set alert thresholds
    monitor.SetAlertThreshold("block_validation", 80, 0.1);  // 80ms max, 10% error rate
    monitor.SetAlertThreshold("network_request", 100, 0.05); // 100ms max, 5% error rate
    
    // Register alert callback
    monitor.RegisterAlertCallback([](const std::string& type, const std::string& message) {
        logging::ConsoleLogger::Warning("ALERT [" + type + "]: " + message);
    });
}

int main() {
    std::cout << "Neo C++ Performance Monitoring Example\n";
    std::cout << "=====================================\n\n";
    
    // Initialize monitoring
    auto& monitor = PerformanceMonitor::GetInstance();
    monitor.Start();
    monitor.SetTracingEnabled(true);
    
    // Setup alerts
    SetupAlerts();
    
    // Update system metrics
    SystemMetrics sys_metrics;
    sys_metrics.cpu_usage_percent = 45.5;
    sys_metrics.memory_used_bytes = 1024 * 1024 * 512;  // 512 MB
    sys_metrics.memory_available_bytes = 1024 * 1024 * 1536;  // 1.5 GB
    sys_metrics.thread_count = std::thread::hardware_concurrency();
    sys_metrics.blockchain_height = 1000000;
    sys_metrics.active_connections = 8;
    monitor.UpdateSystemMetrics(sys_metrics);
    
    std::cout << "Running simulations...\n";
    
    // Run simulations
    std::thread block_thread(SimulateBlockProcessing);
    std::thread network_thread(SimulateNetworkOperations);
    std::thread cache_thread(SimulateCacheOperations);
    
    // Wait for simulations to complete
    block_thread.join();
    network_thread.join();
    cache_thread.join();
    
    std::cout << "\nSimulations complete.\n";
    
    // Display results
    DisplayMetrics();
    ExportMetrics();
    
    // Cleanup
    monitor.Stop();
    
    std::cout << "\n=== Monitoring Example Complete ===\n";
    
    return 0;
}