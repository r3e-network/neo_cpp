#include <gtest/gtest.h>
#include <neo/monitoring/metrics_collector.h>
#include <neo/monitoring/blockchain_metrics.h>
#include <neo/monitoring/network_metrics.h>
#include <thread>
#include <chrono>

using namespace neo::monitoring;

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing metrics
        MetricsCollector::GetInstance().Clear();
    }
    
    void TearDown() override {
        MetricsCollector::GetInstance().Clear();
    }
};

TEST_F(MetricsTest, CounterBasicOperations) {
    auto& collector = MetricsCollector::GetInstance();
    auto counter = collector.RegisterCounter("test_counter", "Test counter metric");
    
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->GetValue(), 0.0);
    
    counter->Increment();
    EXPECT_EQ(counter->GetValue(), 1.0);
    
    counter->Increment(5.0);
    EXPECT_EQ(counter->GetValue(), 6.0);
    
    counter->Reset();
    EXPECT_EQ(counter->GetValue(), 0.0);
}

TEST_F(MetricsTest, GaugeBasicOperations) {
    auto& collector = MetricsCollector::GetInstance();
    auto gauge = collector.RegisterGauge("test_gauge", "Test gauge metric");
    
    ASSERT_NE(gauge, nullptr);
    EXPECT_EQ(gauge->GetValue(), 0.0);
    
    gauge->Set(42.5);
    EXPECT_EQ(gauge->GetValue(), 42.5);
    
    gauge->Increment(7.5);
    EXPECT_EQ(gauge->GetValue(), 50.0);
    
    gauge->Decrement(10.0);
    EXPECT_EQ(gauge->GetValue(), 40.0);
}

TEST_F(MetricsTest, HistogramBasicOperations) {
    auto& collector = MetricsCollector::GetInstance();
    std::vector<double> buckets = {0.1, 0.5, 1.0, 5.0};
    auto histogram = collector.RegisterHistogram("test_histogram", "Test histogram metric", buckets);
    
    ASSERT_NE(histogram, nullptr);
    
    // Observe some values
    histogram->Observe(0.05);  // Should go in 0.1 bucket
    histogram->Observe(0.3);   // Should go in 0.5 bucket
    histogram->Observe(0.7);   // Should go in 1.0 bucket
    histogram->Observe(2.0);   // Should go in 5.0 bucket
    histogram->Observe(10.0);  // Should go in +Inf bucket
    
    // Check Prometheus output contains expected buckets
    std::string output = histogram->ToPrometheus();
    EXPECT_TRUE(output.find("le=\"0.1\"") != std::string::npos);
    EXPECT_TRUE(output.find("le=\"0.5\"") != std::string::npos);
    EXPECT_TRUE(output.find("le=\"1\"") != std::string::npos);
    EXPECT_TRUE(output.find("le=\"5\"") != std::string::npos);
    EXPECT_TRUE(output.find("le=\"+Inf\"") != std::string::npos);
    EXPECT_TRUE(output.find("_sum") != std::string::npos);
    EXPECT_TRUE(output.find("_count") != std::string::npos);
}

TEST_F(MetricsTest, SummaryBasicOperations) {
    auto& collector = MetricsCollector::GetInstance();
    auto summary = collector.RegisterSummary("test_summary", "Test summary metric");
    
    ASSERT_NE(summary, nullptr);
    
    // Observe some values
    for (int i = 1; i <= 100; ++i) {
        summary->Observe(static_cast<double>(i));
    }
    
    // Check Prometheus output contains quantiles
    std::string output = summary->ToPrometheus();
    EXPECT_TRUE(output.find("quantile=\"0.5\"") != std::string::npos);
    EXPECT_TRUE(output.find("quantile=\"0.9\"") != std::string::npos);
    EXPECT_TRUE(output.find("quantile=\"0.99\"") != std::string::npos);
    EXPECT_TRUE(output.find("_sum") != std::string::npos);
    EXPECT_TRUE(output.find("_count") != std::string::npos);
}

TEST_F(MetricsTest, MetricsCollectorRegistry) {
    auto& collector = MetricsCollector::GetInstance();
    
    // Register multiple metrics
    auto counter1 = collector.RegisterCounter("counter1", "First counter");
    auto counter2 = collector.RegisterCounter("counter2", "Second counter");
    auto gauge1 = collector.RegisterGauge("gauge1", "First gauge");
    
    // Verify we can retrieve them
    EXPECT_EQ(collector.GetCounter("counter1"), counter1);
    EXPECT_EQ(collector.GetCounter("counter2"), counter2);
    EXPECT_EQ(collector.GetGauge("gauge1"), gauge1);
    
    // Non-existent metrics should return nullptr
    EXPECT_EQ(collector.GetCounter("nonexistent"), nullptr);
    EXPECT_EQ(collector.GetGauge("nonexistent"), nullptr);
}

TEST_F(MetricsTest, ScopedTimer) {
    auto& collector = MetricsCollector::GetInstance();
    auto histogram = collector.RegisterHistogram("timer_test", "Timer test metric");
    
    {
        ScopedTimer timer(histogram);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Check that time was recorded
    std::string output = histogram->ToPrometheus();
    EXPECT_TRUE(output.find("timer_test_count 1") != std::string::npos);
}

TEST_F(MetricsTest, BlockchainMetrics) {
    auto& blockchain = BlockchainMetrics::GetInstance();
    blockchain.Initialize();
    
    // Test block metrics
    blockchain.OnBlockReceived();
    blockchain.OnBlockProcessed(0.5);
    blockchain.OnBlockValidated(true);
    blockchain.SetBlockHeight(12345);
    
    // Test transaction metrics
    blockchain.OnTransactionReceived();
    blockchain.OnTransactionProcessed(0.001);
    blockchain.OnTransactionValidated(true);
    blockchain.SetMempoolSize(100);
    
    // Test state metrics
    blockchain.SetAccountCount(1000);
    blockchain.SetContractCount(50);
    blockchain.SetValidatorCount(7);
    
    // Verify metrics were recorded
    auto& collector = MetricsCollector::GetInstance();
    auto blocks_received = collector.GetCounter("neo_blocks_received_total");
    ASSERT_NE(blocks_received, nullptr);
    EXPECT_EQ(blocks_received->GetValue(), 1.0);
    
    auto block_height = collector.GetGauge("neo_block_height");
    ASSERT_NE(block_height, nullptr);
    EXPECT_EQ(block_height->GetValue(), 12345.0);
}

TEST_F(MetricsTest, NetworkMetrics) {
    auto& network = NetworkMetrics::GetInstance();
    network.Initialize();
    
    // Test connection metrics
    network.OnPeerConnected("peer1");
    network.OnPeerConnected("peer2");
    network.OnPeerDisconnected("peer1");
    network.SetMaxPeers(50);
    
    // Test message metrics
    network.OnMessageSent("block", 1024);
    network.OnMessageReceived("transaction", 256);
    network.OnMessageProcessed("block", 0.01);
    
    // Test RPC metrics
    network.OnRpcRequest("getblock");
    network.OnRpcResponse("getblock", 0.05, true);
    network.SetActiveRpcConnections(5);
    
    // Verify metrics were recorded
    auto& collector = MetricsCollector::GetInstance();
    auto peers_connected = collector.GetCounter("neo_peers_connected_total");
    ASSERT_NE(peers_connected, nullptr);
    EXPECT_EQ(peers_connected->GetValue(), 2.0);
    
    auto peers_current = collector.GetGauge("neo_peers_current");
    ASSERT_NE(peers_current, nullptr);
    EXPECT_EQ(peers_current->GetValue(), 1.0);
}

TEST_F(MetricsTest, PrometheusExport) {
    auto& collector = MetricsCollector::GetInstance();
    
    // Create various metrics
    auto counter = collector.RegisterCounter("export_counter", "Export test counter");
    counter->Increment(42);
    
    auto gauge = collector.RegisterGauge("export_gauge", "Export test gauge");
    gauge->Set(3.14);
    
    // Export to Prometheus format
    std::string prometheus_output = collector.ExportPrometheus();
    
    // Verify output contains expected metrics
    EXPECT_TRUE(prometheus_output.find("# HELP export_counter") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("# TYPE export_counter counter") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("export_counter 42") != std::string::npos);
    
    EXPECT_TRUE(prometheus_output.find("# HELP export_gauge") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("# TYPE export_gauge gauge") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("export_gauge 3.14") != std::string::npos);
}

TEST_F(MetricsTest, JSONExport) {
    auto& collector = MetricsCollector::GetInstance();
    
    // Create some metrics
    auto counter = collector.RegisterCounter("json_counter", "JSON test counter");
    counter->Increment(10);
    
    auto gauge = collector.RegisterGauge("json_gauge", "JSON test gauge");
    gauge->Set(20.5);
    
    // Export to JSON format
    std::string json_output = collector.ExportJSON();
    
    // Verify JSON contains expected fields
    EXPECT_TRUE(json_output.find("\"name\": \"json_counter\"") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"value\": 10") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"name\": \"json_gauge\"") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"value\": 20.5") != std::string::npos);
}

TEST_F(MetricsTest, ThreadSafety) {
    auto& collector = MetricsCollector::GetInstance();
    auto counter = collector.RegisterCounter("thread_counter", "Thread safety test");
    auto gauge = collector.RegisterGauge("thread_gauge", "Thread safety test");
    
    const int num_threads = 10;
    const int operations_per_thread = 1000;
    
    std::vector<std::thread> threads;
    
    // Start threads that increment counter
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([counter, gauge, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                counter->Increment();
                gauge->Increment();
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify final values
    EXPECT_EQ(counter->GetValue(), num_threads * operations_per_thread);
    EXPECT_EQ(gauge->GetValue(), num_threads * operations_per_thread);
}

// Macro usage tests
TEST_F(MetricsTest, MacroUsage) {
    auto& collector = MetricsCollector::GetInstance();
    
    // Register metrics that macros will use
    collector.RegisterCounter("macro_counter", "Macro test counter");
    collector.RegisterGauge("macro_gauge", "Macro test gauge");
    collector.RegisterHistogram("macro_histogram", "Macro test histogram");
    
    // Use macros
    METRIC_INCREMENT("macro_counter");
    METRIC_GAUGE_SET("macro_gauge", 42.0);
    METRIC_OBSERVE("macro_histogram", 0.5);
    
    // Verify values
    auto counter = collector.GetCounter("macro_counter");
    EXPECT_EQ(counter->GetValue(), 1.0);
    
    auto gauge = collector.GetGauge("macro_gauge");
    EXPECT_EQ(gauge->GetValue(), 42.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}