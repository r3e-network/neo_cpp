/**
 * @file test_performance_monitor.cpp
 * @brief Unit tests for PerformanceMonitor
 */

#include <gtest/gtest.h>
#include <neo/monitoring/performance_monitor.h>
#include <thread>
#include <chrono>
#include <regex>
#include <nlohmann/json.hpp>

using namespace neo::monitoring;
using namespace std::chrono_literals;

class PerformanceMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor_ = &PerformanceMonitor::GetInstance();
        // Note: No Clear() method available, singleton maintains state
        monitor_->Start();
    }
    
    void TearDown() override {
        monitor_->Stop();
        // Note: No Clear() method available
    }
    
    PerformanceMonitor* monitor_;
};

TEST_F(PerformanceMonitorTest, SingletonInstance) {
    auto& instance1 = PerformanceMonitor::GetInstance();
    auto& instance2 = PerformanceMonitor::GetInstance();
    
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(PerformanceMonitorTest, BasicOperationTiming) {
    const std::string op_name = "test_operation";
    
    // Record an operation
    {
        auto timer = monitor_->CreateTimer(op_name);
        std::this_thread::sleep_for(50ms);
    }
    
    // Check metrics
    auto metrics = monitor_->GetOperationMetrics(op_name);
    
    EXPECT_EQ(metrics.count.load(), 1);
    EXPECT_EQ(metrics.errors.load(), 0);
    EXPECT_GE(metrics.total_duration_ms.load(), 50);
    EXPECT_LT(metrics.total_duration_ms.load(), 100);
}

TEST_F(PerformanceMonitorTest, MultipleOperationTiming) {
    const std::string op_name = "multi_op";
    const int num_operations = 10;
    
    for (int i = 0; i < num_operations; ++i) {
        auto timer = monitor_->CreateTimer(op_name);
        std::this_thread::sleep_for(10ms);
    }
    
    auto metrics = monitor_->GetOperationMetrics(op_name);
    
    EXPECT_EQ(metrics.count.load(), num_operations);
    EXPECT_GE(metrics.GetAverageDurationMs(), 10.0);
    EXPECT_LT(metrics.GetAverageDurationMs(), 20.0);
}

TEST_F(PerformanceMonitorTest, ErrorTracking) {
    const std::string op_name = "error_op";
    
    // Record successful operations
    for (int i = 0; i < 8; ++i) {
        monitor_->RecordOperation(op_name, 10, true);
    }
    
    // Record failed operations
    for (int i = 0; i < 2; ++i) {
        monitor_->RecordOperation(op_name, 5, false);
    }
    
    auto metrics = monitor_->GetOperationMetrics(op_name);
    
    EXPECT_EQ(metrics.count.load(), 8);
    EXPECT_EQ(metrics.errors.load(), 2);
    EXPECT_NEAR(metrics.GetErrorRate(), 0.2, 0.01);  // 20% error rate
}

TEST_F(PerformanceMonitorTest, MinMaxTracking) {
    const std::string op_name = "minmax_op";
    
    monitor_->RecordOperation(op_name, 10, true);
    monitor_->RecordOperation(op_name, 50, true);
    monitor_->RecordOperation(op_name, 30, true);
    monitor_->RecordOperation(op_name, 5, true);
    monitor_->RecordOperation(op_name, 100, true);
    
    auto metrics = monitor_->GetOperationMetrics(op_name);
    
    EXPECT_EQ(metrics.min_duration_ms.load(), 5);
    EXPECT_EQ(metrics.max_duration_ms.load(), 100);
}

TEST_F(PerformanceMonitorTest, CustomMetrics) {
    monitor_->RecordMetric("cache_hits", 150);
    monitor_->RecordMetric("cache_misses", 50);
    monitor_->RecordMetric("active_connections", 10);
    
    auto custom_metrics = monitor_->GetCustomMetrics();
    
    EXPECT_EQ(custom_metrics["cache_hits"], 150);
    EXPECT_EQ(custom_metrics["cache_misses"], 50);
    EXPECT_EQ(custom_metrics["active_connections"], 10);
}

TEST_F(PerformanceMonitorTest, SystemMetrics) {
    SystemMetrics metrics;
    metrics.cpu_usage_percent = 45.5;
    metrics.memory_used_bytes = 1024 * 1024 * 512;  // 512 MB
    metrics.memory_available_bytes = 1024 * 1024 * 1024;  // 1 GB
    metrics.thread_count = 8;
    metrics.blockchain_height = 1000000;
    metrics.active_connections = 15;
    
    monitor_->UpdateSystemMetrics(metrics);
    
    auto retrieved = monitor_->GetSystemMetrics();
    EXPECT_DOUBLE_EQ(retrieved.cpu_usage_percent, 45.5);
    EXPECT_EQ(retrieved.memory_used_bytes, 1024 * 1024 * 512);
    EXPECT_EQ(retrieved.thread_count, 8);
    EXPECT_EQ(retrieved.blockchain_height, 1000000);
    EXPECT_EQ(retrieved.active_connections, 15);
    
    // Check memory percentage calculation
    double mem_percent = retrieved.GetMemoryUsagePercent();
    EXPECT_NEAR(mem_percent, 33.33, 0.1);  // 512/(512+1024) * 100
}

TEST_F(PerformanceMonitorTest, PrometheusExport) {
    // Record some metrics
    monitor_->RecordOperation("api_call", 25, true);
    monitor_->RecordOperation("api_call", 30, true);
    monitor_->RecordOperation("api_call", 35, false);
    monitor_->RecordMetric("queue_size", 42);
    
    std::string prometheus = monitor_->ExportPrometheusMetrics();
    
    // Check for expected Prometheus format
    EXPECT_TRUE(prometheus.find("# HELP neo_operation_duration_ms") != std::string::npos);
    EXPECT_TRUE(prometheus.find("# TYPE neo_operation_duration_ms histogram") != std::string::npos);
    EXPECT_TRUE(prometheus.find("neo_operation_count{operation=\"api_call\"} 2") != std::string::npos);
    EXPECT_TRUE(prometheus.find("neo_operation_errors{operation=\"api_call\"} 1") != std::string::npos);
    EXPECT_TRUE(prometheus.find("neo_custom_metric{name=\"queue_size\"} 42") != std::string::npos);
}

TEST_F(PerformanceMonitorTest, JsonExport) {
    // Record metrics
    monitor_->RecordOperation("block_validation", 100, true);
    monitor_->RecordMetric("pending_transactions", 250);
    
    SystemMetrics sys;
    sys.cpu_usage_percent = 60.0;
    sys.memory_used_bytes = 2147483648;  // 2GB
    monitor_->UpdateSystemMetrics(sys);
    
    std::string json_str = monitor_->ExportJsonMetrics();
    
    // Parse JSON
    nlohmann::json root = nlohmann::json::parse(json_str);
    
    // Verify structure
    EXPECT_TRUE(root.contains("timestamp"));
    EXPECT_TRUE(root.contains("system"));
    EXPECT_TRUE(root.contains("operations"));
    EXPECT_TRUE(root.contains("custom"));
    
    // Check specific values
    EXPECT_EQ(root["operations"]["block_validation"]["count"], 1);
    EXPECT_EQ(root["custom"]["pending_transactions"], 250);
    EXPECT_DOUBLE_EQ(root["system"]["cpu_usage_percent"], 60.0);
}

TEST_F(PerformanceMonitorTest, AlertThresholds) {
    bool alert_triggered = false;
    std::string alert_message;
    
    // Register alert callback
    monitor_->RegisterAlertCallback([&alert_triggered, &alert_message](
        const std::string& type, const std::string& msg) {
        alert_triggered = true;
        alert_message = msg;
    });
    
    // Set threshold
    monitor_->SetAlertThreshold("slow_operation", 50, 0.1);  // 50ms max, 10% error rate
    
    // Record operations that should trigger alert
    monitor_->RecordOperation("slow_operation", 100, true);  // Exceeds duration threshold
    
    // Alert should be triggered
    std::this_thread::sleep_for(100ms);  // Give time for alert processing
    EXPECT_TRUE(alert_triggered);
    EXPECT_TRUE(alert_message.find("slow_operation") != std::string::npos);
}

TEST_F(PerformanceMonitorTest, ConcurrentOperations) {
    const int num_threads = 10;
    const int ops_per_thread = 100;
    std::atomic<int> total_ops{0};
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, ops_per_thread, &total_ops]() {
            std::string op_name = "concurrent_op_" + std::to_string(t % 3);
            
            for (int i = 0; i < ops_per_thread; ++i) {
                auto timer = monitor_->CreateTimer(op_name);
                std::this_thread::sleep_for(1ms);
                total_ops++;
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_ops.load(), num_threads * ops_per_thread);
    
    // Verify metrics
    for (int i = 0; i < 3; ++i) {
        std::string op_name = "concurrent_op_" + std::to_string(i);
        auto metrics = monitor_->GetOperationMetrics(op_name);
        EXPECT_GT(metrics.count.load(), 0);
    }
}

TEST_F(PerformanceMonitorTest, TracingMode) {
    monitor_->SetTracingEnabled(true);
    
    // Perform operations with tracing
    {
        auto timer = monitor_->CreateTimer("traced_operation");
        std::this_thread::sleep_for(20ms);
    }
    
    // Tracing should provide detailed output (implementation specific)
    auto metrics = monitor_->GetOperationMetrics("traced_operation");
    EXPECT_EQ(metrics.count.load(), 1);
    
    monitor_->SetTracingEnabled(false);
}

TEST_F(PerformanceMonitorTest, ClearMetrics) {
    // Add various metrics
    monitor_->RecordOperation("op1", 10, true);
    monitor_->RecordOperation("op2", 20, true);
    monitor_->RecordMetric("metric1", 100);
    
    // Verify metrics exist
    auto metrics1 = monitor_->GetOperationMetrics("op1");
    auto metrics2 = monitor_->GetOperationMetrics("op2");
    EXPECT_GT(metrics1.count.load(), 0);
    EXPECT_GT(metrics2.count.load(), 0);
    EXPECT_GT(monitor_->GetCustomMetrics().size(), 0);
    
    // Note: PerformanceMonitor doesn't have a Clear() method
    // We'll test that metrics accumulate instead
    monitor_->RecordOperation("op1", 15, true);
    
    // Verify metrics accumulated
    auto updated = monitor_->GetOperationMetrics("op1");
    EXPECT_EQ(updated.count.load(), 2);
}

TEST_F(PerformanceMonitorTest, MacroConvenience) {
    // Test MONITOR_OPERATION macro
    {
        MONITOR_OPERATION("macro_test");
        std::this_thread::sleep_for(15ms);
    }
    
    auto metrics = monitor_->GetOperationMetrics("macro_test");
    EXPECT_EQ(metrics.count.load(), 1);
    EXPECT_GE(metrics.total_duration_ms.load(), 15);
    
    // Test RECORD_METRIC macro
    RECORD_METRIC("macro_metric", 999);
    
    auto custom = monitor_->GetCustomMetrics();
    EXPECT_EQ(custom["macro_metric"], 999);
}

TEST_F(PerformanceMonitorTest, GetAllOperationMetrics) {
    // Record various operations
    monitor_->RecordOperation("op_a", 10.0, true);
    monitor_->RecordOperation("op_b", 20.0, true);
    monitor_->RecordOperation("op_c", 30.0, false);
    
    auto all_metrics = monitor_->GetAllOperationMetrics();
    
    EXPECT_EQ(all_metrics.size(), 3);
    EXPECT_TRUE(all_metrics.find("op_a") != all_metrics.end());
    EXPECT_TRUE(all_metrics.find("op_b") != all_metrics.end());
    EXPECT_TRUE(all_metrics.find("op_c") != all_metrics.end());
    
    EXPECT_EQ(all_metrics["op_a"].count.load(), 1);
    EXPECT_EQ(all_metrics["op_b"].count.load(), 1);
    EXPECT_EQ(all_metrics["op_c"].errors.load(), 1);
}

TEST_F(PerformanceMonitorTest, PerformanceBenchmark) {
    const int num_operations = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        monitor_->RecordOperation("benchmark_op", 1.0, true);
    }
    
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    // Should handle at least 10K ops/second
    double ops_per_second = (num_operations * 1000.0) / ms;
    EXPECT_GT(ops_per_second, 10000);
    
    std::cout << "PerformanceMonitor Benchmark: " << ops_per_second << " ops/sec\n";
    
    // Verify metrics
    auto metrics = monitor_->GetOperationMetrics("benchmark_op");
    EXPECT_EQ(metrics.count.load(), num_operations);
}