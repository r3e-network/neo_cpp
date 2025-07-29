// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_transactionverificationcontext.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_TRANSACTIONVERIFICATIONCONTEXT_CPP_H
#define TESTS_UNIT_LEDGER_TEST_TRANSACTIONVERIFICATIONCONTEXT_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/ledger/transaction_verification_context.h>

namespace neo
{
namespace test
{

class TransactionVerificationContextTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for TransactionVerificationContext testing
        protocol_settings = std::make_shared<ProtocolSettings>();
        protocol_settings->SetNetwork(0x860833102);
        protocol_settings->SetMaxTransactionsPerBlock(512);

        // Create verification context
        verification_context = std::make_shared<ledger::TransactionVerificationContext>(protocol_settings);

        // Create test transactions
        test_tx1 = CreateTestTransaction(
            io::UInt256::Parse("1111111111111111111111111111111111111111111111111111111111111111"), 1000000, 2000000);
        test_tx2 = CreateTestTransaction(
            io::UInt256::Parse("2222222222222222222222222222222222222222222222222222222222222222"), 1500000, 2500000);
        test_tx3 = CreateTestTransaction(
            io::UInt256::Parse("3333333333333333333333333333333333333333333333333333333333333333"), 800000, 1800000);

        // Conflicting transaction (same signer)
        conflicting_tx = CreateTestTransaction(
            io::UInt256::Parse("4444444444444444444444444444444444444444444444444444444444444444"), 2000000, 3000000);
    }

    void TearDown() override
    {
        // Clean up test fixtures
        verification_context.reset();
        protocol_settings.reset();
        test_tx1.reset();
        test_tx2.reset();
        test_tx3.reset();
        conflicting_tx.reset();
    }

    // Helper methods and test data for TransactionVerificationContext testing
    std::shared_ptr<ProtocolSettings> protocol_settings;
    std::shared_ptr<ledger::TransactionVerificationContext> verification_context;
    std::shared_ptr<ledger::Transaction> test_tx1;
    std::shared_ptr<ledger::Transaction> test_tx2;
    std::shared_ptr<ledger::Transaction> test_tx3;
    std::shared_ptr<ledger::Transaction> conflicting_tx;

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

// TransactionVerificationContext test methods converted from C# UT_TransactionVerificationContext.cs functionality

TEST_F(TransactionVerificationContextTest, ConstructorCreatesEmptyContext)
{
    EXPECT_EQ(verification_context->GetTransactionCount(), 0);
    EXPECT_TRUE(verification_context->IsEmpty());
}

TEST_F(TransactionVerificationContextTest, AddTransactionToContext)
{
    bool added = verification_context->AddTransaction(test_tx1);
    EXPECT_TRUE(added);
    EXPECT_EQ(verification_context->GetTransactionCount(), 1);
    EXPECT_FALSE(verification_context->IsEmpty());
}

TEST_F(TransactionVerificationContextTest, AddDuplicateTransaction)
{
    EXPECT_TRUE(verification_context->AddTransaction(test_tx1));
    EXPECT_EQ(verification_context->GetTransactionCount(), 1);

    // Adding same transaction again should fail
    EXPECT_FALSE(verification_context->AddTransaction(test_tx1));
    EXPECT_EQ(verification_context->GetTransactionCount(), 1);  // Count unchanged
}

TEST_F(TransactionVerificationContextTest, AddMultipleTransactions)
{
    EXPECT_TRUE(verification_context->AddTransaction(test_tx1));
    EXPECT_TRUE(verification_context->AddTransaction(test_tx2));
    EXPECT_TRUE(verification_context->AddTransaction(test_tx3));

    EXPECT_EQ(verification_context->GetTransactionCount(), 3);
}

TEST_F(TransactionVerificationContextTest, CheckConflicts)
{
    verification_context->AddTransaction(test_tx1);

    // Non-conflicting transaction should be allowed
    bool conflict1 = verification_context->CheckConflicts(test_tx2);
    EXPECT_FALSE(conflict1);

    // Same transaction should conflict
    bool conflict2 = verification_context->CheckConflicts(test_tx1);
    EXPECT_TRUE(conflict2);
}

TEST_F(TransactionVerificationContextTest, RemoveTransaction)
{
    verification_context->AddTransaction(test_tx1);
    verification_context->AddTransaction(test_tx2);
    EXPECT_EQ(verification_context->GetTransactionCount(), 2);

    bool removed = verification_context->RemoveTransaction(test_tx1->GetHash());
    EXPECT_TRUE(removed);
    EXPECT_EQ(verification_context->GetTransactionCount(), 1);
}

TEST_F(TransactionVerificationContextTest, RemoveNonExistentTransaction)
{
    verification_context->AddTransaction(test_tx1);

    bool removed = verification_context->RemoveTransaction(test_tx2->GetHash());
    EXPECT_FALSE(removed);
    EXPECT_EQ(verification_context->GetTransactionCount(), 1);  // Count unchanged
}

TEST_F(TransactionVerificationContextTest, ContainsTransaction)
{
    verification_context->AddTransaction(test_tx1);

    EXPECT_TRUE(verification_context->Contains(test_tx1->GetHash()));
    EXPECT_FALSE(verification_context->Contains(test_tx2->GetHash()));
}

TEST_F(TransactionVerificationContextTest, GetTotalNetworkFee)
{
    verification_context->AddTransaction(test_tx1);  // 1000000
    verification_context->AddTransaction(test_tx2);  // 1500000

    int64_t total_fee = verification_context->GetTotalNetworkFee();
    EXPECT_EQ(total_fee, 2500000);  // 1000000 + 1500000
}

TEST_F(TransactionVerificationContextTest, GetTotalSystemFee)
{
    verification_context->AddTransaction(test_tx1);  // 2000000
    verification_context->AddTransaction(test_tx2);  // 2500000

    int64_t total_fee = verification_context->GetTotalSystemFee();
    EXPECT_EQ(total_fee, 4500000);  // 2000000 + 2500000
}

TEST_F(TransactionVerificationContextTest, ClearContext)
{
    verification_context->AddTransaction(test_tx1);
    verification_context->AddTransaction(test_tx2);
    verification_context->AddTransaction(test_tx3);
    EXPECT_EQ(verification_context->GetTransactionCount(), 3);

    verification_context->Clear();
    EXPECT_EQ(verification_context->GetTransactionCount(), 0);
    EXPECT_TRUE(verification_context->IsEmpty());
}

TEST_F(TransactionVerificationContextTest, GetAllTransactions)
{
    verification_context->AddTransaction(test_tx1);
    verification_context->AddTransaction(test_tx2);

    auto all_transactions = verification_context->GetAllTransactions();
    EXPECT_EQ(all_transactions.size(), 2);

    // Check that our transactions are in the result
    bool found_tx1 = false, found_tx2 = false;
    for (const auto& tx : all_transactions)
    {
        if (tx->GetHash() == test_tx1->GetHash())
            found_tx1 = true;
        if (tx->GetHash() == test_tx2->GetHash())
            found_tx2 = true;
    }
    EXPECT_TRUE(found_tx1);
    EXPECT_TRUE(found_tx2);
}

TEST_F(TransactionVerificationContextTest, MaxTransactionLimit)
{
    // Add transactions up to the limit
    int max_transactions = protocol_settings->GetMaxTransactionsPerBlock();

    // Should be able to add up to the limit
    for (int i = 0; i < std::min(max_transactions, 10); ++i)
    {
        auto tx = CreateTestTransaction(io::UInt256::FromNumber(i + 1000), 1000000, 2000000);
        bool added = verification_context->AddTransaction(tx);
        EXPECT_TRUE(added);
    }

    EXPECT_LE(verification_context->GetTransactionCount(), max_transactions);
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_LEDGER_TEST_TRANSACTIONVERIFICATIONCONTEXT_CPP_H
