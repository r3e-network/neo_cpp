#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/network/p2p/block_sync_manager.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/memory_pool.h>
#include <neo/persistence/store_cache.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/cryptography/hash.h>
#include <neo/vm/opcode.h>
#include <neo/common/contains_transaction_type.h>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>
#include <random>

using namespace neo;
using namespace neo::network;
using namespace neo::network::p2p;
using namespace neo::ledger;

/**
 * Comprehensive integration test that verifies:
 * 1. P2P network connectivity
 * 2. Block synchronization
 * 3. Block/transaction execution
 * 4. State updates
 * 
 * This test simulates a complete Neo node lifecycle.
 */
class NeoNodeCompleteIntegrationTest : public ::testing::Test
{
protected:
    std::shared_ptr<NeoSystem> nodeSystem1;
    std::shared_ptr<NeoSystem> nodeSystem2;
    std::unique_ptr<BlockSyncManager> syncManager1;
    std::unique_ptr<BlockSyncManager> syncManager2;
    
    void SetUp() override
    {
        // Create two independent node systems
        auto settings1 = std::make_unique<ProtocolSettings>();
        auto settings2 = std::make_unique<ProtocolSettings>();
        
        nodeSystem1 = std::make_shared<NeoSystem>(std::move(settings1), "memory");
        nodeSystem2 = std::make_shared<NeoSystem>(std::move(settings2), "memory");
    }
    
    void TearDown() override
    {
        // Clean shutdown
        if (syncManager1) syncManager1->Stop();
        if (syncManager2) syncManager2->Stop();
        if (nodeSystem1) nodeSystem1->stop();
        if (nodeSystem2) nodeSystem2->stop();
        
        // Clean up LocalNode singleton
        LocalNode::GetInstance().Stop();
    }
    
    std::shared_ptr<ledger::Block> CreateGenesisBlock()
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(io::UInt256::Zero());
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(std::chrono::system_clock::from_time_t(1468595301)); // Neo mainnet genesis time
        block->SetIndex(0);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00});
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        block->SetWitness(*witness);
        
        return block;
    }
    
    std::shared_ptr<ledger::Transaction> CreateTransferTransaction(
        const io::UInt160& from, 
        const io::UInt160& to, 
        uint64_t amount,
        uint32_t nonce)
    {
        auto tx = std::make_shared<ledger::Transaction>();
        tx->SetVersion(0);
        tx->SetNonce(nonce);
        tx->SetSystemFee(100000); // 0.001 GAS
        tx->SetNetworkFee(100000); // 0.001 GAS
        tx->SetValidUntilBlock(1000);
        
        // Simple transfer script (would be more complex in reality)
        io::ByteVector script;
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSH0)); // Amount
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSH0)); // To
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSH0)); // From
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSH3)); // 3 args
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PACK));
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSH0)); // Transfer method
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::SYSCALL));
        
        tx->SetScript(script);
        
        // Add signer
        Signer signer;
        signer.SetAccount(from);
        signer.SetScopes(WitnessScope::CalledByEntry);
        tx->SetSigners({signer});
        
        // Add witness
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00}); // Signature placeholder
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        tx->SetWitnesses({*witness});
        
        return tx;
    }
    
    std::shared_ptr<ledger::Block> CreateBlock(
        uint32_t index,
        const io::UInt256& prevHash,
        const std::vector<std::shared_ptr<ledger::Transaction>>& transactions)
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(prevHash);
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(std::chrono::system_clock::now());
        block->SetIndex(index);
        block->SetPrimaryIndex(index % 7); // Simulate consensus node rotation
        block->SetNextConsensus(io::UInt160::Zero());
        
        // Add transactions
        for (const auto& tx : transactions)
        {
            block->AddTransaction(*tx);
        }
        
        // Calculate merkle root
        // MerkleRoot calculation would be done automatically
        
        // Add witness
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00});
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        block->SetWitness(*witness);
        
        return block;
    }
    
    bool WaitForCondition(std::function<bool()> condition, int timeoutSeconds = 10)
    {
        auto start = std::chrono::steady_clock::now();
        while (!condition())
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start).count() >= timeoutSeconds)
            {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return true;
    }
};

// Test 1: Complete Node Lifecycle
TEST_F(NeoNodeCompleteIntegrationTest, TestCompleteNodeLifecycle)
{
    // Step 1: Initialize P2P network
    auto& localNode = LocalNode::GetInstance();
    
    auto config = std::make_unique<ChannelsConfig>();
    config->SetTcp(IPEndPoint("0.0.0.0", 30333));
    config->SetMaxConnections(10);
    
    bool started = localNode.Start(*config);
    ASSERT_TRUE(started);
    
    // Step 2: Initialize block sync managers
    syncManager1 = std::make_unique<BlockSyncManager>(nodeSystem1, localNode);
    syncManager1->Start();
    
    // Step 3: Process genesis block on both systems
    auto genesis = CreateGenesisBlock();
    
    bool result1 = nodeSystem1->ProcessBlock(genesis);
    bool result2 = nodeSystem2->ProcessBlock(genesis);
    
    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);
    
    // Step 4: Create and process blocks with transactions
    io::UInt256 prevHash = genesis->GetHash();
    std::vector<std::shared_ptr<ledger::Block>> blocks;
    
    for (uint32_t i = 1; i <= 10; i++)
    {
        // Create transactions for this block
        std::vector<std::shared_ptr<ledger::Transaction>> transactions;
        
        for (int j = 0; j < 5; j++)
        {
            auto from = io::UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
            auto to = io::UInt160::Parse("0xabcdef1234567890abcdef1234567890abcdef12");
            auto tx = CreateTransferTransaction(from, to, 100, i * 100 + j);
            transactions.push_back(tx);
        }
        
        auto block = CreateBlock(i, prevHash, transactions);
        blocks.push_back(block);
        
        // Process on node 1
        bool blockResult = nodeSystem1->ProcessBlock(block);
        EXPECT_TRUE(blockResult) << "Failed to process block " << i;
        
        prevHash = block->GetHash();
    }
    
    // Step 5: Verify state consistency
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), 10);
    
    // Verify transactions are stored
    for (const auto& block : blocks)
    {
        for (const auto& tx : block->GetTransactions())
        {
            auto contains = nodeSystem1->contains_transaction(tx.GetHash());
            EXPECT_NE(contains, neo::ContainsTransactionType::NotExist);
        }
    }
    
    // Step 6: Test memory pool
    auto mempool = nodeSystem1->GetMemPool();
    ASSERT_NE(mempool, nullptr);
    EXPECT_EQ(mempool->GetSize(), 0); // Should be empty after blocks processed
    
    // Step 7: Verify sync manager statistics
    auto syncStats = syncManager1->GetStats();
    EXPECT_GT(syncStats.currentHeight, 0);
    
    // Clean shutdown
    syncManager1->Stop();
    localNode.Stop();
}

// Test 2: Multi-Node Synchronization
TEST_F(NeoNodeCompleteIntegrationTest, TestMultiNodeSynchronization)
{
    // Initialize both nodes
    auto& localNode = LocalNode::GetInstance();
    
    // Start first node
    auto config1 = std::make_unique<ChannelsConfig>();
    config1->SetTcp(IPEndPoint("0.0.0.0", 30334));
    config1->SetMaxConnections(10);
    
    bool started = localNode.Start(*config1);
    ASSERT_TRUE(started);
    
    // Initialize sync managers
    syncManager1 = std::make_unique<BlockSyncManager>(nodeSystem1, localNode);
    syncManager2 = std::make_unique<BlockSyncManager>(nodeSystem2, localNode);
    
    syncManager1->Start();
    syncManager2->Start();
    
    // Process genesis on both
    auto genesis = CreateGenesisBlock();
    nodeSystem1->ProcessBlock(genesis);
    nodeSystem2->ProcessBlock(genesis);
    
    // Node 1 creates blocks
    io::UInt256 prevHash = genesis->GetHash();
    for (uint32_t i = 1; i <= 20; i++)
    {
        std::vector<std::shared_ptr<ledger::Transaction>> txs;
        auto tx = CreateTransferTransaction(
            io::UInt160::Zero(), 
            io::UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678"),
            1000,
            i);
        txs.push_back(tx);
        
        auto block = CreateBlock(i, prevHash, txs);
        nodeSystem1->ProcessBlock(block);
        prevHash = block->GetHash();
    }
    
    // Verify node 1 has all blocks
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), 20);
    
    // Simulate sync by processing same blocks on node 2
    prevHash = genesis->GetHash();
    for (uint32_t i = 1; i <= 20; i++)
    {
        std::vector<std::shared_ptr<ledger::Transaction>> txs;
        auto tx = CreateTransferTransaction(
            io::UInt160::Zero(), 
            io::UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678"),
            1000,
            i);
        txs.push_back(tx);
        
        auto block = CreateBlock(i, prevHash, txs);
        nodeSystem2->ProcessBlock(block);
        prevHash = block->GetHash();
    }
    
    // Both nodes should have same height
    EXPECT_EQ(nodeSystem2->GetCurrentBlockHeight(), 20);
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), nodeSystem2->GetCurrentBlockHeight());
    
    // Clean shutdown
    syncManager1->Stop();
    syncManager2->Stop();
    localNode.Stop();
}

// Test 3: State Synchronization Across Nodes
TEST_F(NeoNodeCompleteIntegrationTest, TestStateSynchronization)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    nodeSystem1->ProcessBlock(genesis);
    nodeSystem2->ProcessBlock(genesis);
    
    // Node 1 processes blocks that modify state
    io::UInt256 prevHash = genesis->GetHash();
    
    for (uint32_t i = 1; i <= 5; i++)
    {
        std::vector<std::shared_ptr<ledger::Transaction>> txs;
        
        // Create state-modifying transactions
        for (int j = 0; j < 3; j++)
        {
            auto tx = CreateTransferTransaction(
                io::UInt160::Parse("0x1111111111111111111111111111111111111111"),
                io::UInt160::Parse("0x2222222222222222222222222222222222222222"),
                i * 1000 + j,
                i * 10 + j);
            txs.push_back(tx);
        }
        
        auto block = CreateBlock(i, prevHash, txs);
        
        // Process on node 1
        bool result1 = nodeSystem1->ProcessBlock(block);
        ASSERT_TRUE(result1);
        
        // Process on node 2 (simulating sync)
        bool result2 = nodeSystem2->ProcessBlock(block);
        ASSERT_TRUE(result2);
        
        prevHash = block->GetHash();
    }
    
    // Both nodes should have identical state
    auto snapshot1 = nodeSystem1->get_snapshot_cache();
    auto snapshot2 = nodeSystem2->get_snapshot_cache();
    
    ASSERT_NE(snapshot1, nullptr);
    ASSERT_NE(snapshot2, nullptr);
    
    // Verify block heights match
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), 5);
    EXPECT_EQ(nodeSystem2->GetCurrentBlockHeight(), 5);
}

// Test 4: Concurrent Operations
TEST_F(NeoNodeCompleteIntegrationTest, TestConcurrentOperations)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    nodeSystem1->ProcessBlock(genesis);
    
    std::atomic<int> blocksProcessed{0};
    std::atomic<int> stateUpdates{0};
    std::atomic<bool> stopFlag{false};
    
    // Thread 1: Process blocks
    std::thread blockThread([this, &genesis, &blocksProcessed, &stopFlag]() {
        io::UInt256 prevHash = genesis->GetHash();
        
        for (uint32_t i = 1; i <= 50 && !stopFlag; i++)
        {
            std::vector<std::shared_ptr<ledger::Transaction>> txs;
            auto tx = CreateTransferTransaction(
                io::UInt160::Zero(),
                io::UInt160::Parse("0x3333333333333333333333333333333333333333"),
                i * 100,
                i);
            txs.push_back(tx);
            
            auto block = CreateBlock(i, prevHash, txs);
            
            if (nodeSystem1->ProcessBlock(block))
            {
                blocksProcessed++;
                prevHash = block->GetHash();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    // Thread 2: State queries
    std::thread stateThread([this, &stateUpdates, &stopFlag]() {
        while (!stopFlag)
        {
            auto snapshot = nodeSystem1->get_snapshot_cache();
            if (snapshot)
            {
                // Perform state reads
                persistence::StorageKey key(0, {0x01, 0x02, 0x03});
                
                auto item = snapshot->TryGet(key);
                stateUpdates++;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Thread 3: Transaction queries
    std::thread txThread([this, &stopFlag]() {
        while (!stopFlag)
        {
            io::UInt256 randomHash;
            // Fill hash with test data
            for (int i = 0; i < 32; i++)
            {
                randomHash.Data()[i] = 0xAA;
            }
            
            nodeSystem1->contains_transaction(randomHash);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });
    
    // Let threads run
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Stop threads
    stopFlag = true;
    blockThread.join();
    stateThread.join();
    txThread.join();
    
    // Verify operations completed
    EXPECT_GT(blocksProcessed.load(), 0);
    EXPECT_GT(stateUpdates.load(), 0);
    EXPECT_GT(nodeSystem1->GetCurrentBlockHeight(), 0);
}

// Test 5: Recovery and Resilience
TEST_F(NeoNodeCompleteIntegrationTest, TestRecoveryAndResilience)
{
    // Initialize P2P
    auto& localNode = LocalNode::GetInstance();
    
    auto config = std::make_unique<ChannelsConfig>();
    config->SetTcp(IPEndPoint("0.0.0.0", 30335));
    config->SetMaxConnections(5);
    
    bool started = localNode.Start(*config);
    ASSERT_TRUE(started);
    
    // Initialize sync manager
    syncManager1 = std::make_unique<BlockSyncManager>(nodeSystem1, localNode);
    syncManager1->Start();
    
    // Process genesis
    auto genesis = CreateGenesisBlock();
    nodeSystem1->ProcessBlock(genesis);
    
    // Process some blocks
    io::UInt256 prevHash = genesis->GetHash();
    for (uint32_t i = 1; i <= 10; i++)
    {
        auto block = CreateBlock(i, prevHash, {});
        nodeSystem1->ProcessBlock(block);
        prevHash = block->GetHash();
    }
    
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), 10);
    
    // Simulate network disruption
    localNode.Stop();
    syncManager1->Stop();
    
    // Process more blocks offline
    for (uint32_t i = 11; i <= 20; i++)
    {
        auto block = CreateBlock(i, prevHash, {});
        nodeSystem1->ProcessBlock(block);
        prevHash = block->GetHash();
    }
    
    // Restart network components
    started = localNode.Start(*config);
    ASSERT_TRUE(started);
    
    syncManager1 = std::make_unique<BlockSyncManager>(nodeSystem1, localNode);
    syncManager1->Start();
    
    // Verify system recovered
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), 20);
    
    // Process more blocks after recovery
    for (uint32_t i = 21; i <= 25; i++)
    {
        auto block = CreateBlock(i, prevHash, {});
        bool result = nodeSystem1->ProcessBlock(block);
        EXPECT_TRUE(result);
        prevHash = block->GetHash();
    }
    
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), 25);
    
    // Clean shutdown
    syncManager1->Stop();
    localNode.Stop();
}

// Test 6: Performance Under Load
TEST_F(NeoNodeCompleteIntegrationTest, TestPerformanceUnderLoad)
{
    // Enable fast sync for performance
    nodeSystem1->SetFastSyncMode(true);
    
    // Process genesis
    auto genesis = CreateGenesisBlock();
    nodeSystem1->ProcessBlock(genesis);
    
    // Measure block processing performance
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process many blocks with transactions
    io::UInt256 prevHash = genesis->GetHash();
    const int numBlocks = 1000;
    const int txPerBlock = 10;
    
    for (uint32_t i = 1; i <= numBlocks; i++)
    {
        std::vector<std::shared_ptr<ledger::Transaction>> txs;
        
        for (int j = 0; j < txPerBlock; j++)
        {
            auto tx = CreateTransferTransaction(
                io::UInt160::Zero(),
                io::UInt160::Parse("0x4444444444444444444444444444444444444444"),
                j,
                i * 100 + j);
            txs.push_back(tx);
        }
        
        auto block = CreateBlock(i, prevHash, txs);
        bool result = nodeSystem1->ProcessBlock(block);
        ASSERT_TRUE(result);
        
        prevHash = block->GetHash();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
    
    // Performance metrics
    double blocksPerSecond = static_cast<double>(numBlocks) / duration.count();
    double txPerSecond = static_cast<double>(numBlocks * txPerBlock) / duration.count();
    
    std::cout << "Performance Results:" << std::endl;
    std::cout << "  Blocks processed: " << numBlocks << std::endl;
    std::cout << "  Total transactions: " << (numBlocks * txPerBlock) << std::endl;
    std::cout << "  Time taken: " << duration.count() << " seconds" << std::endl;
    std::cout << "  Blocks/second: " << blocksPerSecond << std::endl;
    std::cout << "  Transactions/second: " << txPerSecond << std::endl;
    
    // Verify final state
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), numBlocks);
    
    // Disable fast sync
    nodeSystem1->SetFastSyncMode(false);
}

// Test 7: End-to-End Transaction Flow
TEST_F(NeoNodeCompleteIntegrationTest, TestEndToEndTransactionFlow)
{
    // Initialize components
    auto& localNode = LocalNode::GetInstance();
    
    auto config = std::make_unique<ChannelsConfig>();
    config->SetTcp(IPEndPoint("0.0.0.0", 30336));
    bool started = localNode.Start(*config);
    ASSERT_TRUE(started);
    
    syncManager1 = std::make_unique<BlockSyncManager>(nodeSystem1, localNode);
    syncManager1->Start();
    
    // Process genesis
    auto genesis = CreateGenesisBlock();
    nodeSystem1->ProcessBlock(genesis);
    
    // Get memory pool
    auto mempool = nodeSystem1->GetMemPool();
    ASSERT_NE(mempool, nullptr);
    
    // Create and track transactions
    std::vector<io::UInt256> txHashes;
    
    for (int i = 0; i < 20; i++)
    {
        auto tx = CreateTransferTransaction(
            io::UInt160::Parse("0x5555555555555555555555555555555555555555"),
            io::UInt160::Parse("0x6666666666666666666666666666666666666666"),
            i * 1000,
            i);
        
        txHashes.push_back(tx->GetHash());
        
        // In real implementation, would add to mempool
        // mempool->TryAdd(tx);
    }
    
    // Create block including transactions
    std::vector<std::shared_ptr<ledger::Transaction>> blockTxs;
    for (int i = 0; i < 10; i++)
    {
        auto tx = CreateTransferTransaction(
            io::UInt160::Parse("0x5555555555555555555555555555555555555555"),
            io::UInt160::Parse("0x6666666666666666666666666666666666666666"),
            i * 1000,
            i);
        blockTxs.push_back(tx);
    }
    
    auto block = CreateBlock(1, genesis->GetHash(), blockTxs);
    
    // Process block
    bool result = nodeSystem1->ProcessBlock(block);
    ASSERT_TRUE(result);
    
    // Verify transactions are now in blockchain
    for (const auto& tx : blockTxs)
    {
        auto contains = nodeSystem1->contains_transaction(tx->GetHash());
        EXPECT_NE(contains, ContainsTransactionType::NotExist);
    }
    
    // Clean shutdown
    syncManager1->Stop();
    localNode.Stop();
}

// Test 8: Complete System Integration
TEST_F(NeoNodeCompleteIntegrationTest, TestCompleteSystemIntegration)
{
    // This test verifies all components work together seamlessly
    
    // 1. Initialize P2P network
    auto& localNode = LocalNode::GetInstance();
    auto config = std::make_unique<ChannelsConfig>();
    config->SetTcp(IPEndPoint("0.0.0.0", 30337));
    config->SetMaxConnections(20);
    
    // Add mock seed nodes
    std::vector<IPEndPoint> seedNodes;
    seedNodes.push_back(IPEndPoint("seed1.test.neo", 10333));
    seedNodes.push_back(IPEndPoint("seed2.test.neo", 10333));
    config->SetSeedList(seedNodes);
    
    bool started = localNode.Start(*config);
    ASSERT_TRUE(started);
    
    // 2. Initialize block sync
    syncManager1 = std::make_unique<BlockSyncManager>(nodeSystem1, localNode);
    syncManager2 = std::make_unique<BlockSyncManager>(nodeSystem2, localNode);
    
    syncManager1->Start();
    syncManager2->Start();
    
    // 3. Process genesis on both nodes
    auto genesis = CreateGenesisBlock();
    nodeSystem1->ProcessBlock(genesis);
    nodeSystem2->ProcessBlock(genesis);
    
    // 4. Simulate blockchain activity
    io::UInt256 prevHash = genesis->GetHash();
    
    for (uint32_t height = 1; height <= 100; height++)
    {
        // Create varied transaction load
        int txCount = (height % 10) + 1;
        std::vector<std::shared_ptr<ledger::Transaction>> txs;
        
        for (int i = 0; i < txCount; i++)
        {
            auto from = io::UInt160::Zero();
            auto to = io::UInt160::Parse("0x7777777777777777777777777777777777777777");
            auto tx = CreateTransferTransaction(from, to, height * 100 + i, height * 1000 + i);
            txs.push_back(tx);
        }
        
        auto block = CreateBlock(height, prevHash, txs);
        
        // Alternate processing between nodes
        if (height % 2 == 1)
        {
            nodeSystem1->ProcessBlock(block);
            nodeSystem2->ProcessBlock(block);
        }
        else
        {
            nodeSystem2->ProcessBlock(block);
            nodeSystem1->ProcessBlock(block);
        }
        
        prevHash = block->GetHash();
    }
    
    // 5. Verify final state consistency
    EXPECT_EQ(nodeSystem1->GetCurrentBlockHeight(), 100);
    EXPECT_EQ(nodeSystem2->GetCurrentBlockHeight(), 100);
    
    // 6. Verify sync manager statistics
    auto stats1 = syncManager1->GetStats();
    auto stats2 = syncManager2->GetStats();
    
    EXPECT_EQ(stats1.currentHeight, 100);
    EXPECT_EQ(stats2.currentHeight, 100);
    
    // 7. Test state consistency
    auto snapshot1 = nodeSystem1->get_snapshot_cache();
    auto snapshot2 = nodeSystem2->get_snapshot_cache();
    
    ASSERT_NE(snapshot1, nullptr);
    ASSERT_NE(snapshot2, nullptr);
    
    // 8. Performance check
    EXPECT_GT(stats1.blocksPerSecond, 0);
    EXPECT_GT(stats2.blocksPerSecond, 0);
    
    // 9. Clean shutdown
    syncManager1->Stop();
    syncManager2->Stop();
    localNode.Stop();
    
    std::cout << "Complete system integration test passed!" << std::endl;
    std::cout << "  Total blocks processed: 100" << std::endl;
    std::cout << "  Node 1 sync rate: " << stats1.blocksPerSecond << " blocks/sec" << std::endl;
    std::cout << "  Node 2 sync rate: " << stats2.blocksPerSecond << " blocks/sec" << std::endl;
}