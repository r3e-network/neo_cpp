#include <benchmark/benchmark.h>
#include <neo/io/caching/lru_cache.h>
#include <neo/io/caching/ecpoint_cache.h>
#include <neo/io/caching/block_cache.h>
#include <neo/io/caching/transaction_cache.h>
#include <neo/io/caching/contract_cache.h>
#include <neo/io/caching/cache_manager.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/byte_vector.h>
#include <random>
#include <vector>
#include <memory>

using namespace neo::io::caching;
using namespace neo::cryptography::ecc;
using namespace neo::ledger;
using namespace neo::smartcontract;
using namespace neo::io;

// Benchmark LRUCache
static void BM_LRUCache_Add(benchmark::State& state)
{
    // Create cache
    LRUCache<int, int> cache(state.range(0));
    
    // Create random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    
    // Benchmark
    for (auto _ : state)
    {
        int key = dis(gen);
        int value = dis(gen);
        cache.Add(key, value);
    }
}
BENCHMARK(BM_LRUCache_Add)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_LRUCache_Get(benchmark::State& state)
{
    // Create cache
    LRUCache<int, int> cache(state.range(0));
    
    // Create random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    
    // Fill cache
    std::vector<int> keys;
    for (int i = 0; i < state.range(0); ++i)
    {
        int key = dis(gen);
        int value = dis(gen);
        cache.Add(key, value);
        keys.push_back(key);
    }
    
    // Benchmark
    for (auto _ : state)
    {
        int key = keys[dis(gen) % keys.size()];
        int value;
        cache.TryGet(key, value);
    }
}
BENCHMARK(BM_LRUCache_Get)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark ECPointCache
static void BM_ECPointCache_Add(benchmark::State& state)
{
    // Create cache
    ECPointCache cache(state.range(0));
    
    // Create curve
    auto curve = std::make_shared<Secp256r1>();
    
    // Create random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    
    // Benchmark
    for (auto _ : state)
    {
        // Create random ECPoint
        ByteVector privateKey(32);
        for (int i = 0; i < 32; ++i)
            privateKey[i] = static_cast<uint8_t>(dis(gen) % 256);
        
        auto ecpoint = curve->GeneratePublicKey(privateKey.AsSpan());
        
        // Add to cache
        cache.Add(ecpoint);
    }
}
BENCHMARK(BM_ECPointCache_Add)->Arg(100)->Arg(1000);

// Benchmark BlockCache
static void BM_BlockCache_Add(benchmark::State& state)
{
    // Create cache
    BlockCache cache(state.range(0));
    
    // Create random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    
    // Benchmark
    for (auto _ : state)
    {
        // Create random block
        auto block = std::make_shared<Block>();
        block->SetIndex(dis(gen));
        
        // Add to cache
        cache.Add(block);
    }
}
BENCHMARK(BM_BlockCache_Add)->Arg(100)->Arg(1000);

// Benchmark TransactionCache
static void BM_TransactionCache_Add(benchmark::State& state)
{
    // Create cache
    TransactionCache cache(state.range(0));
    
    // Create random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    
    // Benchmark
    for (auto _ : state)
    {
        // Create random transaction
        auto tx = std::make_shared<Transaction>();
        
        // Add to cache
        cache.Add(tx);
    }
}
BENCHMARK(BM_TransactionCache_Add)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark ContractCache
static void BM_ContractCache_Add(benchmark::State& state)
{
    // Create cache
    ContractCache cache(state.range(0));
    
    // Create random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    
    // Benchmark
    for (auto _ : state)
    {
        // Create random contract
        auto contract = std::make_shared<ContractState>();
        
        // Add to cache
        cache.Add(contract);
    }
}
BENCHMARK(BM_ContractCache_Add)->Arg(100)->Arg(1000);

// Benchmark CacheManager
static void BM_CacheManager_GetInstance(benchmark::State& state)
{
    // Benchmark
    for (auto _ : state)
    {
        auto& manager = CacheManager::GetInstance();
        benchmark::DoNotOptimize(manager);
    }
}
BENCHMARK(BM_CacheManager_GetInstance);

BENCHMARK_MAIN();
