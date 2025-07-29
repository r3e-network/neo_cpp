// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_poolitem.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_POOLITEM_CPP_H
#define TESTS_UNIT_LEDGER_TEST_POOLITEM_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/ledger/pool_item.h>

namespace neo
{
namespace test
{

class PoolItemTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for PoolItem testing
        timestamp = std::time(nullptr);

        // Create test transactions with different priorities
        high_priority_tx = CreateTestTransaction(
            io::UInt256::Parse("1111111111111111111111111111111111111111111111111111111111111111"), 5000000,
            10000000  // High fees
        );

        medium_priority_tx = CreateTestTransaction(
            io::UInt256::Parse("2222222222222222222222222222222222222222222222222222222222222222"), 2000000,
            4000000  // Medium fees
        );

        low_priority_tx = CreateTestTransaction(
            io::UInt256::Parse("3333333333333333333333333333333333333333333333333333333333333333"), 100000,
            200000  // Low fees
        );

        // Create pool items
        high_priority_item = std::make_shared<ledger::PoolItem>(high_priority_tx, timestamp);
        medium_priority_item = std::make_shared<ledger::PoolItem>(medium_priority_tx, timestamp + 1);
        low_priority_item = std::make_shared<ledger::PoolItem>(low_priority_tx, timestamp + 2);
    }

    void TearDown() override
    {
        // Clean up test fixtures
        high_priority_tx.reset();
        medium_priority_tx.reset();
        low_priority_tx.reset();
        high_priority_item.reset();
        medium_priority_item.reset();
        low_priority_item.reset();
    }

    // Helper methods and test data for PoolItem testing
    std::time_t timestamp;
    std::shared_ptr<ledger::Transaction> high_priority_tx;
    std::shared_ptr<ledger::Transaction> medium_priority_tx;
    std::shared_ptr<ledger::Transaction> low_priority_tx;
    std::shared_ptr<ledger::PoolItem> high_priority_item;
    std::shared_ptr<ledger::PoolItem> medium_priority_item;
    std::shared_ptr<ledger::PoolItem> low_priority_item;

    // Helper to create test transactions
    std::shared_ptr<ledger::Transaction> CreateTestTransaction(const io::UInt256& hash, int64_t network_fee,
                                                               int64_t system_fee)
    {
        auto tx = std::make_shared<ledger::Transaction>();
        tx->SetHash(hash);
        tx->SetNetworkFee(network_fee);
        tx->SetSystemFee(system_fee);
        tx->SetValidUntilBlock(1000);
        return tx;
    }
};

// PoolItem test methods converted from C# UT_PoolItem.cs functionality

TEST_F(PoolItemTest, ConstructorWithTransactionAndTimestamp)
{
    EXPECT_EQ(high_priority_item->GetTransaction(), high_priority_tx);
    EXPECT_EQ(high_priority_item->GetTimestamp(), timestamp);
    EXPECT_EQ(high_priority_item->GetHash(), high_priority_tx->GetHash());
}

TEST_F(PoolItemTest, GetNetworkFee)
{
    EXPECT_EQ(high_priority_item->GetNetworkFee(), 5000000);
    EXPECT_EQ(medium_priority_item->GetNetworkFee(), 2000000);
    EXPECT_EQ(low_priority_item->GetNetworkFee(), 100000);
}

TEST_F(PoolItemTest, GetSystemFee)
{
    EXPECT_EQ(high_priority_item->GetSystemFee(), 10000000);
    EXPECT_EQ(medium_priority_item->GetSystemFee(), 4000000);
    EXPECT_EQ(low_priority_item->GetSystemFee(), 200000);
}

TEST_F(PoolItemTest, GetTotalFee)
{
    EXPECT_EQ(high_priority_item->GetTotalFee(), 15000000);   // 5M + 10M
    EXPECT_EQ(medium_priority_item->GetTotalFee(), 6000000);  // 2M + 4M
    EXPECT_EQ(low_priority_item->GetTotalFee(), 300000);      // 100K + 200K
}

TEST_F(PoolItemTest, GetFeePerByte)
{
    // Assuming transaction size estimation
    double high_fee_per_byte = high_priority_item->GetFeePerByte();
    double medium_fee_per_byte = medium_priority_item->GetFeePerByte();
    double low_fee_per_byte = low_priority_item->GetFeePerByte();

    EXPECT_GT(high_fee_per_byte, medium_fee_per_byte);
    EXPECT_GT(medium_fee_per_byte, low_fee_per_byte);
}

TEST_F(PoolItemTest, PriorityComparison)
{
    // Higher fee items should have higher priority
    EXPECT_TRUE(high_priority_item->GetPriority() > medium_priority_item->GetPriority());
    EXPECT_TRUE(medium_priority_item->GetPriority() > low_priority_item->GetPriority());
}

TEST_F(PoolItemTest, TimestampComparison)
{
    // Earlier timestamps should have higher priority when fees are equal
    auto same_fee_tx1 = CreateTestTransaction(
        io::UInt256::Parse("4444444444444444444444444444444444444444444444444444444444444444"), 1000000, 2000000);
    auto same_fee_tx2 = CreateTestTransaction(
        io::UInt256::Parse("5555555555555555555555555555555555555555555555555555555555555555"), 1000000, 2000000);

    auto earlier_item = std::make_shared<ledger::PoolItem>(same_fee_tx1, timestamp);
    auto later_item = std::make_shared<ledger::PoolItem>(same_fee_tx2, timestamp + 10);

    EXPECT_GE(earlier_item->GetPriority(), later_item->GetPriority());
}

TEST_F(PoolItemTest, EqualityComparison)
{
    auto same_item = std::make_shared<ledger::PoolItem>(high_priority_tx, timestamp);
    auto different_item = std::make_shared<ledger::PoolItem>(medium_priority_tx, timestamp);

    EXPECT_TRUE(*high_priority_item == *same_item);
    EXPECT_FALSE(*high_priority_item == *different_item);
}

TEST_F(PoolItemTest, GetHashCode)
{
    auto hash1 = high_priority_item->GetHashCode();
    auto same_item = std::make_shared<ledger::PoolItem>(high_priority_tx, timestamp);
    auto hash2 = same_item->GetHashCode();

    EXPECT_EQ(hash1, hash2);  // Same items should have same hash

    auto hash3 = medium_priority_item->GetHashCode();
    EXPECT_NE(hash1, hash3);  // Different items should have different hashes
}

TEST_F(PoolItemTest, GetSize)
{
    size_t size = high_priority_item->GetSize();
    EXPECT_GT(size, 0);

    // Size should include transaction size plus metadata
    size_t tx_size = high_priority_tx->GetSize();
    EXPECT_GE(size, tx_size);
}

TEST_F(PoolItemTest, IsExpired)
{
    uint32_t current_block_height = 500;

    // Transaction valid until block 1000, current block 500
    EXPECT_FALSE(high_priority_item->IsExpired(current_block_height));

    // Test with height beyond valid until block
    uint32_t future_block_height = 1500;
    EXPECT_TRUE(high_priority_item->IsExpired(future_block_height));
}

TEST_F(PoolItemTest, GetAge)
{
    std::time_t current_time = timestamp + 60;  // 60 seconds later
    std::time_t age = high_priority_item->GetAge(current_time);
    EXPECT_EQ(age, 60);
}

TEST_F(PoolItemTest, SortByPriority)
{
    std::vector<std::shared_ptr<ledger::PoolItem>> items = {low_priority_item, high_priority_item,
                                                            medium_priority_item};

    // Sort by priority (highest first)
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b) { return a->GetPriority() > b->GetPriority(); });

    EXPECT_EQ(items[0], high_priority_item);
    EXPECT_EQ(items[1], medium_priority_item);
    EXPECT_EQ(items[2], low_priority_item);
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_LEDGER_TEST_POOLITEM_CPP_H
