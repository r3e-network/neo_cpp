#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/network/p2p/block_sync_manager.h>
#include <neo/persistence/store_cache.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/witness.h>
#include <neo/ledger/signer.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/vm/opcode.h>
#include <neo/common/contains_transaction_type.h>
#include <chrono>
#include <thread>
#include <random>

using namespace neo;
using namespace neo::network;
using namespace neo::network::p2p;
using namespace neo::ledger;
using namespace neo::persistence;

/**
 * Integration tests to verify all 4 core capabilities work together
 */
class NeoCapabilitiesIntegrationTest : public ::testing::Test
{
protected:
    std::shared_ptr<NeoSystem> system;
    
    void SetUp() override
    {
        auto settings = std::make_unique<ProtocolSettings>();
        system = std::make_shared<NeoSystem>(std::move(settings), "memory");
    }
    
    void TearDown() override
    {
        if (system) system->stop();
        // Clean up LocalNode singleton
        LocalNode::GetInstance().Stop();
    }
    
    std::shared_ptr<ledger::Block> CreateTestBlock(uint32_t index, const io::UInt256& prevHash)
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(prevHash);
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
        block->SetIndex(index);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        
        // Add witness
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00});
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        block->SetWitness(*witness);
        
        return block;
    }
    
    std::shared_ptr<Transaction> CreateTestTransaction(uint32_t nonce)
    {
        auto tx = std::make_shared<Transaction>();
        tx->SetVersion(0);
        tx->SetNonce(nonce);
        tx->SetSystemFee(0);
        tx->SetNetworkFee(0);
        tx->SetValidUntilBlock(100);
        tx->SetScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        
        Signer signer;
        signer.SetAccount(io::UInt160::Zero());
        signer.SetScopes(WitnessScope::Global);
        tx->SetSigners({signer});
        
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00});
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        tx->SetWitnesses({*witness});
        
        return tx;
    }
};

// Test 1: P2P + Block Processing Integration
TEST_F(NeoCapabilitiesIntegrationTest, TestP2PAndBlockProcessing)
{
    // Start P2P node
    auto& localNode = LocalNode::GetInstance();
    auto config = std::make_unique<ChannelsConfig>();
    config->SetTcp(IPEndPoint("0.0.0.0", 20444));
    config->SetMaxConnections(10);
    
    bool started = localNode.Start(*config);
    ASSERT_TRUE(started);
    
    // Create and process a block
    auto block = CreateTestBlock(0, io::UInt256::Zero());
    bool processed = system->ProcessBlock(block);
    EXPECT_TRUE(processed);
    
    // Verify block was stored
    EXPECT_EQ(system->GetCurrentBlockHeight(), 0);
    
    localNode.Stop();
}

// Test 2: Block Sync + State Update Integration
TEST_F(NeoCapabilitiesIntegrationTest, TestBlockSyncAndStateUpdate)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Start sync manager
    auto syncManager = std::make_unique<BlockSyncManager>(system, localNode);
    syncManager->Start();
    
    // Process genesis block
    auto genesis = CreateTestBlock(0, io::UInt256::Zero());
    system->ProcessBlock(genesis);
    
    // Create state update
    auto snapshot = system->get_snapshot_cache();
    ASSERT_NE(snapshot, nullptr);
    
    StorageKey testKey(1, {0x01, 0x02, 0x03});
    StorageItem testItem(io::ByteVector{0x10, 0x20, 0x30});
    snapshot->Add(testKey, testItem);
    snapshot->Commit();
    
    // Verify state was updated
    auto newSnapshot = system->get_snapshot_cache();
    auto retrievedItem = newSnapshot->TryGet(testKey);
    ASSERT_NE(retrievedItem, nullptr);
    EXPECT_EQ(retrievedItem->GetValue(), testItem.GetValue());
    
    syncManager->Stop();
}

// Test 3: Transaction Execution + State Update Integration
TEST_F(NeoCapabilitiesIntegrationTest, TestTransactionExecutionAndState)
{
    // Process genesis
    auto genesis = CreateTestBlock(0, io::UInt256::Zero());
    system->ProcessBlock(genesis);
    
    // Create block with transaction
    auto block = CreateTestBlock(1, genesis->GetHash());
    auto tx = CreateTestTransaction(1234);
    block->AddTransaction(*tx);
    
    // Process block
    bool processed = system->ProcessBlock(block);
    EXPECT_TRUE(processed);
    
    // Verify transaction is stored
    auto contains = system->contains_transaction(tx->GetHash());
    EXPECT_NE(contains, neo::ContainsTransactionType::NotExist);
    
    // Verify state updated
    EXPECT_EQ(system->GetCurrentBlockHeight(), 1);
}

// Test 4: Complete Capability Integration
TEST_F(NeoCapabilitiesIntegrationTest, TestCompleteIntegration)
{
    // 1. Start P2P
    auto& localNode = LocalNode::GetInstance();
    auto config = std::make_unique<ChannelsConfig>();
    config->SetTcp(IPEndPoint("0.0.0.0", 20445));
    bool p2pStarted = localNode.Start(*config);
    ASSERT_TRUE(p2pStarted);
    
    // 2. Start Block Sync
    auto syncManager = std::make_unique<BlockSyncManager>(system, localNode);
    syncManager->Start();
    
    // 3. Process blocks with transactions
    io::UInt256 prevHash = io::UInt256::Zero();
    for (uint32_t i = 0; i < 5; i++)
    {
        auto block = CreateTestBlock(i, prevHash);
        
        // Add transactions
        for (int j = 0; j < 3; j++)
        {
            auto tx = CreateTestTransaction(i * 100 + j);
            block->AddTransaction(*tx);
        }
        
        bool processed = system->ProcessBlock(block);
        EXPECT_TRUE(processed);
        prevHash = block->GetHash();
    }
    
    // 4. Verify state updates
    EXPECT_EQ(system->GetCurrentBlockHeight(), 4);
    
    // Write custom state
    auto snapshot = system->get_snapshot_cache();
    StorageKey appKey(100, {0xAA, 0xBB, 0xCC});
    StorageItem appData(io::ByteVector{0x01, 0x02, 0x03, 0x04, 0x05});
    snapshot->Add(appKey, appData);
    snapshot->Commit();
    
    // Verify custom state
    auto verifySnapshot = system->get_snapshot_cache();
    auto item = verifySnapshot->TryGet(appKey);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->GetValue(), appData.GetValue());
    
    // Clean up
    syncManager->Stop();
    localNode.Stop();
}

// Test 5: Concurrent Operations
TEST_F(NeoCapabilitiesIntegrationTest, TestConcurrentOperations)
{
    std::atomic<bool> stopFlag{false};
    std::atomic<int> blocksProcessed{0};
    std::atomic<int> stateUpdates{0};
    
    // Thread 1: Process blocks
    std::thread blockThread([this, &stopFlag, &blocksProcessed]() {
        io::UInt256 prevHash = io::UInt256::Zero();
        for (uint32_t i = 0; i < 10 && !stopFlag; i++)
        {
            auto block = CreateTestBlock(i, prevHash);
            if (system->ProcessBlock(block))
            {
                blocksProcessed++;
                prevHash = block->GetHash();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    // Thread 2: State operations
    std::thread stateThread([this, &stopFlag, &stateUpdates]() {
        while (!stopFlag && stateUpdates < 20)
        {
            auto snapshot = system->get_snapshot_cache();
            if (snapshot)
            {
                StorageKey key(200, {static_cast<uint8_t>(stateUpdates)});
                StorageItem item(io::ByteVector{static_cast<uint8_t>(stateUpdates * 2)});
                snapshot->Add(key, item);
                snapshot->Commit();
                stateUpdates++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    });
    
    // Let threads run
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stopFlag = true;
    
    blockThread.join();
    stateThread.join();
    
    // Verify operations completed
    EXPECT_GT(blocksProcessed.load(), 0);
    EXPECT_GT(stateUpdates.load(), 0);
}

// Test 6: Error Recovery
TEST_F(NeoCapabilitiesIntegrationTest, TestErrorRecovery)
{
    // Start P2P
    auto& localNode = LocalNode::GetInstance();
    auto config = std::make_unique<ChannelsConfig>();
    config->SetTcp(IPEndPoint("0.0.0.0", 20446));
    localNode.Start(*config);
    
    // Process some blocks
    io::UInt256 prevHash = io::UInt256::Zero();
    for (uint32_t i = 0; i < 3; i++)
    {
        auto block = CreateTestBlock(i, prevHash);
        system->ProcessBlock(block);
        prevHash = block->GetHash();
    }
    
    // Stop and restart P2P
    localNode.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    localNode.Start(*config);
    
    // Continue processing blocks
    for (uint32_t i = 3; i < 5; i++)
    {
        auto block = CreateTestBlock(i, prevHash);
        bool processed = system->ProcessBlock(block);
        EXPECT_TRUE(processed);
        prevHash = block->GetHash();
    }
    
    EXPECT_EQ(system->GetCurrentBlockHeight(), 4);
    
    localNode.Stop();
}

// Test 7: Performance Test
TEST_F(NeoCapabilitiesIntegrationTest, TestPerformance)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process many blocks
    io::UInt256 prevHash = io::UInt256::Zero();
    const int numBlocks = 100;
    
    for (uint32_t i = 0; i < numBlocks; i++)
    {
        auto block = CreateTestBlock(i, prevHash);
        
        // Add some transactions
        for (int j = 0; j < 5; j++)
        {
            auto tx = CreateTestTransaction(i * 1000 + j);
            block->AddTransaction(*tx);
        }
        
        bool processed = system->ProcessBlock(block);
        ASSERT_TRUE(processed);
        prevHash = block->GetHash();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Processed " << numBlocks << " blocks in " << duration.count() << "ms" << std::endl;
    std::cout << "Average: " << (duration.count() / numBlocks) << "ms per block" << std::endl;
    
    EXPECT_EQ(system->GetCurrentBlockHeight(), numBlocks - 1);
}

// Test 8: Memory Pool Integration
TEST_F(NeoCapabilitiesIntegrationTest, TestMemoryPoolIntegration)
{
    auto mempool = system->GetMemPool();
    ASSERT_NE(mempool, nullptr);
    
    // Process genesis
    auto genesis = CreateTestBlock(0, io::UInt256::Zero());
    system->ProcessBlock(genesis);
    
    // Create multiple transactions
    std::vector<std::shared_ptr<Transaction>> transactions;
    for (int i = 0; i < 10; i++)
    {
        auto tx = CreateTestTransaction(5000 + i);
        transactions.push_back(tx);
    }
    
    // Create block with some transactions
    auto block = CreateTestBlock(1, genesis->GetHash());
    for (int i = 0; i < 5; i++)
    {
        block->AddTransaction(*transactions[i]);
    }
    
    // Process block
    bool processed = system->ProcessBlock(block);
    EXPECT_TRUE(processed);
    
    // Verify included transactions are in blockchain
    for (int i = 0; i < 5; i++)
    {
        auto contains = system->contains_transaction(transactions[i]->GetHash());
        EXPECT_NE(contains, neo::ContainsTransactionType::NotExist);
    }
}