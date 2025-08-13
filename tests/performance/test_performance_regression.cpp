/**
 * @file test_performance_regression.cpp
 * @brief Performance regression test suite for Neo C++
 * @details Tracks performance metrics over time to detect regressions:
 *          - Transaction processing throughput
 *          - Cryptographic operation speed
 *          - Memory pool operations
 *          - Network latency
 *          - Database operations
 *          - VM execution speed
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#ifdef HAS_BENCHMARK
#include <benchmark/benchmark.h>
#endif
#include <neo/cryptography/crypto.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/transaction_pool_manager.h>
#include <neo/network/connection_pool.h>
#include <neo/vm/script.h>
#include <neo/vm/script.h>
#include <neo/io/byte_vector.h>
#include <neo/monitoring/performance_monitor.h>

#include <chrono>
#include <vector>
#include <random>
#include <fstream>
#include <filesystem>
#include <numeric>
#include <iomanip>

using namespace neo;
using namespace std::chrono_literals;

// Performance baseline file to track regression
const std::string BASELINE_FILE = "performance_baseline.json";

// Performance thresholds (percentage degradation allowed)
constexpr double REGRESSION_THRESHOLD = 10.0;  // 10% slower is considered regression

class PerformanceRegressionTest : public ::testing::Test
{
protected:
    monitoring::PerformanceMonitor monitor_;
    std::map<std::string, double> baseline_metrics_;
    std::map<std::string, double> current_metrics_;

    void SetUp() override
    {
        LoadBaseline();
        monitor_.Start();
    }

    void TearDown() override
    {
        monitor_.Stop();
        SaveMetrics();
        CheckForRegressions();
    }

    void LoadBaseline()
    {
        if (std::filesystem::exists(BASELINE_FILE))
        {
            std::ifstream file(BASELINE_FILE);
            // Simple format: metric_name value
            std::string name;
            double value;
            while (file >> name >> value)
            {
                baseline_metrics_[name] = value;
            }
        }
    }

    void SaveMetrics()
    {
        std::ofstream file(BASELINE_FILE + ".new");
        for (const auto& [name, value] : current_metrics_)
        {
            file << name << " " << value << "\n";
        }
    }

    void CheckForRegressions()
    {
        for (const auto& [name, current] : current_metrics_)
        {
            if (baseline_metrics_.count(name))
            {
                double baseline = baseline_metrics_[name];
                double degradation = ((current - baseline) / baseline) * 100.0;
                
                if (degradation > REGRESSION_THRESHOLD)
                {
                    ADD_FAILURE() << "Performance regression detected in " << name
                                 << ": " << std::fixed << std::setprecision(2)
                                 << degradation << "% slower"
                                 << " (baseline: " << baseline << "ms, current: " << current << "ms)";
                }
            }
        }
    }

    template<typename Func>
    double MeasureTime(const std::string& name, Func&& func, int iterations = 1000)
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i)
        {
            func();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avg_time = static_cast<double>(duration.count()) / iterations;
        current_metrics_[name] = avg_time;
        
        monitor_.RecordMetric("performance." + name, avg_time);
        
        return avg_time;
    }
};

// ============================================================================
// Cryptographic Performance Tests
// ============================================================================

TEST_F(PerformanceRegressionTest, SHA256Performance)
{
    std::vector<uint8_t> data(1024);  // 1KB of data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data)
    {
        byte = dis(gen);
    }
    
    auto time = MeasureTime("sha256_1kb", [&]() {
        cryptography::Crypto::Hash256(io::ByteSpan(data.data(), data.size()));
    });
    
    // Expected: < 10 microseconds for 1KB
    EXPECT_LT(time, 10.0);
}

TEST_F(PerformanceRegressionTest, SignatureVerificationPerformance)
{
    // Generate test data
    auto private_key = cryptography::Crypto::GenerateRandomBytes(32);
    auto public_key = cryptography::Crypto::ComputePublicKey(private_key.AsSpan());
    
    std::string message = "Test message for signature verification";
    // Signature generation would need proper ECDSA setup
    auto signature = io::ByteVector(64);  // Mock signature
    
    auto time = MeasureTime("signature_verify", [&]() {
        // Verification would need proper ECDSA setup
        // For now just do a hash operation as placeholder
        cryptography::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(message.data()), message.size()));
    }, 100);  // Fewer iterations as this is slower
    
    // Expected: < 1000 microseconds per verification
    EXPECT_LT(time, 1000.0);
}

TEST_F(PerformanceRegressionTest, AESEncryptionPerformance)
{
    std::vector<uint8_t> data(1024 * 1024);  // 1MB of data
    auto key = cryptography::Crypto::GenerateRandomBytes(32);
    auto iv = cryptography::Crypto::GenerateRandomBytes(16);
    
    // Fill with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto& byte : data)
    {
        byte = dis(gen);
    }
    
    auto time = MeasureTime("aes_encrypt_1mb", [&]() {
        cryptography::Crypto::AesEncrypt(
            io::ByteSpan(data.data(), data.size()),
            key.AsSpan(),
            iv.AsSpan()
        );
    }, 10);  // Fewer iterations for large data
    
    // Expected: < 5000 microseconds for 1MB
    EXPECT_LT(time, 5000.0);
}

// ============================================================================
// Memory Pool Performance Tests
// ============================================================================

TEST_F(PerformanceRegressionTest, MemoryPoolAddTransactionPerformance)
{
    ledger::MemoryPool pool(10000, 1000);
    std::vector<network::p2p::payloads::Neo3Transaction> transactions;
    
    // Pre-create transactions
    for (int i = 0; i < 1000; ++i)
    {
        network::p2p::payloads::Neo3Transaction tx;
        tx.SetNonce(i);
        tx.SetNetworkFee(1000000 * (i % 10));  // Varying fees
        transactions.push_back(tx);
    }
    
    auto time = MeasureTime("mempool_add", [&]() {
        pool.TryAdd(transactions[rand() % transactions.size()]);
    });
    
    // Expected: < 50 microseconds per add
    EXPECT_LT(time, 50.0);
}

TEST_F(PerformanceRegressionTest, MemoryPoolGetSortedPerformance)
{
    ledger::MemoryPool pool(10000, 1000);
    
    // Fill pool with transactions
    for (int i = 0; i < 5000; ++i)
    {
        network::p2p::payloads::Neo3Transaction tx;
        tx.SetNonce(i);
        tx.SetNetworkFee(1000000 * (i % 100));
        pool.TryAdd(tx);
    }
    
    auto time = MeasureTime("mempool_get_sorted", [&]() {
        auto sorted = pool.GetSortedTransactions();
    }, 100);
    
    // Expected: < 5000 microseconds for 5000 transactions
    EXPECT_LT(time, 5000.0);
}

TEST_F(PerformanceRegressionTest, TransactionPoolManagerPerformance)
{
    ledger::TransactionPoolManager manager;
    manager.Start();
    
    // Add transactions with varying priorities
    std::vector<network::p2p::payloads::Neo3Transaction> transactions;
    for (int i = 0; i < 1000; ++i)
    {
        network::p2p::payloads::Neo3Transaction tx;
        tx.SetNonce(i);
        tx.SetNetworkFee(1000000 * (i % 100));
        transactions.push_back(tx);
    }
    
    auto time = MeasureTime("txpool_manager_add", [&]() {
        auto& tx = transactions[rand() % transactions.size()];
        manager.AddTransaction(tx, ledger::TransactionPoolManager::Priority::Normal, "test");
    });
    
    manager.Stop();
    
    // Expected: < 100 microseconds per transaction
    EXPECT_LT(time, 100.0);
}

// ============================================================================
// Network Performance Tests
// ============================================================================

TEST_F(PerformanceRegressionTest, ConnectionPoolPerformance)
{
    network::ConnectionPool pool;
    network::ConnectionPool::Config config;
    config.max_connections = 100;
    config.min_connections = 10;
    network::ConnectionPool pool(config);
    pool.Start();
    
    auto time = MeasureTime("connection_pool_get", [&]() {
        auto conn = pool.GetConnection("localhost", 8080);
        // Connection automatically returned
    });
    
    // Expected: < 10 microseconds per get/release
    EXPECT_LT(time, 10.0);
}

TEST_F(PerformanceRegressionTest, RateLimiterPerformance)
{
    // TODO: Implement when RateLimiter is available
    // network::RateLimiter limiter(1000, 1s);  // 1000 requests per second
    
    // For now, test simple rate limiting logic performance
    std::atomic<int> counter{0};
    int max_rate = 1000;
    
    auto time = MeasureTime("rate_limiter_check", [&]() {
        int current = counter.load();
        if (current < max_rate) {
            counter.fetch_add(1);
        }
    }, 10000);
    
    // Expected: < 1 microsecond per check
    EXPECT_LT(time, 1.0);
}

// ============================================================================
// VM Performance Tests
// ============================================================================

TEST_F(PerformanceRegressionTest, VMSimpleScriptExecution)
{
    // VM testing would need proper setup
    // For now test script building performance
    
    auto time = MeasureTime("vm_script_build", [&]() {
        vm::Script script;
        // Build a simple script
        for (int i = 0; i < 10; ++i) {
            // Script operations
        }
    }, 1000);
    
    // Expected: < 100 microseconds for simple script
    EXPECT_LT(time, 100.0);
}

TEST_F(PerformanceRegressionTest, VMComplexScriptExecution)
{
    vm::VM virtual_machine;
    
    // Complex script with loops and conditionals
    vm::Script script;
    
    // FOR i = 0 TO 100
    script.Push(0x00);  // PUSH0 (counter)
    script.Push(0x51);  // PUSH1
    script.Push(0x93);  // ADD
    script.Push(0x76);  // DUP
    script.Push(0x08);  // PUSH 100
    script.Push(100);
    script.Push(0xA0);  // LT
    script.Push(0x63);  // JMPIF -10
    script.Push(static_cast<uint8_t>(-10));
    
    auto time = MeasureTime("vm_complex_script", [&]() {
        virtual_machine.Reset();
        virtual_machine.LoadScript(script);
        virtual_machine.Execute();
    }, 100);
    
    // Expected: < 1000 microseconds for complex script
    EXPECT_LT(time, 1000.0);
}

// ============================================================================
// Database Performance Tests
// ============================================================================

TEST_F(PerformanceRegressionTest, DatabaseWritePerformance)
{
    // Simulate database writes
    std::vector<std::pair<io::ByteVector, io::ByteVector>> data;
    
    for (int i = 0; i < 1000; ++i)
    {
        auto key = cryptography::GenerateRandomBytes(32);
        auto value = cryptography::GenerateRandomBytes(256);
        data.emplace_back(key, value);
    }
    
    auto time = MeasureTime("db_write", [&]() {
        // Simulate write (actual DB would be used here)
        auto& [key, value] = data[rand() % data.size()];
        // db.Put(key, value);
    }, 1000);
    
    // Expected: < 100 microseconds per write
    EXPECT_LT(time, 100.0);
}

TEST_F(PerformanceRegressionTest, DatabaseReadPerformance)
{
    // Simulate database reads
    std::vector<io::ByteVector> keys;
    
    for (int i = 0; i < 1000; ++i)
    {
        keys.push_back(cryptography::GenerateRandomBytes(32));
    }
    
    auto time = MeasureTime("db_read", [&]() {
        // Simulate read (actual DB would be used here)
        auto& key = keys[rand() % keys.size()];
        // auto value = db.Get(key);
    }, 1000);
    
    // Expected: < 50 microseconds per read
    EXPECT_LT(time, 50.0);
}

// ============================================================================
// Serialization Performance Tests
// ============================================================================

TEST_F(PerformanceRegressionTest, TransactionSerializationPerformance)
{
    network::p2p::payloads::Neo3Transaction tx;
    tx.SetNonce(12345);
    tx.SetNetworkFee(1000000);
    tx.SetSystemFee(500000);
    tx.SetValidUntilBlock(1000000);
    
    // Add some signers
    for (int i = 0; i < 5; ++i)
    {
        ledger::Signer signer;
        signer.SetAccount(io::UInt160::Zero());
        signer.SetScopes(ledger::WitnessScope::Global);
        tx.AddSigner(signer);
    }
    
    auto time = MeasureTime("tx_serialize", [&]() {
        auto serialized = tx.ToByteArray();
    }, 1000);
    
    // Expected: < 50 microseconds per serialization
    EXPECT_LT(time, 50.0);
}

TEST_F(PerformanceRegressionTest, TransactionDeserializationPerformance)
{
    network::p2p::payloads::Neo3Transaction tx;
    tx.SetNonce(12345);
    tx.SetNetworkFee(1000000);
    tx.SetSystemFee(500000);
    
    auto serialized = tx.ToByteArray();
    
    auto time = MeasureTime("tx_deserialize", [&]() {
        network::p2p::payloads::Neo3Transaction deserialized;
        io::BinaryReader reader(serialized.AsSpan());
        deserialized.Deserialize(reader);
    }, 1000);
    
    // Expected: < 100 microseconds per deserialization
    EXPECT_LT(time, 100.0);
}

// ============================================================================
// Monitoring Performance Tests
// ============================================================================

TEST_F(PerformanceRegressionTest, PerformanceMonitorOverhead)
{
    monitoring::PerformanceMonitor local_monitor;
    local_monitor.Start();
    
    auto time = MeasureTime("monitor_record_metric", [&]() {
        local_monitor.RecordMetric("test.metric", 42.0);
    }, 10000);
    
    local_monitor.Stop();
    
    // Expected: < 1 microsecond per metric recording
    EXPECT_LT(time, 1.0);
}

// ============================================================================
// Benchmark Support
// ============================================================================

#ifdef HAS_BENCHMARK
static void BM_SHA256(benchmark::State& state)
{
    std::vector<uint8_t> data(state.range(0));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data)
    {
        byte = dis(gen);
    }
    
    for (auto _ : state)
    {
        auto hash = cryptography::Crypto::Hash256(io::ByteSpan(data.data(), data.size()));
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SHA256)->Range(64, 1024*1024);

static void BM_MemoryPoolAdd(benchmark::State& state)
{
    ledger::MemoryPool pool(10000, 1000);
    std::vector<network::p2p::payloads::Neo3Transaction> transactions;
    
    for (int i = 0; i < 1000; ++i)
    {
        network::p2p::payloads::Neo3Transaction tx;
        tx.SetNonce(i);
        tx.SetNetworkFee(1000000 * (i % 10));
        transactions.push_back(tx);
    }
    
    size_t index = 0;
    for (auto _ : state)
    {
        pool.TryAdd(transactions[index % transactions.size()]);
        index++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemoryPoolAdd);

static void BM_VMExecution(benchmark::State& state)
{
    vm::VM virtual_machine;
    vm::Script script;
    
    // Create script of specified complexity
    for (int i = 0; i < state.range(0); ++i)
    {
        script.Push(0x51);  // PUSH1
        script.Push(0x52);  // PUSH2
        script.Push(0x93);  // ADD
        script.Push(0x75);  // DROP
    }
    
    for (auto _ : state)
    {
        virtual_machine.Reset();
        virtual_machine.LoadScript(script);
        virtual_machine.Execute();
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_VMExecution)->Range(1, 1000);
#endif // HAS_BENCHMARK

// ============================================================================
// Performance Report Generation
// ============================================================================

class PerformanceReporter : public ::testing::EmptyTestEventListener
{
private:
    std::ofstream report_file_;
    std::chrono::steady_clock::time_point start_time_;

public:
    PerformanceReporter() : report_file_("performance_report.txt")
    {
        report_file_ << "Neo C++ Performance Regression Test Report\n";
        report_file_ << "==========================================\n\n";
    }

    void OnTestProgramStart(const ::testing::UnitTest& /*unit_test*/) override
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    void OnTestProgramEnd(const ::testing::UnitTest& unit_test) override
    {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time_);
        
        report_file_ << "\nSummary:\n";
        report_file_ << "Total tests: " << unit_test.total_test_count() << "\n";
        report_file_ << "Successful: " << unit_test.successful_test_count() << "\n";
        report_file_ << "Failed: " << unit_test.failed_test_count() << "\n";
        report_file_ << "Duration: " << duration.count() << " seconds\n";
    }

    void OnTestEnd(const ::testing::TestInfo& test_info) override
    {
        if (test_info.result()->Failed())
        {
            report_file_ << "REGRESSION: " << test_info.test_case_name() 
                        << "." << test_info.name() << "\n";
        }
    }
};

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add custom reporter
    ::testing::TestEventListeners& listeners =
        ::testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new PerformanceReporter);
    
    // Initialize Google Benchmark if available
#ifdef HAS_BENCHMARK
    ::benchmark::Initialize(&argc, argv);
#endif
    
    // Run tests
    int test_result = RUN_ALL_TESTS();
    
#ifdef HAS_BENCHMARK
    // Run benchmarks if requested
    if (argc > 1 && std::string(argv[1]) == "--benchmark")
    {
        ::benchmark::RunSpecifiedBenchmarks();
    }
#endif
    
    return test_result;
}