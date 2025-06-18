# Neo C++ Testing Guide

## Overview

This guide covers comprehensive testing strategies for the Neo C++ blockchain node, including unit testing, integration testing, performance testing, and security testing. The testing framework ensures code quality, reliability, and performance.

## Testing Framework Architecture

### Test Structure
```
tests/
├── unit/                   # Unit tests for individual components
│   ├── consensus/         # Consensus algorithm tests
│   ├── cryptography/      # Cryptographic function tests
│   ├── ledger/           # Blockchain ledger tests
│   ├── network/          # Network protocol tests
│   ├── smartcontract/    # Smart contract tests
│   └── vm/               # Virtual machine tests
├── integration/           # Integration tests for subsystems
│   ├── blockchain/       # End-to-end blockchain tests
│   ├── network/          # Network integration tests
│   └── rpc/             # RPC API integration tests
├── performance/          # Performance and benchmark tests
│   ├── benchmarks/      # Micro-benchmarks
│   └── stress/          # Stress testing
├── security/             # Security and vulnerability tests
├── compatibility/        # Neo N3 compatibility tests
├── fixtures/             # Test data and fixtures
├── mocks/               # Mock implementations
└── utils/               # Testing utilities
```

## Unit Testing

### Test Framework: Catch2

#### Basic Test Structure
```cpp
// tests/unit/ledger/test_blockchain.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <memory>

#include "neo/ledger/blockchain.h"
#include "tests/mocks/mock_store.h"
#include "tests/mocks/mock_protocol_settings.h"
#include "tests/utils/test_helpers.h"

using namespace neo::ledger;
using namespace neo::tests;

class BlockchainTestFixture {
public:
    BlockchainTestFixture()
        : settings_(std::make_shared<MockProtocolSettings>())
        , store_(std::make_shared<MockStore>())
        , blockchain_(std::make_shared<Blockchain>(settings_, store_)) {
        // Setup test environment
    }

protected:
    std::shared_ptr<MockProtocolSettings> settings_;
    std::shared_ptr<MockStore> store_;
    std::shared_ptr<Blockchain> blockchain_;
};

TEST_CASE_METHOD(BlockchainTestFixture, "Blockchain initialization", "[blockchain][unit]") {
    SECTION("Successful initialization with valid parameters") {
        REQUIRE(blockchain_->Initialize());
        REQUIRE(blockchain_->GetHeight() == 0);
        REQUIRE(blockchain_->GetCurrentBlockHash() == UInt256::Zero());
    }
    
    SECTION("Initialization fails with null settings") {
        REQUIRE_THROWS_AS(
            Blockchain(nullptr, store_),
            std::invalid_argument
        );
    }
    
    SECTION("Initialization fails with null store") {
        REQUIRE_THROWS_AS(
            Blockchain(settings_, nullptr),
            std::invalid_argument
        );
    }
}

TEST_CASE_METHOD(BlockchainTestFixture, "Block processing", "[blockchain][unit]") {
    REQUIRE(blockchain_->Initialize());
    
    SECTION("Process valid genesis block") {
        auto genesisBlock = TestHelpers::CreateGenesisBlock();
        REQUIRE(blockchain_->ProcessBlock(genesisBlock));
        REQUIRE(blockchain_->GetHeight() == 1);
    }
    
    SECTION("Reject invalid block") {
        auto invalidBlock = TestHelpers::CreateInvalidBlock();
        REQUIRE_FALSE(blockchain_->ProcessBlock(invalidBlock));
        REQUIRE(blockchain_->GetHeight() == 0);
    }
    
    SECTION("Reject duplicate block") {
        auto block = TestHelpers::CreateValidBlock(1);
        REQUIRE(blockchain_->ProcessBlock(block));
        REQUIRE_FALSE(blockchain_->ProcessBlock(block)); // Duplicate
    }
}

TEST_CASE("Block validation rules", "[blockchain][unit]") {
    auto settings = std::make_shared<MockProtocolSettings>();
    
    SECTION("Block size limits") {
        auto oversizedBlock = TestHelpers::CreateOversizedBlock();
        REQUIRE_FALSE(oversizedBlock->Verify(settings));
    }
    
    SECTION("Transaction count limits") {
        auto block = TestHelpers::CreateBlockWithTransactions(1000);
        REQUIRE_FALSE(block->Verify(settings)); // Exceeds max transactions
    }
    
    SECTION("Merkle root validation") {
        auto block = TestHelpers::CreateValidBlock(1);
        auto originalRoot = block->GetHeader().GetMerkleRoot();
        
        // Corrupt a transaction
        auto transactions = block->GetTransactions();
        transactions[0] = TestHelpers::CreateRandomTransaction();
        
        auto corruptedBlock = std::make_shared<Block>(block->GetHeader(), transactions);
        REQUIRE(corruptedBlock->CalculateMerkleRoot() != originalRoot);
        REQUIRE_FALSE(corruptedBlock->Verify(settings));
    }
}
```

#### Parameterized Tests
```cpp
TEST_CASE("Hash function validation", "[cryptography][unit]") {
    using namespace neo::cryptography;
    
    // Test multiple input sizes
    auto testData = GENERATE(
        std::vector<uint8_t>{},                    // Empty
        std::vector<uint8_t>{0x42},               // Single byte
        std::vector<uint8_t>(32, 0xFF),           // 32 bytes
        std::vector<uint8_t>(1024, 0xAA)          // Large data
    );
    
    SECTION("SHA256 produces correct output length") {
        auto result = Hash::SHA256(testData);
        REQUIRE(result.Size() == 32);
    }
    
    SECTION("Hash160 produces correct output length") {
        auto result = Hash::Hash160(testData);
        REQUIRE(result.Size() == 20);
    }
    
    SECTION("Double hashing is deterministic") {
        auto hash1 = Hash::SHA256(testData);
        auto hash2 = Hash::SHA256(testData);
        REQUIRE(hash1 == hash2);
    }
}

TEST_CASE("Transaction validation with various fee scenarios", "[ledger][unit]") {
    auto systemFee = GENERATE(0ULL, 1000ULL, 1000000ULL, UINT64_MAX);
    auto networkFee = GENERATE(0ULL, 500ULL, 5000ULL, 50000ULL);
    
    auto transaction = TestHelpers::CreateTransaction();
    transaction->SetSystemFee(systemFee);
    transaction->SetNetworkFee(networkFee);
    
    CAPTURE(systemFee, networkFee);
    
    auto isValid = transaction->Verify(
        TestHelpers::GetDefaultSettings(),
        TestHelpers::GetEmptySnapshot(),
        TestHelpers::GetEmptyMempool()
    );
    
    // Validation logic based on fee amounts
    if (systemFee > 0 || networkFee > 0) {
        REQUIRE(isValid);
    }
}
```

### Mock Objects

#### Mock Store Implementation
```cpp
// tests/mocks/mock_store.h
#pragma once

#include <gmock/gmock.h>
#include "neo/persistence/istore.h"

namespace neo::tests {

class MockStore : public persistence::IStore {
public:
    MOCK_METHOD(std::shared_ptr<ISnapshot>, GetSnapshot, (), (override));
    MOCK_METHOD(void, Put, (const StorageKey& key, const StorageItem& value), (override));
    MOCK_METHOD(std::optional<StorageItem>, TryGet, (const StorageKey& key), (override));
    MOCK_METHOD(bool, Contains, (const StorageKey& key), (override));
    MOCK_METHOD(void, Delete, (const StorageKey& key), (override));
    MOCK_METHOD(void, PutBatch, (const std::vector<std::pair<StorageKey, StorageItem>>& batch), (override));
    MOCK_METHOD(void, DeleteBatch, (const std::vector<StorageKey>& keys), (override));
    MOCK_METHOD(std::unique_ptr<IIterator>, Seek, (const StorageKey& key), (override));
    MOCK_METHOD(std::unique_ptr<IIterator>, SeekAll, (), (override));
};

class MockSnapshot : public persistence::ISnapshot {
public:
    MOCK_METHOD(std::optional<StorageItem>, TryGet, (const StorageKey& key), (override));
    MOCK_METHOD(void, Put, (const StorageKey& key, const StorageItem& value), (override));
    MOCK_METHOD(void, Delete, (const StorageKey& key), (override));
    MOCK_METHOD(void, Commit, (), (override));
    MOCK_METHOD(bool, Contains, (const StorageKey& key), (override));
    MOCK_METHOD(std::unique_ptr<IIterator>, Seek, (const StorageKey& key), (override));
};

} // namespace neo::tests
```

#### Test Helper Utilities
```cpp
// tests/utils/test_helpers.h
#pragma once

#include <memory>
#include <vector>
#include <random>

#include "neo/ledger/block.h"
#include "neo/ledger/transaction.h"
#include "neo/protocol_settings.h"

namespace neo::tests {

class TestHelpers {
public:
    // Block creation helpers
    static std::shared_ptr<Block> CreateGenesisBlock();
    static std::shared_ptr<Block> CreateValidBlock(uint32_t index);
    static std::shared_ptr<Block> CreateInvalidBlock();
    static std::shared_ptr<Block> CreateOversizedBlock();
    static std::shared_ptr<Block> CreateBlockWithTransactions(size_t count);
    
    // Transaction creation helpers
    static std::shared_ptr<Transaction> CreateTransaction();
    static std::shared_ptr<Transaction> CreateRandomTransaction();
    static std::shared_ptr<Transaction> CreateTransactionWithFee(uint64_t systemFee, uint64_t networkFee);
    
    // Configuration helpers
    static std::shared_ptr<ProtocolSettings> GetDefaultSettings();
    static std::shared_ptr<ProtocolSettings> GetTestnetSettings();
    
    // Test environment helpers
    static std::shared_ptr<DataCache> GetEmptySnapshot();
    static std::shared_ptr<MemoryPool> GetEmptyMempool();
    
    // Cryptography helpers
    static std::vector<uint8_t> GenerateRandomBytes(size_t length);
    static UInt256 GenerateRandomHash();
    static UInt160 GenerateRandomScriptHash();
    
    // Time helpers
    static uint64_t GetCurrentTimestamp();
    static uint64_t GetTimestampInFuture(uint32_t secondsFromNow);
    
private:
    static std::random_device rd_;
    static std::mt19937 gen_;
};

} // namespace neo::tests
```

### VM Testing

#### Opcode Testing
```cpp
// tests/unit/vm/test_opcodes.cpp
#include <catch2/catch_test_macros.hpp>
#include "neo/vm/execution_engine.h"
#include "neo/vm/script_builder.h"
#include "tests/utils/vm_test_helpers.h"

using namespace neo::vm;
using namespace neo::tests;

TEST_CASE("Arithmetic opcodes", "[vm][opcodes][unit]") {
    auto engine = std::make_shared<ExecutionEngine>();
    
    SECTION("ADD operation") {
        ScriptBuilder builder;
        builder.EmitPush(5);
        builder.EmitPush(3);
        builder.Emit(OpCode::ADD);
        
        engine->LoadScript(builder.ToArray());
        auto result = engine->Execute();
        
        REQUIRE(result == VMState::HALT);
        REQUIRE(engine->GetResultStack().size() == 1);
        REQUIRE(engine->GetResultStack()[0]->GetInteger() == 8);
    }
    
    SECTION("Division by zero") {
        ScriptBuilder builder;
        builder.EmitPush(10);
        builder.EmitPush(0);
        builder.Emit(OpCode::DIV);
        
        engine->LoadScript(builder.ToArray());
        auto result = engine->Execute();
        
        REQUIRE(result == VMState::FAULT);
    }
    
    SECTION("Overflow handling") {
        ScriptBuilder builder;
        builder.EmitPush(std::numeric_limits<int64_t>::max());
        builder.EmitPush(1);
        builder.Emit(OpCode::ADD);
        
        engine->LoadScript(builder.ToArray());
        auto result = engine->Execute();
        
        // Should handle overflow gracefully
        REQUIRE(result == VMState::HALT);
    }
}

TEST_CASE("Stack manipulation opcodes", "[vm][opcodes][unit]") {
    auto engine = std::make_shared<ExecutionEngine>();
    
    SECTION("DUP operation") {
        ScriptBuilder builder;
        builder.EmitPush(42);
        builder.Emit(OpCode::DUP);
        
        engine->LoadScript(builder.ToArray());
        REQUIRE(engine->Execute() == VMState::HALT);
        
        auto stack = engine->GetResultStack();
        REQUIRE(stack.size() == 2);
        REQUIRE(stack[0]->GetInteger() == 42);
        REQUIRE(stack[1]->GetInteger() == 42);
    }
    
    SECTION("SWAP operation") {
        ScriptBuilder builder;
        builder.EmitPush(1);
        builder.EmitPush(2);
        builder.Emit(OpCode::SWAP);
        
        engine->LoadScript(builder.ToArray());
        REQUIRE(engine->Execute() == VMState::HALT);
        
        auto stack = engine->GetResultStack();
        REQUIRE(stack.size() == 2);
        REQUIRE(stack[0]->GetInteger() == 1);
        REQUIRE(stack[1]->GetInteger() == 2);
    }
}

TEST_CASE("Smart contract execution", "[vm][contracts][unit]") {
    auto engine = VmTestHelpers::CreateApplicationEngine();
    
    SECTION("Simple contract method call") {
        auto contractHash = VmTestHelpers::DeployTestContract();
        
        engine->CallContract(contractHash, "getValue", {});
        auto result = engine->Execute();
        
        REQUIRE(result == VMState::HALT);
        REQUIRE(engine->GetResultStack().size() == 1);
    }
    
    SECTION("Contract storage operations") {
        auto contractHash = VmTestHelpers::DeployStorageContract();
        
        // Store value
        std::vector<std::shared_ptr<StackItem>> args = {
            std::make_shared<ByteArrayStackItem>(std::vector<uint8_t>{'k', 'e', 'y'}),
            std::make_shared<IntegerStackItem>(42)
        };
        
        engine->CallContract(contractHash, "store", args);
        REQUIRE(engine->Execute() == VMState::HALT);
        
        // Retrieve value
        std::vector<std::shared_ptr<StackItem>> getArgs = {
            std::make_shared<ByteArrayStackItem>(std::vector<uint8_t>{'k', 'e', 'y'})
        };
        
        engine->CallContract(contractHash, "get", getArgs);
        REQUIRE(engine->Execute() == VMState::HALT);
        REQUIRE(engine->GetResultStack()[0]->GetInteger() == 42);
    }
}
```

## Integration Testing

### Blockchain Integration Tests
```cpp
// tests/integration/test_blockchain_integration.cpp
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <chrono>

#include "neo/node/neo_system.h"
#include "tests/utils/test_network.h"

using namespace std::chrono_literals;

TEST_CASE("Multi-node blockchain sync", "[integration][blockchain]") {
    TestNetwork network;
    
    // Create three nodes
    auto node1 = network.CreateNode("node1");
    auto node2 = network.CreateNode("node2");
    auto node3 = network.CreateNode("node3");
    
    // Start all nodes
    node1->Start();
    node2->Start();
    node3->Start();
    
    // Wait for network formation
    std::this_thread::sleep_for(5s);
    
    SECTION("Nodes establish connections") {
        REQUIRE(node1->GetPeerCount() >= 2);
        REQUIRE(node2->GetPeerCount() >= 2);
        REQUIRE(node3->GetPeerCount() >= 2);
    }
    
    SECTION("Block propagation across network") {
        // Generate block on node1
        auto block = node1->CreateAndProcessBlock();
        
        // Wait for propagation
        std::this_thread::sleep_for(3s);
        
        // Verify all nodes have the same height
        REQUIRE(node1->GetBlockHeight() == node2->GetBlockHeight());
        REQUIRE(node2->GetBlockHeight() == node3->GetBlockHeight());
        
        // Verify all nodes have the same latest block
        auto hash1 = node1->GetCurrentBlockHash();
        auto hash2 = node2->GetCurrentBlockHash();
        auto hash3 = node3->GetCurrentBlockHash();
        
        REQUIRE(hash1 == hash2);
        REQUIRE(hash2 == hash3);
    }
    
    SECTION("Transaction relay and confirmation") {
        // Create transaction on node1
        auto transaction = node1->CreateTestTransaction();
        
        // Broadcast transaction
        node1->BroadcastTransaction(transaction);
        
        // Wait for propagation
        std::this_thread::sleep_for(2s);
        
        // Verify transaction exists in all mempools
        REQUIRE(node1->GetMempool()->Contains(transaction->GetHash()));
        REQUIRE(node2->GetMempool()->Contains(transaction->GetHash()));
        REQUIRE(node3->GetMempool()->Contains(transaction->GetHash()));
        
        // Generate block containing transaction
        auto block = node2->CreateAndProcessBlock();
        
        // Wait for block propagation
        std::this_thread::sleep_for(3s);
        
        // Verify transaction is confirmed on all nodes
        REQUIRE(node1->GetTransaction(transaction->GetHash()) != nullptr);
        REQUIRE(node2->GetTransaction(transaction->GetHash()) != nullptr);
        REQUIRE(node3->GetTransaction(transaction->GetHash()) != nullptr);
    }
}

TEST_CASE("Network partition and recovery", "[integration][network]") {
    TestNetwork network;
    
    // Create 5 nodes for Byzantine fault tolerance
    std::vector<std::shared_ptr<TestNode>> nodes;
    for (int i = 0; i < 5; ++i) {
        nodes.push_back(network.CreateNode("node" + std::to_string(i)));
        nodes[i]->Start();
    }
    
    // Wait for full connectivity
    std::this_thread::sleep_for(10s);
    
    SECTION("Consensus continues with majority partition") {
        // Create network partition: 3 nodes vs 2 nodes
        network.CreatePartition({nodes[0], nodes[1], nodes[2]}, {nodes[3], nodes[4]});
        
        auto initialHeight = nodes[0]->GetBlockHeight();
        
        // Wait for consensus rounds
        std::this_thread::sleep_for(30s);
        
        // Majority partition should continue producing blocks
        REQUIRE(nodes[0]->GetBlockHeight() > initialHeight);
        REQUIRE(nodes[1]->GetBlockHeight() > initialHeight);
        REQUIRE(nodes[2]->GetBlockHeight() > initialHeight);
        
        // Minority partition should stall
        REQUIRE(nodes[3]->GetBlockHeight() == initialHeight);
        REQUIRE(nodes[4]->GetBlockHeight() == initialHeight);
    }
    
    SECTION("Network heals after partition resolution") {
        // Create and resolve partition
        network.CreatePartition({nodes[0], nodes[1], nodes[2]}, {nodes[3], nodes[4]});
        std::this_thread::sleep_for(30s);
        
        auto majorityHeight = nodes[0]->GetBlockHeight();
        auto minorityHeight = nodes[3]->GetBlockHeight();
        
        // Heal partition
        network.HealPartition();
        std::this_thread::sleep_for(20s);
        
        // All nodes should converge to the same height
        for (const auto& node : nodes) {
            REQUIRE(node->GetBlockHeight() >= majorityHeight);
        }
        
        // Verify chain consistency
        auto referenceHash = nodes[0]->GetCurrentBlockHash();
        for (size_t i = 1; i < nodes.size(); ++i) {
            REQUIRE(nodes[i]->GetCurrentBlockHash() == referenceHash);
        }
    }
}
```

### RPC Integration Tests
```cpp
// tests/integration/test_rpc_integration.cpp
#include <catch2/catch_test_macros.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "neo/rpc/rpc_server.h"
#include "tests/utils/rpc_test_client.h"

TEST_CASE("RPC API integration", "[integration][rpc]") {
    auto node = TestHelpers::CreateTestNode();
    auto rpcServer = std::make_shared<RpcServer>(node, "127.0.0.1", 20332);
    
    rpcServer->Start();
    auto client = std::make_shared<RpcTestClient>("http://127.0.0.1:20332");
    
    SECTION("Basic blockchain queries") {
        // Test getversion
        auto version = client->Call("getversion", {});
        REQUIRE(version["result"]["network"].get<uint32_t>() == 860833102);
        
        // Test getblockcount
        auto blockCount = client->Call("getblockcount", {});
        REQUIRE(blockCount["result"].get<uint32_t>() >= 0);
        
        // Test getbestblockhash
        auto bestHash = client->Call("getbestblockhash", {});
        REQUIRE_FALSE(bestHash["result"].get<std::string>().empty());
    }
    
    SECTION("Transaction submission and retrieval") {
        // Create test transaction
        auto transaction = TestHelpers::CreateSignedTransaction();
        auto rawTx = transaction->ToHexString();
        
        // Submit transaction
        auto submitResult = client->Call("sendrawtransaction", {rawTx});
        REQUIRE(submitResult.contains("result"));
        
        auto txHash = transaction->GetHash().ToString();
        
        // Retrieve transaction
        auto getTxResult = client->Call("getrawtransaction", {txHash, true});
        REQUIRE(getTxResult["result"]["hash"].get<std::string>() == txHash);
    }
    
    SECTION("Smart contract invocation") {
        // Deploy test contract
        auto contractHash = TestHelpers::DeployTestContract(node);
        
        // Invoke contract function
        nlohmann::json params = {
            contractHash.ToString(),
            "testMethod",
            {42, "test_string"}
        };
        
        auto result = client->Call("invokefunction", params);
        REQUIRE(result["result"]["state"].get<std::string>() == "HALT");
        REQUIRE_FALSE(result["result"]["stack"].empty());
    }
    
    SECTION("Bulk operations performance") {
        // Test multiple concurrent requests
        std::vector<std::thread> threads;
        std::atomic<int> successCount{0};
        
        for (int i = 0; i < 100; ++i) {
            threads.emplace_back([&client, &successCount]() {
                auto result = client->Call("getblockcount", {});
                if (result.contains("result")) {
                    successCount++;
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(successCount >= 95); // Allow for some failures
    }
}
```

## Performance Testing

### Benchmark Framework
```cpp
// tests/performance/benchmark_vm.cpp
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "neo/vm/execution_engine.h"
#include "neo/vm/script_builder.h"
#include "tests/utils/performance_helpers.h"

TEST_CASE("VM Performance Benchmarks", "[benchmark][vm]") {
    
    BENCHMARK("Simple arithmetic operations") {
        auto engine = std::make_shared<ExecutionEngine>();
        ScriptBuilder builder;
        
        // Create script with 1000 ADD operations
        for (int i = 0; i < 1000; ++i) {
            builder.EmitPush(i);
            builder.EmitPush(i + 1);
            builder.Emit(OpCode::ADD);
            builder.Emit(OpCode::DROP);
        }
        
        engine->LoadScript(builder.ToArray());
        return engine->Execute();
    };
    
    BENCHMARK("Stack manipulation") {
        auto engine = std::make_shared<ExecutionEngine>();
        ScriptBuilder builder;
        
        // Push 1000 values and manipulate stack
        for (int i = 0; i < 1000; ++i) {
            builder.EmitPush(i);
        }
        for (int i = 0; i < 500; ++i) {
            builder.Emit(OpCode::DUP);
            builder.Emit(OpCode::SWAP);
            builder.Emit(OpCode::DROP);
        }
        
        engine->LoadScript(builder.ToArray());
        return engine->Execute();
    };
    
    BENCHMARK("Cryptographic operations") {
        auto engine = std::make_shared<ExecutionEngine>();
        ScriptBuilder builder;
        
        // Hash operations
        for (int i = 0; i < 100; ++i) {
            builder.EmitPush(PerformanceHelpers::GenerateRandomData(32));
            builder.EmitSysCall("System.Crypto.SHA256");
        }
        
        engine->LoadScript(builder.ToArray());
        return engine->Execute();
    };
}

TEST_CASE("Blockchain Performance Benchmarks", "[benchmark][blockchain]") {
    auto blockchain = PerformanceHelpers::CreateBenchmarkBlockchain();
    
    BENCHMARK("Block processing") {
        auto block = PerformanceHelpers::CreateBlockWithTransactions(100);
        return blockchain->ProcessBlock(block);
    };
    
    BENCHMARK("Transaction validation") {
        auto transactions = PerformanceHelpers::CreateTransactionBatch(1000);
        
        bool allValid = true;
        for (const auto& tx : transactions) {
            if (!tx->Verify(blockchain->GetSettings(),
                           blockchain->GetSnapshot(),
                           blockchain->GetMempool())) {
                allValid = false;
                break;
            }
        }
        return allValid;
    };
    
    BENCHMARK("Storage operations") {
        auto store = blockchain->GetStore();
        auto keys = PerformanceHelpers::GenerateStorageKeys(1000);
        auto values = PerformanceHelpers::GenerateStorageValues(1000);
        
        for (size_t i = 0; i < keys.size(); ++i) {
            store->Put(keys[i], values[i]);
        }
        
        // Read operations
        for (const auto& key : keys) {
            auto value = store->TryGet(key);
        }
        
        return true;
    };
}
```

### Stress Testing
```cpp
// tests/performance/stress_test.cpp
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <chrono>
#include <atomic>

#include "tests/utils/stress_test_helpers.h"

TEST_CASE("Stress Test - High Transaction Volume", "[stress]") {
    auto node = StressTestHelpers::CreateHighPerformanceNode();
    node->Start();
    
    std::atomic<int> processedTxs{0};
    std::atomic<int> failedTxs{0};
    std::atomic<bool> shouldStop{false};
    
    // Transaction generator thread
    std::thread generator([&]() {
        while (!shouldStop) {
            auto batch = StressTestHelpers::CreateTransactionBatch(100);
            for (const auto& tx : batch) {
                if (node->GetMempool()->TryAdd(tx)) {
                    processedTxs++;
                } else {
                    failedTxs++;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Block producer thread
    std::thread producer([&]() {
        while (!shouldStop) {
            node->CreateAndProcessBlock();
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
    });
    
    // Run stress test for 5 minutes
    std::this_thread::sleep_for(std::chrono::minutes(5));
    shouldStop = true;
    
    generator.join();
    producer.join();
    
    // Verify performance metrics
    double successRate = static_cast<double>(processedTxs) / (processedTxs + failedTxs);
    REQUIRE(successRate > 0.95); // 95% success rate
    REQUIRE(processedTxs > 10000); // Minimum throughput
    
    // Verify node stability
    REQUIRE(node->IsRunning());
    REQUIRE(node->GetBlockHeight() > 0);
}

TEST_CASE("Stress Test - Memory Usage", "[stress][memory]") {
    auto node = StressTestHelpers::CreateMemoryLimitedNode(1024 * 1024 * 1024); // 1GB limit
    node->Start();
    
    auto initialMemory = StressTestHelpers::GetMemoryUsage();
    
    // Generate large number of transactions
    for (int i = 0; i < 100000; ++i) {
        auto tx = StressTestHelpers::CreateLargeTransaction();
        node->GetMempool()->TryAdd(tx);
        
        if (i % 1000 == 0) {
            auto currentMemory = StressTestHelpers::GetMemoryUsage();
            auto memoryIncrease = currentMemory - initialMemory;
            
            // Ensure memory usage stays within reasonable bounds
            REQUIRE(memoryIncrease < 500 * 1024 * 1024); // 500MB increase max
        }
    }
    
    // Force garbage collection and verify cleanup
    node->ForceCleanup();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    auto finalMemory = StressTestHelpers::GetMemoryUsage();
    auto totalIncrease = finalMemory - initialMemory;
    
    REQUIRE(totalIncrease < 200 * 1024 * 1024); // 200MB final increase max
}
```

## Security Testing

### Vulnerability Testing
```cpp
// tests/security/test_security.cpp
#include <catch2/catch_test_macros.hpp>
#include "tests/utils/security_test_helpers.h"

TEST_CASE("Input validation security", "[security]") {
    auto node = SecurityTestHelpers::CreateSecureNode();
    
    SECTION("Buffer overflow protection") {
        // Attempt to create oversized payloads
        auto oversizedBlock = SecurityTestHelpers::CreateOversizedPayload(10 * 1024 * 1024);
        REQUIRE_FALSE(node->ProcessMessage(oversizedBlock));
        
        auto oversizedTransaction = SecurityTestHelpers::CreateOversizedTransaction();
        REQUIRE_FALSE(node->GetMempool()->TryAdd(oversizedTransaction));
    }
    
    SECTION("Script injection protection") {
        // Attempt malicious script execution
        auto maliciousScript = SecurityTestHelpers::CreateMaliciousScript();
        auto engine = std::make_shared<ExecutionEngine>();
        
        engine->LoadScript(maliciousScript);
        auto result = engine->Execute();
        
        // Should fail or timeout, not execute malicious code
        REQUIRE(result != VMState::HALT);
    }
    
    SECTION("Denial of service protection") {
        // Rapid connection attempts
        std::vector<std::thread> attackThreads;
        std::atomic<int> rejectedConnections{0};
        
        for (int i = 0; i < 1000; ++i) {
            attackThreads.emplace_back([&]() {
                if (!SecurityTestHelpers::AttemptConnection(node)) {
                    rejectedConnections++;
                }
            });
        }
        
        for (auto& thread : attackThreads) {
            thread.join();
        }
        
        // Should reject excessive connections
        REQUIRE(rejectedConnections > 900);
        REQUIRE(node->IsRunning()); // Node should remain stable
    }
}

TEST_CASE("Cryptographic security", "[security][crypto]") {
    SECTION("Key generation entropy") {
        std::set<std::string> generatedKeys;
        
        // Generate 1000 keys and ensure uniqueness
        for (int i = 0; i < 1000; ++i) {
            auto keyPair = SecurityTestHelpers::GenerateKeyPair();
            auto privateKey = keyPair->GetPrivateKey();
            auto keyHex = SecurityTestHelpers::ToHexString(privateKey);
            
            REQUIRE(generatedKeys.find(keyHex) == generatedKeys.end());
            generatedKeys.insert(keyHex);
        }
    }
    
    SECTION("Signature verification security") {
        auto keyPair = SecurityTestHelpers::GenerateKeyPair();
        auto message = SecurityTestHelpers::GenerateRandomMessage();
        
        auto signature = keyPair->Sign(message);
        REQUIRE(keyPair->GetPublicKey().VerifySignature(message, signature));
        
        // Tamper with message
        auto tamperedMessage = message;
        tamperedMessage[0] ^= 0xFF;
        REQUIRE_FALSE(keyPair->GetPublicKey().VerifySignature(tamperedMessage, signature));
        
        // Tamper with signature
        auto tamperedSignature = signature;
        tamperedSignature[0] ^= 0xFF;
        REQUIRE_FALSE(keyPair->GetPublicKey().VerifySignature(message, tamperedSignature));
    }
}
```

## Test Automation and CI/CD

### GitHub Actions Configuration
```yaml
# .github/workflows/test.yml
name: Comprehensive Testing

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main, develop ]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        compiler: [gcc-11, clang-13]
        build-type: [Debug, Release]
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install -y build-essential cmake ninja-build \
          libssl-dev libleveldb-dev libboost-all-dev \
          valgrind gcovr lcov
    
    - name: Setup vcpkg
      run: |
        git clone https://github.com/Microsoft/vcpkg.git
        cd vcpkg && ./bootstrap-vcpkg.sh
    
    - name: Configure CMake
      run: |
        cmake -B build -G Ninja \
          -DCMAKE_BUILD_TYPE=${{ matrix.build-type }} \
          -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
          -DBUILD_TESTS=ON \
          -DENABLE_COVERAGE=${{ matrix.build-type == 'Debug' }}
    
    - name: Build
      run: cmake --build build --parallel
    
    - name: Run unit tests
      run: |
        cd build
        ctest --output-on-failure --parallel 4 -L "unit"
    
    - name: Memory leak check
      if: matrix.build-type == 'Debug'
      run: |
        cd build
        valgrind --tool=memcheck --leak-check=full \
          --error-exitcode=1 --suppressions=../valgrind.supp \
          ./tests/unit/unit_tests
    
    - name: Generate coverage report
      if: matrix.build-type == 'Debug' && matrix.compiler == 'gcc-11'
      run: |
        cd build
        gcovr --root .. --html --html-details -o coverage.html
        gcovr --root .. --xml -o coverage.xml
    
    - name: Upload coverage
      if: matrix.build-type == 'Debug' && matrix.compiler == 'gcc-11'
      uses: codecov/codecov-action@v3
      with:
        file: build/coverage.xml

  integration-tests:
    runs-on: ubuntu-latest
    needs: unit-tests
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Build and test
      run: |
        # Build commands...
        cmake --build build --target integration_tests
        cd build && ctest -L "integration" --timeout 600
    
    - name: Collect integration test artifacts
      if: failure()
      uses: actions/upload-artifact@v3
      with:
        name: integration-test-logs
        path: build/tests/integration/logs/

  performance-tests:
    runs-on: ubuntu-latest
    needs: unit-tests
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Build and benchmark
      run: |
        # Build commands...
        cmake --build build --target benchmarks
        cd build && ./tests/performance/benchmarks --benchmark-format=json --benchmark-out=benchmark_results.json
    
    - name: Performance regression check
      run: |
        python3 scripts/check_performance_regression.py \
          --current build/benchmark_results.json \
          --baseline performance_baseline.json \
          --threshold 10  # 10% regression threshold

  security-tests:
    runs-on: ubuntu-latest
    needs: unit-tests
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Static analysis
      run: |
        # Install tools
        sudo apt install -y cppcheck clang-tidy
        
        # Run static analysis
        cppcheck --enable=all --error-exitcode=1 src/
        clang-tidy src/**/*.cpp -- -Iinclude
    
    - name: Security scan
      run: |
        cmake --build build --target security_tests
        cd build && ./tests/security/security_tests
```

### Test Reporting and Metrics
```bash
#!/bin/bash
# scripts/generate_test_report.sh

# Run all test suites and collect results
cd build

echo "=== Running Test Suites ===" > test_report.txt
echo "Date: $(date)" >> test_report.txt
echo "Commit: $(git rev-parse HEAD)" >> test_report.txt
echo "" >> test_report.txt

# Unit tests
echo "Unit Tests:" >> test_report.txt
ctest -L "unit" --output-junit unit_test_results.xml
xmllint --format unit_test_results.xml >> test_report.txt

# Integration tests
echo "Integration Tests:" >> test_report.txt
ctest -L "integration" --output-junit integration_test_results.xml
xmllint --format integration_test_results.xml >> test_report.txt

# Performance tests
echo "Performance Tests:" >> test_report.txt
./tests/performance/benchmarks --benchmark-format=json --benchmark-out=benchmark_results.json
python3 ../scripts/format_benchmark_results.py benchmark_results.json >> test_report.txt

# Coverage report
echo "Coverage Report:" >> test_report.txt
gcovr --root .. --txt >> test_report.txt

# Memory usage report
echo "Memory Usage Report:" >> test_report.txt
valgrind --tool=massif --massif-out-file=massif.out ./tests/unit/unit_tests
ms_print massif.out >> test_report.txt

echo "Test report generated: test_report.txt"
```

This comprehensive testing guide ensures thorough validation of the Neo C++ blockchain node across all critical dimensions: functionality, performance, security, and reliability.