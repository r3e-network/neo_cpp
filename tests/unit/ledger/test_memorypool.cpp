// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_memorypool.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_MEMORYPOOL_CPP_H
#define TESTS_UNIT_LEDGER_TEST_MEMORYPOOL_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/ledger/memory_pool.h>

namespace neo {
namespace test {

class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for MemoryPool testing
        protocol_settings = std::make_shared<ProtocolSettings>();
        protocol_settings->SetNetwork(0x860833102);
        protocol_settings->SetMemPoolMaxTransactions(50000);
        
        memory_pool = std::make_shared<ledger::MemoryPool>(protocol_settings);
        
        // Create test transactions
        test_tx1 = CreateTestTransaction(io::UInt256::Parse("1111111111111111111111111111111111111111111111111111111111111111"));
        test_tx2 = CreateTestTransaction(io::UInt256::Parse("2222222222222222222222222222222222222222222222222222222222222222"));
        test_tx3 = CreateTestTransaction(io::UInt256::Parse("3333333333333333333333333333333333333333333333333333333333333333"));
    }

    void TearDown() override {
        // Clean up test fixtures
        memory_pool->Clear();
        memory_pool.reset();
        protocol_settings.reset();
        test_tx1.reset();
        test_tx2.reset();
        test_tx3.reset();
    }

    // Helper methods and test data for MemoryPool testing
    std::shared_ptr<ProtocolSettings> protocol_settings;
    std::shared_ptr<ledger::MemoryPool> memory_pool;
    std::shared_ptr<ledger::Transaction> test_tx1;
    std::shared_ptr<ledger::Transaction> test_tx2;
    std::shared_ptr<ledger::Transaction> test_tx3;
    
    // Helper to create test transactions
    std::shared_ptr<ledger::Transaction> CreateTestTransaction(const io::UInt256& hash) {
        auto tx = std::make_shared<ledger::Transaction>();
        tx->SetHash(hash);
        tx->SetNetworkFee(1000000); // 0.01 GAS
        tx->SetSystemFee(2000000);  // 0.02 GAS
        return tx;
    }
};

// MemoryPool test methods converted from C# UT_MemoryPool.cs functionality

TEST_F(MemoryPoolTest, ConstructorCreatesEmptyPool) {
    EXPECT_EQ(memory_pool->GetCount(), 0);
    EXPECT_TRUE(memory_pool->IsEmpty());
}

TEST_F(MemoryPoolTest, AddTransactionToPool) {
    bool added = memory_pool->TryAdd(test_tx1);
    EXPECT_TRUE(added);
    EXPECT_EQ(memory_pool->GetCount(), 1);
    EXPECT_FALSE(memory_pool->IsEmpty());
}

TEST_F(MemoryPoolTest, AddDuplicateTransaction) {
    EXPECT_TRUE(memory_pool->TryAdd(test_tx1));
    EXPECT_EQ(memory_pool->GetCount(), 1);
    
    // Adding same transaction again should fail
    EXPECT_FALSE(memory_pool->TryAdd(test_tx1));
    EXPECT_EQ(memory_pool->GetCount(), 1); // Count unchanged
}

TEST_F(MemoryPoolTest, AddMultipleTransactions) {
    EXPECT_TRUE(memory_pool->TryAdd(test_tx1));
    EXPECT_TRUE(memory_pool->TryAdd(test_tx2));
    EXPECT_TRUE(memory_pool->TryAdd(test_tx3));
    
    EXPECT_EQ(memory_pool->GetCount(), 3);
}

TEST_F(MemoryPoolTest, ContainsTransaction) {
    memory_pool->TryAdd(test_tx1);
    
    EXPECT_TRUE(memory_pool->Contains(test_tx1->GetHash()));
    EXPECT_FALSE(memory_pool->Contains(test_tx2->GetHash()));
}

TEST_F(MemoryPoolTest, RemoveTransaction) {
    memory_pool->TryAdd(test_tx1);
    memory_pool->TryAdd(test_tx2);
    EXPECT_EQ(memory_pool->GetCount(), 2);
    
    bool removed = memory_pool->TryRemove(test_tx1->GetHash());
    EXPECT_TRUE(removed);
    EXPECT_EQ(memory_pool->GetCount(), 1);
    EXPECT_FALSE(memory_pool->Contains(test_tx1->GetHash()));
    EXPECT_TRUE(memory_pool->Contains(test_tx2->GetHash()));
}

TEST_F(MemoryPoolTest, RemoveNonExistentTransaction) {
    memory_pool->TryAdd(test_tx1);
    
    bool removed = memory_pool->TryRemove(test_tx2->GetHash());
    EXPECT_FALSE(removed);
    EXPECT_EQ(memory_pool->GetCount(), 1); // Count unchanged
}

TEST_F(MemoryPoolTest, GetTransaction) {
    memory_pool->TryAdd(test_tx1);
    
    auto retrieved_tx = memory_pool->GetTransaction(test_tx1->GetHash());
    EXPECT_NE(retrieved_tx, nullptr);
    EXPECT_EQ(retrieved_tx->GetHash(), test_tx1->GetHash());
    
    auto non_existent = memory_pool->GetTransaction(test_tx2->GetHash());
    EXPECT_EQ(non_existent, nullptr);
}

TEST_F(MemoryPoolTest, ClearPool) {
    memory_pool->TryAdd(test_tx1);
    memory_pool->TryAdd(test_tx2);
    memory_pool->TryAdd(test_tx3);
    EXPECT_EQ(memory_pool->GetCount(), 3);
    
    memory_pool->Clear();
    EXPECT_EQ(memory_pool->GetCount(), 0);
    EXPECT_TRUE(memory_pool->IsEmpty());
}

TEST_F(MemoryPoolTest, GetAllTransactions) {
    memory_pool->TryAdd(test_tx1);
    memory_pool->TryAdd(test_tx2);
    
    auto all_transactions = memory_pool->GetAllTransactions();
    EXPECT_EQ(all_transactions.size(), 2);
    
    // Check that our transactions are in the result
    bool found_tx1 = false, found_tx2 = false;
    for (const auto& tx : all_transactions) {
        if (tx->GetHash() == test_tx1->GetHash()) found_tx1 = true;
        if (tx->GetHash() == test_tx2->GetHash()) found_tx2 = true;
    }
    EXPECT_TRUE(found_tx1);
    EXPECT_TRUE(found_tx2);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_LEDGER_TEST_MEMORYPOOL_CPP_H
