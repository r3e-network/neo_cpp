// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_transactionstate.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_TRANSACTIONSTATE_CPP_H
#define TESTS_UNIT_LEDGER_TEST_TRANSACTIONSTATE_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/ledger/transaction_state.h>

namespace neo {
namespace test {

class TransactionStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for TransactionState testing
        test_hash = io::UInt256::Parse("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        block_index = 12345;
        tx_index = 42;
        
        // Create test transaction
        test_transaction = std::make_shared<ledger::Transaction>();
        test_transaction->SetHash(test_hash);
        test_transaction->SetNetworkFee(1000000);
        test_transaction->SetSystemFee(2000000);
        
        // Create TransactionState instances
        confirmed_state = std::make_shared<ledger::TransactionState>(test_transaction, block_index, tx_index);
        unconfirmed_state = std::make_shared<ledger::TransactionState>(test_transaction);
    }

    void TearDown() override {
        // Clean up test fixtures
        test_transaction.reset();
        confirmed_state.reset();
        unconfirmed_state.reset();
    }

    // Helper methods and test data for TransactionState testing
    io::UInt256 test_hash;
    uint32_t block_index;
    uint32_t tx_index;
    std::shared_ptr<ledger::Transaction> test_transaction;
    std::shared_ptr<ledger::TransactionState> confirmed_state;
    std::shared_ptr<ledger::TransactionState> unconfirmed_state;
};

// TransactionState test methods converted from C# UT_TransactionState.cs functionality

TEST_F(TransactionStateTest, ConstructorWithConfirmedTransaction) {
    EXPECT_EQ(confirmed_state->GetTransaction(), test_transaction);
    EXPECT_EQ(confirmed_state->GetBlockIndex(), block_index);
    EXPECT_EQ(confirmed_state->GetTransactionIndex(), tx_index);
    EXPECT_TRUE(confirmed_state->IsConfirmed());
}

TEST_F(TransactionStateTest, ConstructorWithUnconfirmedTransaction) {
    EXPECT_EQ(unconfirmed_state->GetTransaction(), test_transaction);
    EXPECT_FALSE(unconfirmed_state->IsConfirmed());
    // Block index and tx index should be invalid for unconfirmed
    EXPECT_EQ(unconfirmed_state->GetBlockIndex(), std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(unconfirmed_state->GetTransactionIndex(), std::numeric_limits<uint32_t>::max());
}

TEST_F(TransactionStateTest, GetTransactionHash) {
    EXPECT_EQ(confirmed_state->GetHash(), test_hash);
    EXPECT_EQ(unconfirmed_state->GetHash(), test_hash);
}

TEST_F(TransactionStateTest, ConfirmTransaction) {
    // Start with unconfirmed state
    EXPECT_FALSE(unconfirmed_state->IsConfirmed());
    
    // Confirm it
    unconfirmed_state->Confirm(block_index, tx_index);
    
    EXPECT_TRUE(unconfirmed_state->IsConfirmed());
    EXPECT_EQ(unconfirmed_state->GetBlockIndex(), block_index);
    EXPECT_EQ(unconfirmed_state->GetTransactionIndex(), tx_index);
}

TEST_F(TransactionStateTest, Serialization) {
    // Test serialization of confirmed state
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    confirmed_state->Serialize(writer);
    
    // Test deserialization
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized_state = ledger::TransactionState::Deserialize(reader);
    
    EXPECT_EQ(deserialized_state.GetHash(), test_hash);
    EXPECT_EQ(deserialized_state.GetBlockIndex(), block_index);
    EXPECT_EQ(deserialized_state.GetTransactionIndex(), tx_index);
    EXPECT_TRUE(deserialized_state.IsConfirmed());
}

TEST_F(TransactionStateTest, SerializationUnconfirmed) {
    // Test serialization of unconfirmed state
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    unconfirmed_state->Serialize(writer);
    
    // Test deserialization
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized_state = ledger::TransactionState::Deserialize(reader);
    
    EXPECT_EQ(deserialized_state.GetHash(), test_hash);
    EXPECT_FALSE(deserialized_state.IsConfirmed());
}

TEST_F(TransactionStateTest, Clone) {
    auto cloned_state = confirmed_state->Clone();
    
    EXPECT_NE(cloned_state.get(), confirmed_state.get()); // Different objects  
    EXPECT_EQ(cloned_state->GetHash(), confirmed_state->GetHash());
    EXPECT_EQ(cloned_state->GetBlockIndex(), confirmed_state->GetBlockIndex());
    EXPECT_EQ(cloned_state->GetTransactionIndex(), confirmed_state->GetTransactionIndex());
    EXPECT_EQ(cloned_state->IsConfirmed(), confirmed_state->IsConfirmed());
}

TEST_F(TransactionStateTest, EqualityComparison) {
    auto same_state = std::make_shared<ledger::TransactionState>(test_transaction, block_index, tx_index);
    auto different_block = std::make_shared<ledger::TransactionState>(test_transaction, block_index + 1, tx_index);
    
    EXPECT_TRUE(*confirmed_state == *same_state);
    EXPECT_FALSE(*confirmed_state == *different_block);
    EXPECT_FALSE(*confirmed_state == *unconfirmed_state);
}

TEST_F(TransactionStateTest, GetSize) {
    size_t confirmed_size = confirmed_state->GetSize();
    size_t unconfirmed_size = unconfirmed_state->GetSize();
    
    EXPECT_GT(confirmed_size, 0);
    EXPECT_GT(unconfirmed_size, 0);
    // Confirmed state should be larger due to block/tx index storage
    EXPECT_GE(confirmed_size, unconfirmed_size);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_LEDGER_TEST_TRANSACTIONSTATE_CPP_H
