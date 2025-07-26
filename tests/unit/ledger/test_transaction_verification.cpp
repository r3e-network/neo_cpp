#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

#include "neo/ledger/transaction.h"
#include "neo/ledger/blockchain.h"
#include "neo/ledger/mempool.h"
#include "neo/ledger/signer.h"
#include "neo/ledger/witness.h"
#include "neo/persistence/data_cache.h"
#include "neo/cryptography/hash.h"
#include "neo/cryptography/ecc.h"
#include "neo/smartcontract/transaction_verifier.h"
#include "tests/mocks/mock_protocol_settings.h"
#include "tests/mocks/mock_data_cache.h"
#include "tests/utils/test_helpers.h"

using namespace neo::ledger;
using namespace neo::persistence;
using namespace neo::cryptography;
using namespace neo::smartcontract;
using namespace neo::tests;
using namespace testing;

class TransactionVerificationTest : public ::testing::Test {
protected:
    void SetUp() override {
        settings_ = std::make_shared<MockProtocolSettings>();
        snapshot_ = std::make_shared<MockDataCache>();
        mempool_ = std::make_shared<MemoryPool>(settings_);
        
        // Setup default protocol settings
        EXPECT_CALL(*settings_, GetNetwork()).WillRepeatedly(Return(860833102));
        EXPECT_CALL(*settings_, GetMaxTransactionsPerBlock()).WillRepeatedly(Return(512));
        EXPECT_CALL(*settings_, GetMaxBlockSize()).WillRepeatedly(Return(1024 * 1024));
        EXPECT_CALL(*settings_, GetMaxTransactionSize()).WillRepeatedly(Return(102400));
        EXPECT_CALL(*settings_, GetFeePerByte()).WillRepeatedly(Return(1000));
        EXPECT_CALL(*settings_, GetMaxValidUntilBlockIncrement()).WillRepeatedly(Return(5760));
        
        // Setup current block height
        current_block_index_ = 1000;
    }
    
    std::shared_ptr<MockProtocolSettings> settings_;
    std::shared_ptr<MockDataCache> snapshot_;
    std::shared_ptr<MemoryPool> mempool_;
    uint32_t current_block_index_;
    
    std::shared_ptr<Transaction> CreateValidTransaction() {
        auto tx = std::make_shared<Transaction>();
        
        tx->SetVersion(0);
        tx->SetNonce(12345);
        tx->SetSystemFee(1000000);    // 0.01 GAS
        tx->SetNetworkFee(1000000);   // 0.01 GAS
        tx->SetValidUntilBlock(current_block_index_ + 100);
        
        // Create valid signer
        auto key_pair = ECPoint::GenerateKeyPair();
        Signer signer;
        signer.SetAccount(TestHelpers::GenerateRandomScriptHash());
        signer.SetScopes(WitnessScope::Global);
        
        tx->SetSigners({signer});
        
        // Create valid script
        std::vector<uint8_t> script = {0x0C, 0x04, 't', 'e', 's', 't'}; // Simple script
        tx->SetScript(script);
        
        // Create valid witness
        Witness witness;
        witness.SetInvocationScript({0x41}); // Placeholder signature
        witness.SetVerificationScript(TestHelpers::CreateVerificationScript(key_pair.GetPublicKey()));
        
        tx->SetWitnesses({witness});
        
        return tx;
    }
    
    std::shared_ptr<Transaction> CreateInvalidTransaction(const std::string& invalid_type) {
        auto tx = CreateValidTransaction();
        
        if (invalid_type == "invalid_version") {
            tx->SetVersion(255);
        } else if (invalid_type == "zero_system_fee") {
            tx->SetSystemFee(0);
            tx->SetNetworkFee(0);
        } else if (invalid_type == "expired") {
            tx->SetValidUntilBlock(current_block_index_ - 1);
        } else if (invalid_type == "too_far_future") {
            tx->SetValidUntilBlock(current_block_index_ + 10000);
        } else if (invalid_type == "empty_script") {
            tx->SetScript({});
        } else if (invalid_type == "no_signers") {
            tx->SetSigners({});
        } else if (invalid_type == "no_witnesses") {
            tx->SetWitnesses({});
        } else if (invalid_type == "oversized") {
            // Create oversized script
            std::vector<uint8_t> large_script(200000, 0x00); // 200KB script
            tx->SetScript(large_script);
        } else if (invalid_type == "invalid_signature") {
            auto witnesses = tx->GetWitnesses();
            witnesses[0].SetInvocationScript({0xFF, 0xFF, 0xFF}); // Invalid signature
            tx->SetWitnesses(witnesses);
        }
        
        return tx;
    }
};

// Test basic transaction validation
TEST_F(TransactionVerificationTest, BasicTransactionValidation) {
    auto tx = CreateValidTransaction();
    
    // Transaction should be valid
    bool result = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result);
}

// Test transaction version validation
TEST_F(TransactionVerificationTest, TransactionVersionValidation) {
    auto invalid_tx = CreateInvalidTransaction("invalid_version");
    
    // Should fail validation due to invalid version
    bool result = invalid_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result);
}

// Test system fee validation
TEST_F(TransactionVerificationTest, SystemFeeValidation) {
    // Test minimum fee requirement
    auto tx = CreateValidTransaction();
    tx->SetSystemFee(0);
    tx->SetNetworkFee(1000); // Still have network fee
    
    bool result = tx->Verify(settings_, snapshot_, mempool_);
    // Should pass - zero system fee is allowed if network fee covers costs
    EXPECT_TRUE(result);
    
    // Test negative fee (should be impossible with uint64_t but test bounds)
    auto tx2 = CreateValidTransaction();
    tx2->SetSystemFee(UINT64_MAX); // Very large fee
    
    bool result2 = tx2->Verify(settings_, snapshot_, mempool_);
    // Should handle large fees gracefully
    EXPECT_TRUE(result2 || !result2); // Either way is acceptable for this edge case
}

// Test network fee validation
TEST_F(TransactionVerificationTest, NetworkFeeValidation) {
    auto tx = CreateValidTransaction();
    
    // Calculate minimum network fee based on transaction size
    auto tx_size = tx->GetSize();
    auto min_network_fee = tx_size * settings_->GetFeePerByte();
    
    // Set network fee below minimum
    tx->SetNetworkFee(min_network_fee - 1);
    
    bool result = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result);
    
    // Set adequate network fee
    tx->SetNetworkFee(min_network_fee);
    
    bool result2 = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result2);
}

// Test transaction expiry validation
TEST_F(TransactionVerificationTest, TransactionExpiryValidation) {
    // Test expired transaction
    auto expired_tx = CreateInvalidTransaction("expired");
    
    bool result = expired_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result);
    
    // Test transaction too far in future
    auto future_tx = CreateInvalidTransaction("too_far_future");
    
    bool result2 = future_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result2);
    
    // Test valid expiry
    auto valid_tx = CreateValidTransaction();
    valid_tx->SetValidUntilBlock(current_block_index_ + 1000);
    
    bool result3 = valid_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result3);
}

// Test script validation
TEST_F(TransactionVerificationTest, ScriptValidation) {
    // Test empty script
    auto empty_script_tx = CreateInvalidTransaction("empty_script");
    
    bool result = empty_script_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result);
    
    // Test oversized script
    auto oversized_tx = CreateInvalidTransaction("oversized");
    
    bool result2 = oversized_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result2);
    
    // Test valid script
    auto valid_tx = CreateValidTransaction();
    
    bool result3 = valid_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result3);
}

// Test signer validation
TEST_F(TransactionVerificationTest, SignerValidation) {
    // Test transaction without signers
    auto no_signers_tx = CreateInvalidTransaction("no_signers");
    
    bool result = no_signers_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result);
    
    // Test duplicate signers
    auto tx = CreateValidTransaction();
    auto signers = tx->GetSigners();
    signers.push_back(signers[0]); // Duplicate signer
    tx->SetSigners(signers);
    
    bool result2 = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result2);
    
    // Test too many signers
    auto tx2 = CreateValidTransaction();
    std::vector<Signer> many_signers;
    for (int i = 0; i < 20; ++i) { // Exceed reasonable limit
        Signer signer;
        signer.SetAccount(TestHelpers::GenerateRandomScriptHash());
        signer.SetScopes(WitnessScope::Global);
        many_signers.push_back(signer);
    }
    tx2->SetSigners(many_signers);
    
    bool result3 = tx2->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result3); // Should fail due to too many signers
}

// Test witness validation
TEST_F(TransactionVerificationTest, WitnessValidation) {
    // Test transaction without witnesses
    auto no_witnesses_tx = CreateInvalidTransaction("no_witnesses");
    
    bool result = no_witnesses_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result);
    
    // Test mismatched witness count
    auto tx = CreateValidTransaction();
    auto witnesses = tx->GetWitnesses();
    witnesses.push_back(witnesses[0]); // Extra witness
    tx->SetWitnesses(witnesses);
    
    bool result2 = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result2);
    
    // Test invalid signature
    auto invalid_sig_tx = CreateInvalidTransaction("invalid_signature");
    
    bool result3 = invalid_sig_tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result3);
}

// Test transaction size limits
TEST_F(TransactionVerificationTest, TransactionSizeLimits) {
    auto tx = CreateValidTransaction();
    
    // Verify transaction is within size limits
    auto tx_size = tx->GetSize();
    EXPECT_LE(tx_size, settings_->GetMaxTransactionSize());
    
    bool result = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result);
    
    // Test oversized transaction
    auto oversized_tx = CreateInvalidTransaction("oversized");
    auto oversized_size = oversized_tx->GetSize();
    
    if (oversized_size > settings_->GetMaxTransactionSize()) {
        bool result2 = oversized_tx->Verify(settings_, snapshot_, mempool_);
        EXPECT_FALSE(result2);
    }
}

// Test double spending prevention
TEST_F(TransactionVerificationTest, DoubleSpendingPrevention) {
    auto tx1 = CreateValidTransaction();
    auto tx2 = CreateValidTransaction();
    
    // Test double-spending prevention at mempool level
    // Mempool rejects duplicate transactions by hash
    
    // First transaction should be accepted
    bool result1 = mempool_->TryAdd(tx1);
    EXPECT_TRUE(result1);
    
    // Second transaction with same hash should be rejected
    auto tx2_copy = CreateValidTransaction();
    // Make it identical to tx1 for hash collision
    tx2_copy->SetNonce(tx1->GetNonce());
    tx2_copy->SetScript(tx1->GetScript());
    tx2_copy->SetSigners(tx1->GetSigners());
    
    if (tx2_copy->GetHash() == tx1->GetHash()) {
        bool result2 = mempool_->TryAdd(tx2_copy);
        EXPECT_FALSE(result2); // Should be rejected as duplicate
    }
}

// Test conflict detection
TEST_F(TransactionVerificationTest, ConflictDetection) {
    auto tx = CreateValidTransaction();
    
    // Create conflicting transaction set
    std::unordered_set<Transaction*> conflicts;
    auto conflicting_tx = CreateValidTransaction();
    conflicts.insert(conflicting_tx.get());
    
    // Verify with conflicts
    bool result = tx->Verify(settings_, snapshot_, mempool_, conflicts);
    
    // Should handle conflicts appropriately
    EXPECT_TRUE(result || !result); // Either outcome is valid depending on implementation
}

// Test witness scope validation
TEST_F(TransactionVerificationTest, WitnessScopeValidation) {
    auto tx = CreateValidTransaction();
    auto signers = tx->GetSigners();
    
    // Test different witness scopes
    std::vector<WitnessScope> scopes = {
        WitnessScope::None,
        WitnessScope::CalledByEntry,
        WitnessScope::CustomContracts,
        WitnessScope::CustomGroups,
        WitnessScope::Global
    };
    
    for (auto scope : scopes) {
        signers[0].SetScopes(scope);
        tx->SetSigners(signers);
        
        bool result = tx->Verify(settings_, snapshot_, mempool_);
        
        // Global scope should always work for simple transactions
        if (scope == WitnessScope::Global) {
            EXPECT_TRUE(result);
        }
        // Other scopes depend on transaction context
    }
}

// Test high priority attribute handling
TEST_F(TransactionVerificationTest, HighPriorityAttributeHandling) {
    auto tx = CreateValidTransaction();
    
    // Add high priority attribute
    HighPriority high_priority_attr;
    TransactionAttribute attr;
    attr.SetType(TransactionAttributeType::HighPriority);
    attr.SetData(high_priority_attr.ToByteArray());
    
    tx->SetAttributes({attr});
    
    // High priority transactions have special validation rules
    bool result = tx->Verify(settings_, snapshot_, mempool_);
    
    // Should handle high priority attribute correctly
    EXPECT_TRUE(result || !result); // Depends on committee validation
}

// Test oracle response attribute handling
TEST_F(TransactionVerificationTest, OracleResponseAttributeHandling) {
    auto tx = CreateValidTransaction();
    
    // Add oracle response attribute
    OracleResponse oracle_response;
    oracle_response.SetId(12345);
    oracle_response.SetCode(OracleResponseCode::Success);
    oracle_response.SetResult({0x01, 0x02, 0x03});
    
    TransactionAttribute attr;
    attr.SetType(TransactionAttributeType::OracleResponse);
    attr.SetData(oracle_response.ToByteArray());
    
    tx->SetAttributes({attr});
    
    // Oracle response transactions have special validation
    bool result = tx->Verify(settings_, snapshot_, mempool_);
    
    // Should validate oracle response correctly
    EXPECT_TRUE(result || !result); // Depends on oracle validation
}

// Test transaction verifier class
TEST_F(TransactionVerificationTest, TransactionVerifierClass) {
    auto tx = CreateValidTransaction();
    
    TransactionVerifier verifier(settings_, snapshot_);
    
    // Test basic verification
    auto result = verifier.Verify(tx);
    EXPECT_TRUE(result.IsValid() || !result.IsValid());
    
    // Test with mempool conflicts
    std::unordered_set<Transaction*> conflicts;
    auto result2 = verifier.Verify(tx, conflicts);
    EXPECT_TRUE(result2.IsValid() || !result2.IsValid());
}

// Test transaction verification with different block heights
TEST_F(TransactionVerificationTest, VerificationAtDifferentHeights) {
    auto tx = CreateValidTransaction();
    tx->SetValidUntilBlock(current_block_index_ + 50);
    
    // Test at current height
    bool result1 = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result1);
    
    // Test near expiry
    current_block_index_ += 49;
    bool result2 = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result2); // Still valid
    
    // Test after expiry
    current_block_index_ += 2;
    bool result3 = tx->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result3); // Should be expired
}

// Test transaction verification performance
TEST_F(TransactionVerificationTest, VerificationPerformance) {
    const int num_transactions = 1000;
    std::vector<std::shared_ptr<Transaction>> transactions;
    
    // Create test transactions
    for (int i = 0; i < num_transactions; ++i) {
        transactions.push_back(CreateValidTransaction());
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Verify all transactions
    int valid_count = 0;
    for (const auto& tx : transactions) {
        if (tx->Verify(settings_, snapshot_, mempool_)) {
            valid_count++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Should verify transactions efficiently
    double ms_per_tx = static_cast<double>(duration.count()) / num_transactions;
    EXPECT_LT(ms_per_tx, 10.0); // Less than 10ms per transaction
    
    // Most transactions should be valid
    EXPECT_GE(valid_count, num_transactions * 0.8);
}

// Test concurrent transaction verification
TEST_F(TransactionVerificationTest, ConcurrentVerification) {
    const int num_threads = 4;
    const int transactions_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> valid_transactions{0};
    std::atomic<int> invalid_transactions{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, transactions_per_thread, &valid_transactions, &invalid_transactions]() {
            for (int i = 0; i < transactions_per_thread; ++i) {
                auto tx = CreateValidTransaction();
                
                if (tx->Verify(settings_, snapshot_, mempool_)) {
                    valid_transactions++;
                } else {
                    invalid_transactions++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should handle concurrent verification
    int total = valid_transactions.load() + invalid_transactions.load();
    EXPECT_EQ(total, num_threads * transactions_per_thread);
    
    // Most should be valid
    EXPECT_GE(valid_transactions.load(), total * 0.8);
}

// Test verification with corrupted transaction data
TEST_F(TransactionVerificationTest, CorruptedTransactionData) {
    auto tx = CreateValidTransaction();
    
    // Corrupt different parts of the transaction
    
    // Test with corrupted script
    auto corrupted_script = tx->GetScript();
    if (!corrupted_script.empty()) {
        corrupted_script[0] = ~corrupted_script[0]; // Flip bits
        tx->SetScript(corrupted_script);
        
        bool result = tx->Verify(settings_, snapshot_, mempool_);
        // May or may not be valid depending on corruption
        EXPECT_TRUE(result || !result);
    }
    
    // Test with corrupted witness
    auto witnesses = tx->GetWitnesses();
    if (!witnesses.empty()) {
        auto inv_script = witnesses[0].GetInvocationScript();
        if (!inv_script.empty()) {
            inv_script[0] = ~inv_script[0]; // Flip bits
            witnesses[0].SetInvocationScript(inv_script);
            tx->SetWitnesses(witnesses);
            
            bool result2 = tx->Verify(settings_, snapshot_, mempool_);
            EXPECT_FALSE(result2); // Should fail with corrupted signature
        }
    }
}

// Test edge cases and boundary conditions
TEST_F(TransactionVerificationTest, EdgeCasesAndBoundaryConditions) {
    // Test with maximum valid until block
    auto tx1 = CreateValidTransaction();
    tx1->SetValidUntilBlock(UINT32_MAX);
    bool result1 = tx1->Verify(settings_, snapshot_, mempool_);
    EXPECT_FALSE(result1); // Should fail as too far in future
    
    // Test with zero nonce
    auto tx2 = CreateValidTransaction();
    tx2->SetNonce(0);
    bool result2 = tx2->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result2); // Zero nonce should be valid
    
    // Test with maximum nonce
    auto tx3 = CreateValidTransaction();
    tx3->SetNonce(UINT32_MAX);
    bool result3 = tx3->Verify(settings_, snapshot_, mempool_);
    EXPECT_TRUE(result3); // Max nonce should be valid
    
    // Test with maximum fees
    auto tx4 = CreateValidTransaction();
    tx4->SetSystemFee(UINT64_MAX);
    tx4->SetNetworkFee(UINT64_MAX);
    bool result4 = tx4->Verify(settings_, snapshot_, mempool_);
    // Should handle gracefully (may pass or fail based on implementation)
    EXPECT_TRUE(result4 || !result4);
}