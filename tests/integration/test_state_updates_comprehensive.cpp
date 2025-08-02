#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/persistence/store_cache.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/opcode.h>
#include <neo/common/contains_transaction_type.h>
#include <chrono>
#include <thread>
#include <random>

using namespace neo;
using namespace neo::persistence;
using namespace neo::ledger;
using namespace neo::smartcontract;

class StateUpdatesTest : public ::testing::Test
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
    }
    
    std::shared_ptr<ledger::Block> CreateGenesisBlock()
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(io::UInt256::Zero());
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(std::chrono::system_clock::now());
        block->SetIndex(0);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00});
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        block->SetWitness(*witness);
        
        return block;
    }
    
    StorageKey CreateStorageKey(uint8_t prefix, const io::ByteVector& key)
    {
        io::ByteVector fullKey;
        fullKey.Push(prefix);
        fullKey.insert(fullKey.end(), key.begin(), key.end());
        
        StorageKey storageKey(0, fullKey); // Contract ID 0
        return storageKey;
    }
    
    std::shared_ptr<StorageItem> CreateStorageItem(const io::ByteVector& value)
    {
        auto item = std::make_shared<StorageItem>(value);
        return item;
    }
};

// Test 1: Basic State Read/Write
TEST_F(StateUpdatesTest, TestBasicStateReadWrite)
{
    // Get a snapshot of the store
    auto snapshot = system->get_snapshot_cache();
    ASSERT_NE(snapshot, nullptr);
    
    // Create a storage key
    io::ByteVector keyData = {0x01, 0x02, 0x03};
    auto storageKey = CreateStorageKey(0x01, keyData);
    
    // Create a storage item
    io::ByteVector valueData = {0x10, 0x20, 0x30, 0x40};
    auto storageItem = CreateStorageItem(valueData);
    
    // Write to store
    snapshot->Add(storageKey, *storageItem);
    
    // Read back
    auto readItem = snapshot->TryGet(storageKey);
    ASSERT_NE(readItem, nullptr);
    EXPECT_EQ(readItem->GetValue(), valueData);
    
    // Commit changes
    snapshot->Commit();
}

// Test 2: Block Height State Updates
TEST_F(StateUpdatesTest, TestBlockHeightStateUpdates)
{
    // Process genesis block
    auto genesis = CreateGenesisBlock();
    bool result = system->ProcessBlock(genesis);
    ASSERT_TRUE(result);
    
    // Get snapshot
    auto snapshot = system->get_snapshot_cache();
    
    // Check block height in state
    io::ByteVector heightKeyData{0x0C}; // Prefix for current block
    heightKeyData.Push(9); // CurrentBlock prefix
    StorageKey heightKey(0x04, heightKeyData); // Using contract ID 4 for Ledger
    
    auto heightItem = snapshot->TryGet(heightKey);
    EXPECT_NE(heightItem, nullptr);
    
    // Process another block
    auto block1 = std::make_shared<ledger::Block>();
    block1->SetVersion(0);
    block1->SetPreviousHash(genesis->GetHash());
    block1->SetMerkleRoot(io::UInt256::Zero());
    block1->SetTimestamp(std::chrono::system_clock::now());
    block1->SetIndex(1);
    block1->SetPrimaryIndex(0);
    block1->SetNextConsensus(io::UInt160::Zero());
    
    auto witness = std::make_shared<Witness>();
    witness->SetInvocationScript(io::ByteVector{0x00});
    witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
    block1->SetWitness(*witness);
    
    result = system->ProcessBlock(block1);
    EXPECT_TRUE(result);
    
    // Height should be updated
    EXPECT_EQ(system->GetCurrentBlockHeight(), 1);
}

// Test 3: Transaction State Updates
TEST_F(StateUpdatesTest, TestTransactionStateUpdates)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Create block with transaction
    auto block = std::make_shared<ledger::Block>();
    block->SetVersion(0);
    block->SetPreviousHash(genesis->GetHash());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(1);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    // Add transaction
    auto tx = std::make_shared<Transaction>();
    tx->SetVersion(0);
    tx->SetNonce(1234);
    tx->SetSystemFee(0);
    tx->SetNetworkFee(0);
    tx->SetValidUntilBlock(100);
    tx->SetScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
    
    Signer signer;
    signer.SetAccount(io::UInt160::Zero());
    signer.SetScopes(WitnessScope::Global);
    tx->SetSigners({signer});
    
    auto txWitness = std::make_shared<Witness>();
    txWitness->SetInvocationScript(io::ByteVector{0x00});
    txWitness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
    tx->SetWitnesses({*txWitness});
    
    block->AddTransaction(*tx);
    // MerkleRoot calculation would be done automatically
    
    auto blockWitness = std::make_shared<Witness>();
    blockWitness->SetInvocationScript(io::ByteVector{0x00});
    blockWitness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
    block->SetWitness(*blockWitness);
    
    // Process block
    bool result = system->ProcessBlock(block);
    EXPECT_TRUE(result);
    
    // Transaction should be in state
    auto containsResult = system->contains_transaction(tx->GetHash());
    EXPECT_NE(containsResult, ContainsTransactionType::NotExist);
}

// Test 4: Concurrent State Updates
TEST_F(StateUpdatesTest, TestConcurrentStateUpdates)
{
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    // Multiple threads updating different keys
    for (int t = 0; t < 5; t++)
    {
        threads.emplace_back([this, t, &successCount]() {
            try
            {
                auto snapshot = system->get_snapshot_cache();
                
                // Each thread writes to different keys
                for (int i = 0; i < 10; i++)
                {
                    io::ByteVector keyData = {static_cast<uint8_t>(t), static_cast<uint8_t>(i)};
                    auto storageKey = CreateStorageKey(0x02, keyData);
                    
                    io::ByteVector valueData = {static_cast<uint8_t>(t * 10 + i)};
                    auto storageItem = CreateStorageItem(valueData);
                    
                    snapshot->Add(storageKey, *storageItem);
                }
                
                snapshot->Commit();
                successCount++;
            }
            catch (const std::exception& e)
            {
                // Handle errors gracefully
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads)
    {
        t.join();
    }
    
    // All threads should succeed
    EXPECT_EQ(successCount.load(), 5);
    
    // Verify all data was written
    auto snapshot = system->get_snapshot_cache();
    for (int t = 0; t < 5; t++)
    {
        for (int i = 0; i < 10; i++)
        {
            io::ByteVector keyData = {static_cast<uint8_t>(t), static_cast<uint8_t>(i)};
            auto storageKey = CreateStorageKey(0x02, keyData);
            
            auto item = snapshot->TryGet(storageKey);
            EXPECT_NE(item, nullptr);
            if (item)
            {
                EXPECT_EQ(item->GetValue().size(), 1);
                EXPECT_EQ(item->GetValue()[0], t * 10 + i);
            }
        }
    }
}

// Test 5: State Rollback
TEST_F(StateUpdatesTest, TestStateRollback)
{
    // Get initial snapshot
    auto snapshot1 = system->get_snapshot_cache();
    
    // Write some data
    io::ByteVector keyData = {0x01, 0x02, 0x03};
    auto storageKey = CreateStorageKey(0x03, keyData);
    
    io::ByteVector valueData = {0xAA, 0xBB, 0xCC};
    auto storageItem = CreateStorageItem(valueData);
    
    snapshot1->Add(storageKey, *storageItem);
    
    // Don't commit - let snapshot go out of scope
    // Changes should be rolled back
    
    // Get new snapshot
    auto snapshot2 = system->get_snapshot_cache();
    auto item = snapshot2->TryGet(storageKey);
    EXPECT_EQ(item, nullptr); // Should not exist
}

// Test 6: Large State Updates
TEST_F(StateUpdatesTest, TestLargeStateUpdates)
{
    auto snapshot = system->get_snapshot_cache();
    
    // Write many key-value pairs
    const int numEntries = 10000;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numEntries; i++)
    {
        io::ByteVector keyData;
        io::BinaryWriter keyWriter(keyData);
        keyWriter.WriteVarInt(i);
        
        auto storageKey = CreateStorageKey(0x04, keyData);
        
        io::ByteVector valueData;
        io::BinaryWriter valueWriter(valueData);
        valueWriter.WriteVarInt(i * i);
        
        auto storageItem = CreateStorageItem(valueData);
        snapshot->Add(storageKey, *storageItem);
    }
    
    snapshot->Commit();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
    
    // Verify random entries
    auto verifySnapshot = system->get_snapshot_cache();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, numEntries - 1);
    
    for (int i = 0; i < 100; i++)
    {
        int index = dis(gen);
        
        io::ByteVector keyData;
        io::BinaryWriter keyWriter(keyData);
        keyWriter.WriteVarInt(index);
        
        auto storageKey = CreateStorageKey(0x04, keyData);
        auto item = verifySnapshot->TryGet(storageKey);
        
        EXPECT_NE(item, nullptr);
        if (item)
        {
            io::BinaryReader reader(item->GetValue());
            auto value = reader.ReadVarInt();
            EXPECT_EQ(value, index * index);
        }
    }
}

// Test 7: State Migration
TEST_F(StateUpdatesTest, TestStateMigration)
{
    // Write old format data
    auto snapshot = system->get_snapshot_cache();
    
    // Old format: simple key-value
    for (int i = 0; i < 10; i++)
    {
        io::ByteVector keyData = {static_cast<uint8_t>(i)};
        auto storageKey = CreateStorageKey(0x05, keyData);
        
        io::ByteVector valueData = {static_cast<uint8_t>(i * 10)};
        auto storageItem = CreateStorageItem(valueData);
        
        snapshot->Add(storageKey, *storageItem);
    }
    
    snapshot->Commit();
    
    // Simulate migration to new format
    auto migrationSnapshot = system->get_snapshot_cache();
    
    // Read old data and write in new format
    for (int i = 0; i < 10; i++)
    {
        // Read old
        io::ByteVector oldKeyData = {static_cast<uint8_t>(i)};
        auto oldStorageKey = CreateStorageKey(0x05, oldKeyData);
        auto oldItem = migrationSnapshot->TryGet(oldStorageKey);
        
        ASSERT_NE(oldItem, nullptr);
        
        // Write new format (with version prefix)
        io::ByteVector newKeyData = {0x01, static_cast<uint8_t>(i)}; // Version 1
        auto newStorageKey = CreateStorageKey(0x06, newKeyData);
        
        io::ByteVector newValueData = {0x01}; // Version
        newValueData.insert(newValueData.end(), oldItem->GetValue().begin(), oldItem->GetValue().end());
        auto newStorageItem = CreateStorageItem(newValueData);
        
        migrationSnapshot->Add(newStorageKey, *newStorageItem);
        
        // Delete old
        migrationSnapshot->Delete(oldStorageKey);
    }
    
    migrationSnapshot->Commit();
    
    // Verify migration
    auto verifySnapshot = system->get_snapshot_cache();
    for (int i = 0; i < 10; i++)
    {
        // Old should not exist
        io::ByteVector oldKeyData = {static_cast<uint8_t>(i)};
        auto oldStorageKey = CreateStorageKey(0x05, oldKeyData);
        EXPECT_EQ(verifySnapshot->TryGet(oldStorageKey), nullptr);
        
        // New should exist with correct format
        io::ByteVector newKeyData = {0x01, static_cast<uint8_t>(i)};
        auto newStorageKey = CreateStorageKey(0x06, newKeyData);
        auto newItem = verifySnapshot->TryGet(newStorageKey);
        
        ASSERT_NE(newItem, nullptr);
        EXPECT_EQ(newItem->GetValue().size(), 2);
        EXPECT_EQ(newItem->GetValue()[0], 0x01); // Version
        EXPECT_EQ(newItem->GetValue()[1], i * 10); // Original value
    }
}

// Test 8: State Consistency
TEST_F(StateUpdatesTest, TestStateConsistency)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Write initial state
    auto snapshot1 = system->get_snapshot_cache();
    
    std::map<StorageKey, io::ByteVector> expectedState;
    
    for (int i = 0; i < 20; i++)
    {
        io::ByteVector keyData = {static_cast<uint8_t>(i)};
        auto storageKey = CreateStorageKey(0x07, keyData);
        
        io::ByteVector valueData = {static_cast<uint8_t>(i), static_cast<uint8_t>(i * 2)};
        auto storageItem = CreateStorageItem(valueData);
        
        snapshot1->Add(storageKey, *storageItem);
        expectedState[storageKey] = valueData;
    }
    
    snapshot1->Commit();
    
    // Multiple updates maintaining consistency
    for (int round = 0; round < 5; round++)
    {
        auto snapshot = system->get_snapshot_cache();
        
        // Update half the values
        for (int i = 0; i < 10; i++)
        {
            io::ByteVector keyData = {static_cast<uint8_t>(i)};
            auto storageKey = CreateStorageKey(0x07, keyData);
            
            io::ByteVector newValueData = {static_cast<uint8_t>(round), static_cast<uint8_t>(i), static_cast<uint8_t>(i * 3)};
            auto storageItem = CreateStorageItem(newValueData);
            
            snapshot->Update(storageKey, *storageItem);
            expectedState[storageKey] = newValueData;
        }
        
        snapshot->Commit();
    }
    
    // Verify final state matches expected
    auto verifySnapshot = system->get_snapshot_cache();
    for (const auto& [key, expectedValue] : expectedState)
    {
        auto item = verifySnapshot->TryGet(key);
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(item->GetValue(), expectedValue);
    }
}

// Test 9: State Query Performance
TEST_F(StateUpdatesTest, TestStateQueryPerformance)
{
    // Populate state with data
    auto populateSnapshot = system->get_snapshot_cache();
    
    const int numEntries = 100000;
    for (int i = 0; i < numEntries; i++)
    {
        io::ByteVector keyData;
        io::BinaryWriter keyWriter(keyData);
        keyWriter.WriteVarInt(i);
        
        auto storageKey = CreateStorageKey(0x08, keyData);
        
        io::ByteVector valueData;
        for (int j = 0; j < 10; j++)
        {
            valueData.Push(static_cast<uint8_t>(i % 256));
        }
        auto storageItem = CreateStorageItem(valueData);
        
        populateSnapshot->Add(storageKey, *storageItem);
    }
    
    populateSnapshot->Commit();
    
    // Test query performance
    auto querySnapshot = system->get_snapshot_cache();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, numEntries - 1);
    
    const int numQueries = 10000;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numQueries; i++)
    {
        int index = dis(gen);
        
        io::ByteVector keyData;
        io::BinaryWriter keyWriter(keyData);
        keyWriter.WriteVarInt(index);
        
        auto storageKey = CreateStorageKey(0x08, keyData);
        auto item = querySnapshot->TryGet(storageKey);
        
        EXPECT_NE(item, nullptr);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Should complete quickly
    EXPECT_LT(duration.count(), 1000); // Less than 1 second for 10k queries
}

// Test 10: Complex State Operations
TEST_F(StateUpdatesTest, TestComplexStateOperations)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Test complex state manipulations
    auto snapshot = system->get_snapshot_cache();
    
    // 1. Batch insert
    std::vector<std::pair<StorageKey, io::ByteVector>> batch;
    for (int i = 0; i < 50; i++)
    {
        io::ByteVector keyData = {0x01, static_cast<uint8_t>(i)};
        auto storageKey = CreateStorageKey(0x09, keyData);
        
        io::ByteVector valueData;
        for (int j = 0; j <= i; j++) valueData.Push(static_cast<uint8_t>(i));
        batch.push_back({storageKey, valueData});
    }
    
    // Insert all
    for (const auto& [key, value] : batch)
    {
        auto item = CreateStorageItem(value);
        snapshot->Add(key, *item);
    }
    
    // 2. Selective update
    for (int i = 0; i < 50; i += 2)
    {
        io::ByteVector keyData = {0x01, static_cast<uint8_t>(i)};
        auto storageKey = CreateStorageKey(0x09, keyData);
        
        io::ByteVector newValue = {0xFF, 0xFF};
        auto item = CreateStorageItem(newValue);
        snapshot->Update(storageKey, *item);
    }
    
    // 3. Selective delete
    for (int i = 1; i < 50; i += 4)
    {
        io::ByteVector keyData = {0x01, static_cast<uint8_t>(i)};
        auto storageKey = CreateStorageKey(0x09, keyData);
        snapshot->Delete(storageKey);
    }
    
    snapshot->Commit();
    
    // Verify final state
    auto verifySnapshot = system->get_snapshot_cache();
    for (int i = 0; i < 50; i++)
    {
        io::ByteVector keyData = {0x01, static_cast<uint8_t>(i)};
        auto storageKey = CreateStorageKey(0x09, keyData);
        auto item = verifySnapshot->TryGet(storageKey);
        
        if (i % 4 == 1)
        {
            // Should be deleted
            EXPECT_EQ(item, nullptr);
        }
        else if (i % 2 == 0)
        {
            // Should be updated
            ASSERT_NE(item, nullptr);
            EXPECT_EQ(item->GetValue(), io::ByteVector({0xFF, 0xFF}));
        }
        else
        {
            // Should be original
            ASSERT_NE(item, nullptr);
            EXPECT_EQ(item->GetValue().size(), i + 1);
        }
    }
}