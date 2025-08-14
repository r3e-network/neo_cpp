/**
 * @file benchmark_suite.cpp
 * @brief Comprehensive performance benchmark suite for Neo C++
 */

#include <benchmark/benchmark.h>
#include <neo/core/neo_system.h>
#include <neo/network/connection_pool.h>
#include <neo/ledger/blockchain_cache.h>
#include <neo/ledger/memory_pool.h>
#include <neo/cryptography/crypto.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/network/p2p_server.h>
#include <neo/consensus/consensus_service.h>
#include <neo/monitoring/performance_monitor.h>
#include <neo/profiling/continuous_profiler.h>

#include <random>
#include <vector>
#include <memory>

using namespace neo;

// ============================================================================
// Cryptography Benchmarks
// ============================================================================

static void BM_SHA256(benchmark::State& state) {
    std::vector<uint8_t> data(state.range(0));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = dis(gen);
    }
    
    for (auto _ : state) {
        auto hash = cryptography::Sha256(io::ByteSpan(data.data(), data.size()));
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SHA256)->Range(32, 1<<20);  // 32 bytes to 1MB

static void BM_RIPEMD160(benchmark::State& state) {
    std::vector<uint8_t> data(state.range(0));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = dis(gen);
    }
    
    for (auto _ : state) {
        auto hash = cryptography::RipeMD160(io::ByteSpan(data.data(), data.size()));
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_RIPEMD160)->Range(32, 1<<16);

static void BM_ECDSASign(benchmark::State& state) {
    auto private_key = cryptography::Crypto::GenerateRandomBytes(32);
    std::vector<uint8_t> message(32);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : message) {
        byte = dis(gen);
    }
    
    for (auto _ : state) {
        auto signature = cryptography::Crypto::Sign(
            io::ByteSpan(message.data(), message.size()),
            private_key.AsSpan()
        );
        benchmark::DoNotOptimize(signature);
    }
}
BENCHMARK(BM_ECDSASign);

static void BM_ECDSAVerify(benchmark::State& state) {
    auto private_key = cryptography::Crypto::GenerateRandomBytes(32);
    auto public_key = cryptography::Crypto::ComputePublicKey(private_key.AsSpan());
    
    std::vector<uint8_t> message(32);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : message) {
        byte = dis(gen);
    }
    
    auto signature = cryptography::Crypto::Sign(
        io::ByteSpan(message.data(), message.size()),
        private_key.AsSpan()
    );
    
    for (auto _ : state) {
        bool valid = cryptography::Crypto::VerifySignature(
            io::ByteSpan(message.data(), message.size()),
            signature.AsSpan(),
            public_key.ToArray().AsSpan()
        );
        benchmark::DoNotOptimize(valid);
    }
}
BENCHMARK(BM_ECDSAVerify);

// ============================================================================
// Blockchain Cache Benchmarks
// ============================================================================

static void BM_BlockchainCacheInsert(benchmark::State& state) {
    ledger::BlockchainCache::Config config;
    config.block_cache_size = state.range(0);
    ledger::BlockchainCache cache(config);
    
    std::vector<std::shared_ptr<ledger::Block>> blocks;
    for (int i = 0; i < state.range(0) * 2; ++i) {
        auto block = std::make_shared<ledger::Block>();
        block->SetIndex(i);
        blocks.push_back(block);
    }
    
    size_t index = 0;
    for (auto _ : state) {
        cache.CacheBlock(blocks[index % blocks.size()]);
        index++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BlockchainCacheInsert)->Range(100, 10000);

static void BM_BlockchainCacheLookup(benchmark::State& state) {
    ledger::BlockchainCache::Config config;
    config.block_cache_size = state.range(0);
    ledger::BlockchainCache cache(config);
    
    // Pre-populate cache
    for (int i = 0; i < state.range(0); ++i) {
        auto block = std::make_shared<ledger::Block>();
        block->SetIndex(i);
        cache.CacheBlock(block);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, state.range(0) - 1);
    
    for (auto _ : state) {
        auto block = cache.GetBlock(dis(gen));
        benchmark::DoNotOptimize(block);
    }
    
    auto stats = cache.GetStats();
    state.counters["hit_rate"] = stats.hit_rate;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BlockchainCacheLookup)->Range(100, 10000);

// ============================================================================
// Memory Pool Benchmarks
// ============================================================================

static void BM_MemoryPoolAdd(benchmark::State& state) {
    ledger::MemoryPool pool(state.range(0), 1024);
    
    std::vector<ledger::Transaction> transactions;
    for (int i = 0; i < state.range(0) * 2; ++i) {
        ledger::Transaction tx;
        tx.SetNonce(i);
        tx.SetSystemFee(100);
        tx.SetNetworkFee(10);
        transactions.push_back(tx);
    }
    
    size_t index = 0;
    for (auto _ : state) {
        pool.TryAdd(transactions[index % transactions.size()]);
        index++;
        
        // Clear pool when full
        if (pool.GetCount() >= state.range(0)) {
            pool.Clear();
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemoryPoolAdd)->Range(100, 10000);

static void BM_MemoryPoolGetSorted(benchmark::State& state) {
    ledger::MemoryPool pool(state.range(0), 1024);
    
    // Pre-populate pool
    for (int i = 0; i < state.range(0); ++i) {
        ledger::Transaction tx;
        tx.SetNonce(i);
        tx.SetSystemFee(100 + i);
        tx.SetNetworkFee(10);
        pool.TryAdd(tx);
    }
    
    for (auto _ : state) {
        auto sorted = pool.GetSortedTransactions();
        benchmark::DoNotOptimize(sorted);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MemoryPoolGetSorted)->Range(10, 1000);

// ============================================================================
// Connection Pool Benchmarks
// ============================================================================

static void BM_ConnectionPoolGetReturn(benchmark::State& state) {
    network::ConnectionPool::Config config;
    config.max_connections = state.range(0);
    config.min_connections = state.range(0) / 2;
    network::ConnectionPool pool(config);
    
    // Mock connection factory
    pool.SetConnectionFactory([](const std::string&, uint16_t) {
        return std::make_shared<network::TcpConnection>();
    });
    
    pool.Start();
    
    for (auto _ : state) {
        auto conn = pool.GetConnection("localhost", 8080);
        benchmark::DoNotOptimize(conn);
        pool.ReturnConnection(conn);
    }
    
    pool.Stop();
    
    auto stats = pool.GetStats();
    state.counters["reused"] = stats.reused_connections;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConnectionPoolGetReturn)->Range(10, 100);

// ============================================================================
// Smart Contract VM Benchmarks
// ============================================================================

static void BM_VMSimpleOperation(benchmark::State& state) {
    for (auto _ : state) {
        vm::Script script;
        // Simple ADD operation
        script.EmitPush(1);
        script.EmitPush(2);
        script.EmitSysCall("System.Math.Add");
        
        smartcontract::ApplicationEngine engine(
            smartcontract::TriggerType::Application,
            nullptr,
            nullptr,
            nullptr,
            1000000  // Gas
        );
        
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VMSimpleOperation);

static void BM_VMComplexOperation(benchmark::State& state) {
    for (auto _ : state) {
        vm::Script script;
        
        // Complex operation with loops
        for (int i = 0; i < state.range(0); ++i) {
            script.EmitPush(i);
        }
        
        for (int i = 0; i < state.range(0) - 1; ++i) {
            script.EmitSysCall("System.Math.Add");
        }
        
        smartcontract::ApplicationEngine engine(
            smartcontract::TriggerType::Application,
            nullptr,
            nullptr,
            nullptr,
            10000000  // Gas
        );
        
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_VMComplexOperation)->Range(10, 100);

// ============================================================================
// Serialization Benchmarks
// ============================================================================

static void BM_TransactionSerialize(benchmark::State& state) {
    ledger::Transaction tx;
    tx.SetNonce(12345);
    tx.SetSystemFee(1000000);
    tx.SetNetworkFee(100000);
    tx.SetValidUntilBlock(1000000);
    
    // Add attributes and witnesses
    for (int i = 0; i < state.range(0); ++i) {
        ledger::TransactionAttribute attr;
        tx.AddAttribute(attr);
    }
    
    for (auto _ : state) {
        auto serialized = tx.ToByteArray();
        benchmark::DoNotOptimize(serialized);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TransactionSerialize)->Range(0, 100);

static void BM_TransactionDeserialize(benchmark::State& state) {
    ledger::Transaction tx;
    tx.SetNonce(12345);
    tx.SetSystemFee(1000000);
    tx.SetNetworkFee(100000);
    tx.SetValidUntilBlock(1000000);
    
    for (int i = 0; i < state.range(0); ++i) {
        ledger::TransactionAttribute attr;
        tx.AddAttribute(attr);
    }
    
    auto serialized = tx.ToByteArray();
    
    for (auto _ : state) {
        ledger::Transaction deserialized;
        deserialized.Deserialize(io::ByteSpan(serialized.data(), serialized.size()));
        benchmark::DoNotOptimize(deserialized);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TransactionDeserialize)->Range(0, 100);

// ============================================================================
// Consensus Benchmarks
// ============================================================================

static void BM_ConsensusMessageProcessing(benchmark::State& state) {
    // This would require a more complex setup with actual consensus service
    // Placeholder for now
    
    for (auto _ : state) {
        consensus::ConsensusMessage msg;
        msg.SetViewNumber(1);
        msg.SetType(consensus::MessageType::PrepareRequest);
        
        // Process message (simplified)
        auto processed = msg.Verify();
        benchmark::DoNotOptimize(processed);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConsensusMessageProcessing);

// ============================================================================
// Performance Monitor Benchmarks
// ============================================================================

static void BM_PerformanceMonitorRecord(benchmark::State& state) {
    monitoring::PerformanceMonitor monitor;
    monitor.Start();
    
    for (auto _ : state) {
        monitor.RecordOperation("test_operation", 1.23);
    }
    
    monitor.Stop();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PerformanceMonitorRecord);

// ============================================================================
// Profiling Overhead Benchmark
// ============================================================================

static void BM_ProfilingOverhead(benchmark::State& state) {
    profiling::ContinuousProfiler::Config config;
    config.sampling_interval = std::chrono::milliseconds(1);
    profiling::ContinuousProfiler profiler(config);
    
    if (state.range(0) > 0) {
        profiler.Start();
    }
    
    // Simple workload
    std::vector<int> data(1000);
    std::iota(data.begin(), data.end(), 0);
    
    for (auto _ : state) {
        PROFILE_FUNCTION(profiler);
        
        // Do some work
        int sum = 0;
        for (int val : data) {
            sum += val;
        }
        benchmark::DoNotOptimize(sum);
    }
    
    if (state.range(0) > 0) {
        profiler.Stop();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProfilingOverhead)->Arg(0)->Arg(1);  // 0=no profiling, 1=with profiling

// ============================================================================
// Main function
// ============================================================================

BENCHMARK_MAIN();