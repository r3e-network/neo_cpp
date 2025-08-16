/**
 * @file benchmark_blockchain.cpp
 * @brief Blockchain and ledger performance benchmarks
 */

#include <benchmark/benchmark.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/mempool.h>
#include <neo/persistence/store.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/key_pair.h>
#include <neo/io/byte_vector.h>
#include <random>
#include <vector>
#include <memory>

using namespace neo::ledger;
using namespace neo::persistence;
using namespace neo::cryptography;
using namespace neo::io;

// ============================================================================
// Helper Functions
// ============================================================================

static Transaction CreateRandomTransaction() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 1000000);
    
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = dis(gen);
    tx.SystemFee = dis(gen);
    tx.NetworkFee = dis(gen);
    tx.ValidUntilBlock = dis(gen) + 1000;
    
    // Add random attributes
    for (int i = 0; i < 3; ++i) {
        TransactionAttribute attr;
        attr.Type = static_cast<TransactionAttributeType>(i);
        attr.Data = ByteVector(32, i);
        tx.Attributes.push_back(attr);
    }
    
    // Add witness
    Witness witness;
    witness.InvocationScript = ByteVector(64, 0xFF);
    witness.VerificationScript = ByteVector(32, 0xAA);
    tx.Witnesses.push_back(witness);
    
    return tx;
}

static Block CreateRandomBlock(uint32_t index) {
    Block block;
    block.Version = 0;
    block.PrevHash = ByteVector(32, index - 1);
    block.MerkleRoot = ByteVector(32, 0xCC);
    block.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    block.Index = index;
    block.NextConsensus = ByteVector(20, 0xDD);
    
    // Add some transactions
    for (int i = 0; i < 10; ++i) {
        block.Transactions.push_back(CreateRandomTransaction());
    }
    
    // Add witness
    Witness witness;
    witness.InvocationScript = ByteVector(64, 0xFF);
    witness.VerificationScript = ByteVector(32, 0xAA);
    block.Witness = witness;
    
    return block;
}

// ============================================================================
// Transaction Benchmarks
// ============================================================================

static void BM_Transaction_Create(benchmark::State& state) {
    for (auto _ : state) {
        auto tx = CreateRandomTransaction();
        benchmark::DoNotOptimize(tx);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Transaction_Create);

static void BM_Transaction_Serialize(benchmark::State& state) {
    auto tx = CreateRandomTransaction();
    
    for (auto _ : state) {
        auto serialized = tx.Serialize();
        benchmark::DoNotOptimize(serialized);
    }
    
    state.SetBytesProcessed(state.iterations() * tx.Size());
}
BENCHMARK(BM_Transaction_Serialize);

static void BM_Transaction_Deserialize(benchmark::State& state) {
    auto tx = CreateRandomTransaction();
    auto serialized = tx.Serialize();
    
    for (auto _ : state) {
        Transaction deserialized;
        deserialized.Deserialize(serialized);
        benchmark::DoNotOptimize(deserialized);
    }
    
    state.SetBytesProcessed(state.iterations() * serialized.Size());
}
BENCHMARK(BM_Transaction_Deserialize);

static void BM_Transaction_Hash(benchmark::State& state) {
    auto tx = CreateRandomTransaction();
    
    for (auto _ : state) {
        auto hash = tx.GetHash();
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Transaction_Hash);

static void BM_Transaction_Verify(benchmark::State& state) {
    auto tx = CreateRandomTransaction();
    Blockchain blockchain;
    
    for (auto _ : state) {
        bool valid = tx.Verify(blockchain.GetSnapshot());
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Transaction_Verify);

// ============================================================================
// Block Benchmarks
// ============================================================================

static void BM_Block_Create(benchmark::State& state) {
    uint32_t index = 1;
    
    for (auto _ : state) {
        auto block = CreateRandomBlock(index++);
        benchmark::DoNotOptimize(block);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Block_Create);

static void BM_Block_Serialize(benchmark::State& state) {
    auto block = CreateRandomBlock(1000);
    
    for (auto _ : state) {
        auto serialized = block.Serialize();
        benchmark::DoNotOptimize(serialized);
    }
    
    state.SetBytesProcessed(state.iterations() * block.Size());
}
BENCHMARK(BM_Block_Serialize);

static void BM_Block_Deserialize(benchmark::State& state) {
    auto block = CreateRandomBlock(1000);
    auto serialized = block.Serialize();
    
    for (auto _ : state) {
        Block deserialized;
        deserialized.Deserialize(serialized);
        benchmark::DoNotOptimize(deserialized);
    }
    
    state.SetBytesProcessed(state.iterations() * serialized.Size());
}
BENCHMARK(BM_Block_Deserialize);

static void BM_Block_CalculateMerkleRoot(benchmark::State& state) {
    auto block = CreateRandomBlock(1000);
    
    for (auto _ : state) {
        auto merkle = block.CalculateMerkleRoot();
        benchmark::DoNotOptimize(merkle);
    }
    
    state.SetItemsProcessed(state.iterations() * block.Transactions.size());
}
BENCHMARK(BM_Block_CalculateMerkleRoot);

static void BM_Block_Verify(benchmark::State& state) {
    auto block = CreateRandomBlock(1000);
    Blockchain blockchain;
    
    for (auto _ : state) {
        bool valid = block.Verify(blockchain.GetSnapshot());
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Block_Verify);

// ============================================================================
// Blockchain Benchmarks
// ============================================================================

static void BM_Blockchain_AddBlock(benchmark::State& state) {
    Blockchain blockchain;
    uint32_t index = 1;
    
    for (auto _ : state) {
        auto block = CreateRandomBlock(index++);
        blockchain.AddBlock(block);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Blockchain_AddBlock);

static void BM_Blockchain_GetBlock(benchmark::State& state) {
    Blockchain blockchain;
    
    // Add some blocks
    for (uint32_t i = 1; i <= 100; ++i) {
        blockchain.AddBlock(CreateRandomBlock(i));
    }
    
    for (auto _ : state) {
        auto block = blockchain.GetBlock(50);
        benchmark::DoNotOptimize(block);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Blockchain_GetBlock);

static void BM_Blockchain_GetTransaction(benchmark::State& state) {
    Blockchain blockchain;
    
    // Add blocks with transactions
    std::vector<ByteVector> txHashes;
    for (uint32_t i = 1; i <= 10; ++i) {
        auto block = CreateRandomBlock(i);
        for (const auto& tx : block.Transactions) {
            txHashes.push_back(tx.GetHash());
        }
        blockchain.AddBlock(block);
    }
    
    for (auto _ : state) {
        auto tx = blockchain.GetTransaction(txHashes[50]);
        benchmark::DoNotOptimize(tx);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Blockchain_GetTransaction);

static void BM_Blockchain_ContainsTransaction(benchmark::State& state) {
    Blockchain blockchain;
    
    // Add blocks with transactions
    std::vector<ByteVector> txHashes;
    for (uint32_t i = 1; i <= 10; ++i) {
        auto block = CreateRandomBlock(i);
        for (const auto& tx : block.Transactions) {
            txHashes.push_back(tx.GetHash());
        }
        blockchain.AddBlock(block);
    }
    
    for (auto _ : state) {
        bool contains = blockchain.ContainsTransaction(txHashes[50]);
        benchmark::DoNotOptimize(contains);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Blockchain_ContainsTransaction);

// ============================================================================
// Mempool Benchmarks
// ============================================================================

static void BM_Mempool_Add(benchmark::State& state) {
    MemPool mempool;
    
    for (auto _ : state) {
        auto tx = CreateRandomTransaction();
        mempool.Add(tx);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Mempool_Add);

static void BM_Mempool_Remove(benchmark::State& state) {
    MemPool mempool;
    std::vector<ByteVector> hashes;
    
    // Add transactions
    for (int i = 0; i < 1000; ++i) {
        auto tx = CreateRandomTransaction();
        hashes.push_back(tx.GetHash());
        mempool.Add(tx);
    }
    
    size_t index = 0;
    for (auto _ : state) {
        mempool.Remove(hashes[index % hashes.size()]);
        index++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Mempool_Remove);

static void BM_Mempool_Contains(benchmark::State& state) {
    MemPool mempool;
    std::vector<ByteVector> hashes;
    
    // Add transactions
    for (int i = 0; i < 1000; ++i) {
        auto tx = CreateRandomTransaction();
        hashes.push_back(tx.GetHash());
        mempool.Add(tx);
    }
    
    for (auto _ : state) {
        bool contains = mempool.Contains(hashes[500]);
        benchmark::DoNotOptimize(contains);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Mempool_Contains);

static void BM_Mempool_GetSortedTransactions(benchmark::State& state) {
    MemPool mempool;
    
    // Add transactions
    for (int i = 0; i < 100; ++i) {
        mempool.Add(CreateRandomTransaction());
    }
    
    for (auto _ : state) {
        auto sorted = mempool.GetSortedTransactions();
        benchmark::DoNotOptimize(sorted);
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_Mempool_GetSortedTransactions);

// ============================================================================
// Persistence/Storage Benchmarks
// ============================================================================

static void BM_Storage_Put(benchmark::State& state) {
    Store store;
    store.Open("benchmark_db", true); // In-memory for benchmarking
    
    std::vector<uint8_t> key_data(32);
    std::vector<uint8_t> value_data(256);
    
    for (auto _ : state) {
        // Generate random key
        for (auto& b : key_data) b = rand() % 256;
        for (auto& b : value_data) b = rand() % 256;
        
        store.Put(ByteVector(key_data), ByteVector(value_data));
    }
    
    state.SetBytesProcessed(state.iterations() * (32 + 256));
    store.Close();
}
BENCHMARK(BM_Storage_Put);

static void BM_Storage_Get(benchmark::State& state) {
    Store store;
    store.Open("benchmark_db", true); // In-memory for benchmarking
    
    // Pre-populate store
    std::vector<ByteVector> keys;
    for (int i = 0; i < 1000; ++i) {
        std::vector<uint8_t> key_data(32, i % 256);
        std::vector<uint8_t> value_data(256, i % 256);
        ByteVector key(key_data);
        keys.push_back(key);
        store.Put(key, ByteVector(value_data));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto value = store.Get(keys[index % keys.size()]);
        benchmark::DoNotOptimize(value);
        index++;
    }
    
    state.SetItemsProcessed(state.iterations());
    store.Close();
}
BENCHMARK(BM_Storage_Get);

static void BM_Storage_Delete(benchmark::State& state) {
    Store store;
    store.Open("benchmark_db", true); // In-memory for benchmarking
    
    // Pre-populate store
    std::vector<ByteVector> keys;
    for (int i = 0; i < 10000; ++i) {
        std::vector<uint8_t> key_data(32, i % 256);
        std::vector<uint8_t> value_data(256, i % 256);
        ByteVector key(key_data);
        keys.push_back(key);
        store.Put(key, ByteVector(value_data));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        store.Delete(keys[index % keys.size()]);
        index++;
    }
    
    state.SetItemsProcessed(state.iterations());
    store.Close();
}
BENCHMARK(BM_Storage_Delete);

static void BM_Storage_BatchWrite(benchmark::State& state) {
    Store store;
    store.Open("benchmark_db", true); // In-memory for benchmarking
    
    for (auto _ : state) {
        WriteBatch batch;
        
        // Add 100 operations to batch
        for (int i = 0; i < 100; ++i) {
            std::vector<uint8_t> key_data(32, i);
            std::vector<uint8_t> value_data(256, i);
            batch.Put(ByteVector(key_data), ByteVector(value_data));
        }
        
        store.Write(batch);
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    store.Close();
}
BENCHMARK(BM_Storage_BatchWrite);

// ============================================================================
// Consensus-Related Benchmarks
// ============================================================================

static void BM_Consensus_BlockValidation(benchmark::State& state) {
    Blockchain blockchain;
    
    // Add genesis block
    blockchain.AddBlock(CreateRandomBlock(0));
    
    for (auto _ : state) {
        auto block = CreateRandomBlock(blockchain.GetHeight() + 1);
        bool valid = block.Verify(blockchain.GetSnapshot());
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Consensus_BlockValidation);

static void BM_Consensus_TransactionValidation(benchmark::State& state) {
    Blockchain blockchain;
    
    for (auto _ : state) {
        auto tx = CreateRandomTransaction();
        bool valid = tx.Verify(blockchain.GetSnapshot());
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Consensus_TransactionValidation);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();