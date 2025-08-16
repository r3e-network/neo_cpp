/**
 * @file stress_test_runner.cpp
 * @brief Comprehensive stress testing suite for Neo C++
 */

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <random>
#include <memory>
#include <functional>
#include <iomanip>

// Neo includes
#include <neo/vm/execution_engine.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/cryptography/sha256.h>
#include <neo/cryptography/ecdsa.h>
#include <neo/network/p2p_protocol.h>
#include <neo/persistence/store.h>

using namespace neo;
using namespace std::chrono;

// Stress test configuration
struct StressConfig {
    size_t num_threads = std::thread::hardware_concurrency();
    size_t operations_per_thread = 10000;
    size_t duration_seconds = 60;
    bool continuous = false;
    bool verbose = false;
};

// Test results
struct TestResults {
    std::atomic<size_t> total_operations{0};
    std::atomic<size_t> successful_operations{0};
    std::atomic<size_t> failed_operations{0};
    std::atomic<size_t> total_time_ms{0};
    high_resolution_clock::time_point start_time;
    high_resolution_clock::time_point end_time;
    
    double GetSuccessRate() const {
        size_t total = total_operations.load();
        return total > 0 ? (successful_operations.load() * 100.0 / total) : 0;
    }
    
    double GetOpsPerSecond() const {
        auto duration = duration_cast<milliseconds>(end_time - start_time).count();
        return duration > 0 ? (total_operations.load() * 1000.0 / duration) : 0;
    }
    
    void Print() const {
        std::cout << "\n=== Stress Test Results ===" << std::endl;
        std::cout << "Total Operations: " << total_operations.load() << std::endl;
        std::cout << "Successful: " << successful_operations.load() << std::endl;
        std::cout << "Failed: " << failed_operations.load() << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(2) 
                  << GetSuccessRate() << "%" << std::endl;
        std::cout << "Operations/sec: " << std::fixed << std::setprecision(0)
                  << GetOpsPerSecond() << std::endl;
        std::cout << "Duration: " 
                  << duration_cast<seconds>(end_time - start_time).count() 
                  << " seconds" << std::endl;
    }
};

// Base stress test class
class StressTest {
protected:
    StressConfig config_;
    TestResults results_;
    std::atomic<bool> stop_flag_{false};
    
public:
    StressTest(const StressConfig& config) : config_(config) {}
    virtual ~StressTest() = default;
    
    virtual void Run() {
        std::cout << "Starting " << GetName() << " with " 
                  << config_.num_threads << " threads..." << std::endl;
        
        results_.start_time = high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (size_t i = 0; i < config_.num_threads; ++i) {
            threads.emplace_back([this, i]() {
                RunThread(i);
            });
        }
        
        // If continuous mode, run for specified duration
        if (config_.continuous) {
            std::this_thread::sleep_for(seconds(config_.duration_seconds));
            stop_flag_ = true;
        }
        
        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }
        
        results_.end_time = high_resolution_clock::now();
        results_.Print();
    }
    
    virtual std::string GetName() const = 0;
    virtual void RunThread(size_t thread_id) = 0;
    
    const TestResults& GetResults() const { return results_; }
};

// VM execution stress test
class VMStressTest : public StressTest {
public:
    using StressTest::StressTest;
    
    std::string GetName() const override {
        return "VM Execution Stress Test";
    }
    
    void RunThread(size_t thread_id) override {
        std::mt19937 rng(thread_id);
        std::uniform_int_distribution<> op_dist(0, 255);
        
        size_t operations = 0;
        while ((config_.continuous && !stop_flag_) || 
               operations < config_.operations_per_thread) {
            
            try {
                // Generate random script
                std::vector<uint8_t> script;
                size_t script_size = 10 + (rng() % 100);
                for (size_t i = 0; i < script_size; ++i) {
                    script.push_back(op_dist(rng));
                }
                
                // Execute script
                vm::ExecutionEngine engine;
                engine.SetGasLimit(1000000);
                engine.LoadScript(io::ByteVector(script));
                
                auto start = high_resolution_clock::now();
                auto state = engine.Execute();
                auto duration = duration_cast<microseconds>(
                    high_resolution_clock::now() - start).count();
                
                results_.total_operations++;
                if (state == vm::VMState::Halt || state == vm::VMState::Fault) {
                    results_.successful_operations++;
                } else {
                    results_.failed_operations++;
                }
                
                results_.total_time_ms += duration / 1000;
                operations++;
                
            } catch (...) {
                results_.failed_operations++;
                operations++;
            }
        }
    }
};

// Cryptography stress test
class CryptoStressTest : public StressTest {
public:
    using StressTest::StressTest;
    
    std::string GetName() const override {
        return "Cryptography Stress Test";
    }
    
    void RunThread(size_t thread_id) override {
        std::mt19937 rng(thread_id);
        
        size_t operations = 0;
        while ((config_.continuous && !stop_flag_) || 
               operations < config_.operations_per_thread) {
            
            try {
                // Generate random data
                std::vector<uint8_t> data(32 + (rng() % 1024));
                for (auto& b : data) {
                    b = rng() % 256;
                }
                io::ByteVector input(data);
                
                // SHA256 hashing
                auto sha = cryptography::SHA256::ComputeHash(input);
                
                // ECDSA operations
                cryptography::KeyPair kp;
                auto signature = kp.Sign(sha);
                bool valid = kp.Verify(sha, signature);
                
                results_.total_operations++;
                if (valid) {
                    results_.successful_operations++;
                } else {
                    results_.failed_operations++;
                }
                
                operations++;
                
            } catch (...) {
                results_.failed_operations++;
                operations++;
            }
        }
    }
};

// Blockchain stress test
class BlockchainStressTest : public StressTest {
private:
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<ledger::MemPool> mempool_;
    std::mutex blockchain_mutex_;
    
public:
    BlockchainStressTest(const StressConfig& config) 
        : StressTest(config),
          blockchain_(std::make_shared<ledger::Blockchain>()),
          mempool_(std::make_shared<ledger::MemPool>()) {}
    
    std::string GetName() const override {
        return "Blockchain Stress Test";
    }
    
    void RunThread(size_t thread_id) override {
        std::mt19937 rng(thread_id);
        
        size_t operations = 0;
        while ((config_.continuous && !stop_flag_) || 
               operations < config_.operations_per_thread) {
            
            try {
                // Create random transaction
                ledger::Transaction tx;
                tx.Version = 0;
                tx.Nonce = rng();
                tx.SystemFee = rng() % 1000000;
                tx.NetworkFee = rng() % 1000000;
                tx.ValidUntilBlock = blockchain_->GetHeight() + 100;
                
                // Add to mempool
                {
                    std::lock_guard<std::mutex> lock(blockchain_mutex_);
                    mempool_->Add(tx);
                }
                
                // Occasionally create blocks
                if (operations % 100 == 0) {
                    ledger::Block block;
                    block.Version = 0;
                    block.Index = blockchain_->GetHeight() + 1;
                    block.Timestamp = system_clock::now().time_since_epoch().count();
                    
                    // Add transactions from mempool
                    auto sorted_txs = mempool_->GetSortedTransactions();
                    for (size_t i = 0; i < std::min(size_t(10), sorted_txs.size()); ++i) {
                        block.Transactions.push_back(sorted_txs[i]);
                    }
                    
                    {
                        std::lock_guard<std::mutex> lock(blockchain_mutex_);
                        blockchain_->AddBlock(block);
                        
                        // Remove included transactions from mempool
                        for (const auto& tx : block.Transactions) {
                            mempool_->Remove(tx.GetHash());
                        }
                    }
                }
                
                results_.total_operations++;
                results_.successful_operations++;
                operations++;
                
            } catch (...) {
                results_.failed_operations++;
                operations++;
            }
        }
    }
};

// Network stress test
class NetworkStressTest : public StressTest {
private:
    std::shared_ptr<network::P2PProtocol> protocol_;
    std::mutex protocol_mutex_;
    
public:
    NetworkStressTest(const StressConfig& config)
        : StressTest(config),
          protocol_(std::make_shared<network::P2PProtocol>()) {}
    
    std::string GetName() const override {
        return "Network Protocol Stress Test";
    }
    
    void RunThread(size_t thread_id) override {
        std::mt19937 rng(thread_id);
        std::uniform_int_distribution<> ip_dist(1, 254);
        std::uniform_int_distribution<> port_dist(10000, 60000);
        
        size_t operations = 0;
        while ((config_.continuous && !stop_flag_) || 
               operations < config_.operations_per_thread) {
            
            try {
                // Simulate various network operations
                int op_type = rng() % 5;
                
                switch (op_type) {
                    case 0: {
                        // Add peer
                        char ip[20];
                        snprintf(ip, sizeof(ip), "192.168.%d.%d", 
                                ip_dist(rng), ip_dist(rng));
                        network::Peer peer(ip, port_dist(rng));
                        
                        std::lock_guard<std::mutex> lock(protocol_mutex_);
                        protocol_->AddPeer(peer);
                        break;
                    }
                    case 1: {
                        // Request blocks
                        std::lock_guard<std::mutex> lock(protocol_mutex_);
                        protocol_->RequestBlocks(rng() % 1000, rng() % 100 + 1);
                        break;
                    }
                    case 2: {
                        // Broadcast transaction
                        std::vector<uint8_t> tx_hash(32);
                        for (auto& b : tx_hash) b = rng() % 256;
                        
                        std::lock_guard<std::mutex> lock(protocol_mutex_);
                        protocol_->BroadcastTransaction(io::ByteVector(tx_hash));
                        break;
                    }
                    case 3: {
                        // Process incoming data
                        std::vector<uint8_t> data(100 + rng() % 1000);
                        for (auto& b : data) b = rng() % 256;
                        
                        std::lock_guard<std::mutex> lock(protocol_mutex_);
                        protocol_->ProcessIncomingData(io::ByteVector(data));
                        break;
                    }
                    case 4: {
                        // Remove random peer
                        std::lock_guard<std::mutex> lock(protocol_mutex_);
                        if (protocol_->GetPeerCount() > 0) {
                            protocol_->RemovePeer(rng());
                        }
                        break;
                    }
                }
                
                results_.total_operations++;
                results_.successful_operations++;
                operations++;
                
            } catch (...) {
                results_.failed_operations++;
                operations++;
            }
        }
    }
};

// Main stress test runner
int main(int argc, char* argv[]) {
    StressConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--threads" && i + 1 < argc) {
            config.num_threads = std::stoi(argv[++i]);
        } else if (arg == "--operations" && i + 1 < argc) {
            config.operations_per_thread = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            config.duration_seconds = std::stoi(argv[++i]);
        } else if (arg == "--continuous") {
            config.continuous = true;
        } else if (arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --threads N       Number of threads (default: hardware concurrency)" << std::endl;
            std::cout << "  --operations N    Operations per thread (default: 10000)" << std::endl;
            std::cout << "  --duration N      Duration in seconds for continuous mode (default: 60)" << std::endl;
            std::cout << "  --continuous      Run continuously for specified duration" << std::endl;
            std::cout << "  --verbose         Enable verbose output" << std::endl;
            return 0;
        }
    }
    
    std::cout << "==============================================\n";
    std::cout << "Neo C++ Stress Testing Suite\n";
    std::cout << "==============================================\n";
    std::cout << "Configuration:\n";
    std::cout << "  Threads: " << config.num_threads << "\n";
    std::cout << "  Operations/thread: " << config.operations_per_thread << "\n";
    std::cout << "  Continuous: " << (config.continuous ? "Yes" : "No") << "\n";
    if (config.continuous) {
        std::cout << "  Duration: " << config.duration_seconds << " seconds\n";
    }
    std::cout << "==============================================\n\n";
    
    // Run all stress tests
    std::vector<std::unique_ptr<StressTest>> tests;
    tests.push_back(std::make_unique<VMStressTest>(config));
    tests.push_back(std::make_unique<CryptoStressTest>(config));
    tests.push_back(std::make_unique<BlockchainStressTest>(config));
    tests.push_back(std::make_unique<NetworkStressTest>(config));
    
    TestResults overall_results;
    overall_results.start_time = high_resolution_clock::now();
    
    for (auto& test : tests) {
        test->Run();
        const auto& results = test->GetResults();
        overall_results.total_operations += results.total_operations;
        overall_results.successful_operations += results.successful_operations;
        overall_results.failed_operations += results.failed_operations;
    }
    
    overall_results.end_time = high_resolution_clock::now();
    
    std::cout << "\n==============================================\n";
    std::cout << "OVERALL RESULTS\n";
    std::cout << "==============================================";
    overall_results.Print();
    
    // Return non-zero if failure rate > 5%
    return overall_results.GetSuccessRate() < 95.0 ? 1 : 0;
}