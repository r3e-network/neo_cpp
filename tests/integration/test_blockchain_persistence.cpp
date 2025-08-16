#include <gtest/gtest.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/snapshot.h>
#include <neo/cryptography/crypto.h>
#include <neo/wallets/key_pair.h>
#include <memory>
#include <thread>
#include <chrono>

using namespace neo::ledger;
using namespace neo::persistence;
using namespace neo::cryptography;
using namespace neo::wallets;

class BlockchainPersistenceIntegrationTest : public ::testing::Test
{
protected:
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<MemoryStore> store;
    std::unique_ptr<Snapshot> snapshot;
    
    void SetUp() override
    {
        store = std::make_unique<MemoryStore>();
        snapshot = std::make_unique<Snapshot>(store.get());
        blockchain = std::make_unique<Blockchain>(snapshot.get());
        blockchain->Initialize();
    }
    
    void TearDown() override
    {
        blockchain.reset();
        snapshot.reset();
        store.reset();
    }
    
    Block CreateTestBlock(uint32_t index, const UInt256& prevHash)
    {
        Block block;
        block.Version = 0;
        block.PrevHash = prevHash;
        block.Timestamp = std::time(nullptr);
        block.Index = index;
        block.NextConsensus = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
        
        // Add some test transactions
        for (int i = 0; i < 3; ++i)
        {
            Transaction tx;
            tx.Version = 0;
            tx.Nonce = std::rand();
            tx.SystemFee = 1000000;
            tx.NetworkFee = 1000000;
            tx.ValidUntilBlock = index + 100;
            block.Transactions.push_back(tx);
        }
        
        // Calculate merkle root
        block.CalculateMerkleRoot();
        
        return block;
    }
};

// Test 1: Full Block Cycle - Create, Add, Persist, Retrieve
TEST_F(BlockchainPersistenceIntegrationTest, FullBlockCycle)
{
    // Get genesis block
    auto genesis = blockchain->GetBlock(0);
    ASSERT_NE(genesis, nullptr);
    EXPECT_EQ(genesis->Index, 0);
    
    // Create and add new block
    auto block1 = CreateTestBlock(1, genesis->Hash());
    EXPECT_TRUE(blockchain->AddBlock(block1));
    
    // Verify block was added
    EXPECT_EQ(blockchain->GetBlockCount(), 2);
    
    // Persist to storage
    snapshot->Commit();
    
    // Create new snapshot and verify persistence
    auto newSnapshot = std::make_unique<Snapshot>(store.get());
    auto retrievedBlock = blockchain->GetBlock(1);
    
    ASSERT_NE(retrievedBlock, nullptr);
    EXPECT_EQ(retrievedBlock->Index, 1);
    EXPECT_EQ(retrievedBlock->PrevHash, genesis->Hash());
    EXPECT_EQ(retrievedBlock->Transactions.size(), 3);
}

// Test 2: Transaction Pool Integration
TEST_F(BlockchainPersistenceIntegrationTest, TransactionPoolIntegration)
{
    auto mempool = blockchain->GetMemPool();
    ASSERT_NE(mempool, nullptr);
    
    // Create test transaction
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = std::rand();
    tx.SystemFee = 1000000;
    tx.NetworkFee = 1000000;
    tx.ValidUntilBlock = blockchain->GetBlockCount() + 100;
    
    // Add to mempool
    EXPECT_TRUE(mempool->TryAdd(tx));
    
    // Verify in mempool
    EXPECT_TRUE(mempool->Contains(tx.Hash()));
    
    // Create block with mempool transactions
    auto block = CreateTestBlock(1, blockchain->GetCurrentBlockHash());
    block.Transactions.push_back(tx);
    
    // Add block
    EXPECT_TRUE(blockchain->AddBlock(block));
    
    // Verify transaction removed from mempool
    EXPECT_FALSE(mempool->Contains(tx.Hash()));
    
    // Verify transaction in blockchain
    auto retrievedTx = blockchain->GetTransaction(tx.Hash());
    ASSERT_NE(retrievedTx, nullptr);
    EXPECT_EQ(retrievedTx->Hash(), tx.Hash());
}

// Test 3: State Management Integration
TEST_F(BlockchainPersistenceIntegrationTest, StateManagementIntegration)
{
    // Create account state
    AccountState account;
    account.ScriptHash = UInt160::Parse("0xabcdef1234567890abcdef1234567890abcdef12");
    account.Balance = 1000000000;
    
    // Store in snapshot
    snapshot->Accounts.Add(account.ScriptHash, account);
    
    // Create contract state
    ContractState contract;
    contract.Id = 1;
    contract.Hash = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    contract.Script = ByteVector::Parse("0102030405");
    
    snapshot->Contracts.Add(contract.Hash, contract);
    
    // Commit changes
    snapshot->Commit();
    
    // Create new snapshot and verify
    auto newSnapshot = std::make_unique<Snapshot>(store.get());
    
    // Retrieve account
    auto retrievedAccount = newSnapshot->Accounts.TryGet(account.ScriptHash);
    ASSERT_NE(retrievedAccount, nullptr);
    EXPECT_EQ(retrievedAccount->Balance, account.Balance);
    
    // Retrieve contract
    auto retrievedContract = newSnapshot->Contracts.TryGet(contract.Hash);
    ASSERT_NE(retrievedContract, nullptr);
    EXPECT_EQ(retrievedContract->Id, contract.Id);
    EXPECT_EQ(retrievedContract->Script, contract.Script);
}

// Test 4: Concurrent Access Test
TEST_F(BlockchainPersistenceIntegrationTest, ConcurrentAccess)
{
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    const int numThreads = 10;
    const int operationsPerThread = 100;
    
    // Launch threads for concurrent reads
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([this, &successCount, operationsPerThread]() {
            for (int j = 0; j < operationsPerThread; ++j)
            {
                auto block = blockchain->GetBlock(0);
                if (block != nullptr && block->Index == 0)
                {
                    successCount++;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads)
    {
        thread.join();
    }
    
    // Verify all reads succeeded
    EXPECT_EQ(successCount, numThreads * operationsPerThread);
}

// Test 5: Rollback and Recovery
TEST_F(BlockchainPersistenceIntegrationTest, RollbackAndRecovery)
{
    // Add multiple blocks
    auto genesis = blockchain->GetBlock(0);
    std::vector<Block> blocks;
    
    for (uint32_t i = 1; i <= 5; ++i)
    {
        auto prevHash = (i == 1) ? genesis->Hash() : blocks[i-2].Hash();
        auto block = CreateTestBlock(i, prevHash);
        blocks.push_back(block);
        EXPECT_TRUE(blockchain->AddBlock(block));
    }
    
    EXPECT_EQ(blockchain->GetBlockCount(), 6);
    
    // Create savepoint
    snapshot->Commit();
    auto savepoint = snapshot->CreateSavepoint();
    
    // Add more blocks
    for (uint32_t i = 6; i <= 8; ++i)
    {
        auto block = CreateTestBlock(i, blocks[i-2].Hash());
        blocks.push_back(block);
        EXPECT_TRUE(blockchain->AddBlock(block));
    }
    
    EXPECT_EQ(blockchain->GetBlockCount(), 9);
    
    // Rollback to savepoint
    snapshot->Rollback(savepoint);
    
    // Verify rollback
    EXPECT_EQ(blockchain->GetBlockCount(), 6);
    EXPECT_EQ(blockchain->GetBlock(5)->Index, 5);
    EXPECT_EQ(blockchain->GetBlock(6), nullptr);
}

// Test 6: Performance Under Load
TEST_F(BlockchainPersistenceIntegrationTest, PerformanceUnderLoad)
{
    const int numBlocks = 100;
    const int txPerBlock = 50;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Add many blocks with transactions
    auto prevHash = blockchain->GetBlock(0)->Hash();
    for (int i = 1; i <= numBlocks; ++i)
    {
        Block block;
        block.Version = 0;
        block.PrevHash = prevHash;
        block.Timestamp = std::time(nullptr);
        block.Index = i;
        block.NextConsensus = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
        
        // Add transactions
        for (int j = 0; j < txPerBlock; ++j)
        {
            Transaction tx;
            tx.Version = 0;
            tx.Nonce = i * 1000 + j;
            tx.SystemFee = 1000000;
            tx.NetworkFee = 1000000;
            tx.ValidUntilBlock = i + 100;
            block.Transactions.push_back(tx);
        }
        
        block.CalculateMerkleRoot();
        EXPECT_TRUE(blockchain->AddBlock(block));
        prevHash = block.Hash();
        
        // Commit every 10 blocks
        if (i % 10 == 0)
        {
            snapshot->Commit();
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify performance
    EXPECT_EQ(blockchain->GetBlockCount(), numBlocks + 1);
    
    // Should complete in reasonable time (< 5 seconds for 100 blocks)
    EXPECT_LT(duration.count(), 5000);
    
    // Calculate throughput
    double blocksPerSecond = (numBlocks * 1000.0) / duration.count();
    double txPerSecond = (numBlocks * txPerBlock * 1000.0) / duration.count();
    
    std::cout << "Performance: " << blocksPerSecond << " blocks/sec, " 
              << txPerSecond << " tx/sec" << std::endl;
}

// Test 7: Data Integrity Verification
TEST_F(BlockchainPersistenceIntegrationTest, DataIntegrityVerification)
{
    // Add blocks with specific data
    std::map<uint32_t, UInt256> blockHashes;
    std::map<UInt256, Transaction> transactions;
    
    auto prevHash = blockchain->GetBlock(0)->Hash();
    for (uint32_t i = 1; i <= 10; ++i)
    {
        auto block = CreateTestBlock(i, prevHash);
        
        // Store hashes for verification
        blockHashes[i] = block.Hash();
        for (const auto& tx : block.Transactions)
        {
            transactions[tx.Hash()] = tx;
        }
        
        EXPECT_TRUE(blockchain->AddBlock(block));
        prevHash = block.Hash();
    }
    
    // Commit all changes
    snapshot->Commit();
    
    // Create new blockchain instance and verify data
    auto newSnapshot = std::make_unique<Snapshot>(store.get());
    auto newBlockchain = std::make_unique<Blockchain>(newSnapshot.get());
    
    // Verify all blocks
    for (const auto& [index, hash] : blockHashes)
    {
        auto block = newBlockchain->GetBlock(index);
        ASSERT_NE(block, nullptr);
        EXPECT_EQ(block->Hash(), hash);
        EXPECT_EQ(block->Index, index);
    }
    
    // Verify all transactions
    for (const auto& [hash, tx] : transactions)
    {
        auto retrievedTx = newBlockchain->GetTransaction(hash);
        ASSERT_NE(retrievedTx, nullptr);
        EXPECT_EQ(retrievedTx->Hash(), hash);
        EXPECT_EQ(retrievedTx->SystemFee, tx.SystemFee);
        EXPECT_EQ(retrievedTx->NetworkFee, tx.NetworkFee);
    }
}

// Test 8: Error Recovery and Resilience
TEST_F(BlockchainPersistenceIntegrationTest, ErrorRecoveryAndResilience)
{
    // Add valid block
    auto block1 = CreateTestBlock(1, blockchain->GetBlock(0)->Hash());
    EXPECT_TRUE(blockchain->AddBlock(block1));
    
    // Try to add invalid block (wrong index)
    auto invalidBlock1 = CreateTestBlock(3, block1.Hash());
    EXPECT_FALSE(blockchain->AddBlock(invalidBlock1));
    
    // Try to add invalid block (wrong prev hash)
    auto invalidBlock2 = CreateTestBlock(2, UInt256::Zero());
    EXPECT_FALSE(blockchain->AddBlock(invalidBlock2));
    
    // Verify blockchain state is still valid
    EXPECT_EQ(blockchain->GetBlockCount(), 2);
    EXPECT_NE(blockchain->GetBlock(1), nullptr);
    
    // Add valid block after errors
    auto block2 = CreateTestBlock(2, block1.Hash());
    EXPECT_TRUE(blockchain->AddBlock(block2));
    
    // Verify recovery
    EXPECT_EQ(blockchain->GetBlockCount(), 3);
    EXPECT_NE(blockchain->GetBlock(2), nullptr);
}

// Test 9: Memory Management
TEST_F(BlockchainPersistenceIntegrationTest, MemoryManagement)
{
    // Monitor memory usage during operations
    size_t initialMemory = store->GetMemoryUsage();
    
    // Add many blocks
    auto prevHash = blockchain->GetBlock(0)->Hash();
    for (int i = 1; i <= 50; ++i)
    {
        auto block = CreateTestBlock(i, prevHash);
        EXPECT_TRUE(blockchain->AddBlock(block));
        prevHash = block.Hash();
    }
    
    size_t afterBlocksMemory = store->GetMemoryUsage();
    
    // Clear cache
    snapshot->ClearCache();
    size_t afterClearMemory = store->GetMemoryUsage();
    
    // Verify memory is managed properly
    EXPECT_GT(afterBlocksMemory, initialMemory);
    EXPECT_LE(afterClearMemory, afterBlocksMemory);
    
    // Verify data still accessible after cache clear
    for (int i = 0; i <= 50; ++i)
    {
        EXPECT_NE(blockchain->GetBlock(i), nullptr);
    }
}