/**
 * @file test_transaction_complete.cpp
 * @brief Complete transaction tests for Neo C++
 * Must match Neo C# transaction validation exactly
 */

#include <gtest/gtest.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/witness.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/witness_rule.h>
#include <neo/ledger/witness_condition.h>
#include <neo/ledger/oracle_response.h>
#include <neo/ledger/conflicts_attribute.h>
#include <neo/ledger/not_valid_before.h>
#include <neo/ledger/high_priority_attribute.h>
#include <neo/vm/script_builder.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/key_pair.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <memory>
#include <vector>

using namespace neo::ledger;
using namespace neo::cryptography;
using namespace neo::io;
using namespace neo::vm;

class TransactionCompleteTest : public ::testing::Test {
protected:
    std::unique_ptr<Transaction> tx_;
    std::vector<std::unique_ptr<KeyPair>> keys_;
    
    void SetUp() override {
        tx_ = std::make_unique<Transaction>();
        
        // Generate test keys
        for (int i = 0; i < 5; ++i) {
            keys_.push_back(std::make_unique<KeyPair>());
        }
    }
    
    UInt160 GetScriptHash(const KeyPair& key) {
        ScriptBuilder sb;
        sb.EmitPush(key.GetPublicKey());
        sb.Emit(OpCode::CHECKSIG);
        auto script = sb.ToArray();
        return Crypto::Hash160(script.AsSpan());
    }
    
    Witness CreateWitness(const KeyPair& key, const UInt256& message) {
        Witness witness;
        witness.InvocationScript = CreateInvocationScript(key.Sign(message));
        witness.VerificationScript = CreateVerificationScript(key.GetPublicKey());
        return witness;
    }
    
    ByteVector CreateInvocationScript(const ByteVector& signature) {
        ScriptBuilder sb;
        sb.EmitPush(signature);
        return sb.ToArray();
    }
    
    ByteVector CreateVerificationScript(const ECPoint& publicKey) {
        ScriptBuilder sb;
        sb.EmitPush(publicKey);
        sb.Emit(OpCode::CHECKSIG);
        return sb.ToArray();
    }
};

// ============================================================================
// Transaction Structure Tests
// ============================================================================

TEST_F(TransactionCompleteTest, Transaction_DefaultValues) {
    EXPECT_EQ(tx_->Version, 0);
    EXPECT_EQ(tx_->Nonce, 0);
    EXPECT_EQ(tx_->SystemFee, 0);
    EXPECT_EQ(tx_->NetworkFee, 0);
    EXPECT_EQ(tx_->ValidUntilBlock, 0);
    EXPECT_TRUE(tx_->Signers.empty());
    EXPECT_TRUE(tx_->Attributes.empty());
    EXPECT_TRUE(tx_->Script.Empty());
    EXPECT_TRUE(tx_->Witnesses.empty());
}

TEST_F(TransactionCompleteTest, Transaction_BasicFields) {
    tx_->Version = 0;
    tx_->Nonce = 12345678;
    tx_->SystemFee = 1000000; // 0.01 GAS
    tx_->NetworkFee = 500000; // 0.005 GAS
    tx_->ValidUntilBlock = 10000;
    tx_->Script = ByteVector::FromString("Test script");
    
    EXPECT_EQ(tx_->Version, 0);
    EXPECT_EQ(tx_->Nonce, 12345678);
    EXPECT_EQ(tx_->SystemFee, 1000000);
    EXPECT_EQ(tx_->NetworkFee, 500000);
    EXPECT_EQ(tx_->ValidUntilBlock, 10000);
    EXPECT_FALSE(tx_->Script.Empty());
}

TEST_F(TransactionCompleteTest, Transaction_Hash) {
    tx_->Nonce = 99999;
    tx_->SystemFee = 2000000;
    tx_->NetworkFee = 1000000;
    tx_->ValidUntilBlock = 20000;
    
    auto hash1 = tx_->GetHash();
    
    // Hash should be deterministic
    auto hash2 = tx_->GetHash();
    EXPECT_EQ(hash1, hash2);
    
    // Changing any field should change hash
    tx_->Nonce = 100000;
    auto hash3 = tx_->GetHash();
    EXPECT_NE(hash1, hash3);
}

TEST_F(TransactionCompleteTest, Transaction_Size) {
    tx_->Nonce = 12345;
    tx_->SystemFee = 1000000;
    tx_->NetworkFee = 500000;
    tx_->ValidUntilBlock = 10000;
    tx_->Script = ByteVector(100, 0xAB);
    
    auto size = tx_->GetSize();
    
    // Size should include all fields
    EXPECT_GT(size, 100); // At least script size
    EXPECT_LT(size, 1000); // Reasonable upper bound
}

TEST_F(TransactionCompleteTest, Transaction_Serialization) {
    tx_->Version = 0;
    tx_->Nonce = 87654321;
    tx_->SystemFee = 5000000;
    tx_->NetworkFee = 2500000;
    tx_->ValidUntilBlock = 50000;
    tx_->Script = ByteVector::FromString("Serialization test");
    
    auto serialized = tx_->Serialize();
    
    Transaction deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Version, tx_->Version);
    EXPECT_EQ(deserialized.Nonce, tx_->Nonce);
    EXPECT_EQ(deserialized.SystemFee, tx_->SystemFee);
    EXPECT_EQ(deserialized.NetworkFee, tx_->NetworkFee);
    EXPECT_EQ(deserialized.ValidUntilBlock, tx_->ValidUntilBlock);
    EXPECT_EQ(deserialized.Script, tx_->Script);
}

// ============================================================================
// Signer Tests
// ============================================================================

TEST_F(TransactionCompleteTest, Signer_Creation) {
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    signer.Scopes = WitnessScope::CalledByEntry;
    
    EXPECT_NE(signer.Account, UInt160::Zero());
    EXPECT_EQ(signer.Scopes, WitnessScope::CalledByEntry);
}

TEST_F(TransactionCompleteTest, Signer_Scopes) {
    // Test all witness scope values
    EXPECT_EQ(static_cast<uint8_t>(WitnessScope::None), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(WitnessScope::CalledByEntry), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(WitnessScope::CustomContracts), 0x10);
    EXPECT_EQ(static_cast<uint8_t>(WitnessScope::CustomGroups), 0x20);
    EXPECT_EQ(static_cast<uint8_t>(WitnessScope::WitnessRules), 0x40);
    EXPECT_EQ(static_cast<uint8_t>(WitnessScope::Global), 0x80);
}

TEST_F(TransactionCompleteTest, Signer_AllowedContracts) {
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    signer.Scopes = WitnessScope::CustomContracts;
    
    // Add allowed contracts
    for (int i = 0; i < 5; ++i) {
        UInt160 contract;
        contract.Fill(i);
        signer.AllowedContracts.push_back(contract);
    }
    
    EXPECT_EQ(signer.AllowedContracts.size(), 5);
}

TEST_F(TransactionCompleteTest, Signer_AllowedGroups) {
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    signer.Scopes = WitnessScope::CustomGroups;
    
    // Add allowed groups
    for (int i = 0; i < 3; ++i) {
        ECPoint group;
        group.Fill(0x02 + (i % 2));
        group.Data()[1] = i;
        signer.AllowedGroups.push_back(group);
    }
    
    EXPECT_EQ(signer.AllowedGroups.size(), 3);
}

TEST_F(TransactionCompleteTest, Signer_Rules) {
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    signer.Scopes = WitnessScope::WitnessRules;
    
    // Add witness rules
    WitnessRule rule;
    rule.Action = WitnessRuleAction::Allow;
    rule.Condition = std::make_shared<BooleanCondition>(true);
    signer.Rules.push_back(rule);
    
    EXPECT_EQ(signer.Rules.size(), 1);
    EXPECT_EQ(signer.Rules[0].Action, WitnessRuleAction::Allow);
}

TEST_F(TransactionCompleteTest, Transaction_MultipleSi gners) {
    // Add multiple signers
    for (int i = 0; i < 3; ++i) {
        Signer signer;
        signer.Account = GetScriptHash(*keys_[i]);
        signer.Scopes = WitnessScope::CalledByEntry;
        tx_->Signers.push_back(signer);
    }
    
    EXPECT_EQ(tx_->Signers.size(), 3);
    
    // Signers should be unique
    for (size_t i = 0; i < tx_->Signers.size(); ++i) {
        for (size_t j = i + 1; j < tx_->Signers.size(); ++j) {
            EXPECT_NE(tx_->Signers[i].Account, tx_->Signers[j].Account);
        }
    }
}

// ============================================================================
// Witness Tests
// ============================================================================

TEST_F(TransactionCompleteTest, Witness_Creation) {
    Witness witness;
    witness.InvocationScript = ByteVector(64, 0xAB); // Signature
    witness.VerificationScript = ByteVector(35, 0xCD); // Public key + CHECKSIG
    
    EXPECT_EQ(witness.InvocationScript.Size(), 64);
    EXPECT_EQ(witness.VerificationScript.Size(), 35);
}

TEST_F(TransactionCompleteTest, Witness_ScriptHash) {
    auto witness = CreateWitness(*keys_[0], tx_->GetHash());
    
    auto scriptHash = witness.GetScriptHash();
    auto expectedHash = GetScriptHash(*keys_[0]);
    
    EXPECT_EQ(scriptHash, expectedHash);
}

TEST_F(TransactionCompleteTest, Witness_Verification) {
    // Create transaction
    tx_->Nonce = 12345;
    tx_->SystemFee = 1000000;
    tx_->NetworkFee = 500000;
    tx_->ValidUntilBlock = 10000;
    tx_->Script = ByteVector::FromString("Test");
    
    // Add signer
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    signer.Scopes = WitnessScope::CalledByEntry;
    tx_->Signers.push_back(signer);
    
    // Create witness
    auto witness = CreateWitness(*keys_[0], tx_->GetHash());
    tx_->Witnesses.push_back(witness);
    
    // Verify witness count matches signers
    EXPECT_EQ(tx_->Witnesses.size(), tx_->Signers.size());
}

TEST_F(TransactionCompleteTest, Witness_MultiSig_2of3) {
    // Create 2-of-3 multisig
    std::vector<ECPoint> publicKeys;
    for (int i = 0; i < 3; ++i) {
        publicKeys.push_back(keys_[i]->GetPublicKey());
    }
    
    // Create multisig verification script
    ScriptBuilder sb;
    sb.EmitPush(2); // M = 2
    for (const auto& pubkey : publicKeys) {
        sb.EmitPush(pubkey);
    }
    sb.EmitPush(3); // N = 3
    sb.Emit(OpCode::CHECKMULTISIG);
    auto verificationScript = sb.ToArray();
    
    // Create invocation script with 2 signatures
    ScriptBuilder invocation;
    invocation.EmitPush(keys_[0]->Sign(tx_->GetHash()));
    invocation.EmitPush(keys_[1]->Sign(tx_->GetHash()));
    auto invocationScript = invocation.ToArray();
    
    Witness witness;
    witness.InvocationScript = invocationScript;
    witness.VerificationScript = verificationScript;
    
    EXPECT_GT(witness.InvocationScript.Size(), 0);
    EXPECT_GT(witness.VerificationScript.Size(), 0);
}

// ============================================================================
// Attribute Tests
// ============================================================================

TEST_F(TransactionCompleteTest, Attribute_Types) {
    EXPECT_EQ(static_cast<uint8_t>(TransactionAttributeType::HighPriority), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(TransactionAttributeType::OracleResponse), 0x11);
    EXPECT_EQ(static_cast<uint8_t>(TransactionAttributeType::NotValidBefore), 0x20);
    EXPECT_EQ(static_cast<uint8_t>(TransactionAttributeType::Conflicts), 0x21);
}

TEST_F(TransactionCompleteTest, Attribute_HighPriority) {
    HighPriorityAttribute attr;
    attr.Type = TransactionAttributeType::HighPriority;
    
    tx_->Attributes.push_back(std::make_shared<HighPriorityAttribute>(attr));
    
    EXPECT_EQ(tx_->Attributes.size(), 1);
    EXPECT_EQ(tx_->Attributes[0]->Type, TransactionAttributeType::HighPriority);
}

TEST_F(TransactionCompleteTest, Attribute_OracleResponse) {
    OracleResponse oracle;
    oracle.Type = TransactionAttributeType::OracleResponse;
    oracle.Id = 12345;
    oracle.Code = OracleResponseCode::Success;
    oracle.Result = ByteVector::FromString("Oracle result");
    
    tx_->Attributes.push_back(std::make_shared<OracleResponse>(oracle));
    
    EXPECT_EQ(tx_->Attributes.size(), 1);
    auto oracleAttr = std::dynamic_pointer_cast<OracleResponse>(tx_->Attributes[0]);
    EXPECT_NE(oracleAttr, nullptr);
    EXPECT_EQ(oracleAttr->Id, 12345);
    EXPECT_EQ(oracleAttr->Code, OracleResponseCode::Success);
}

TEST_F(TransactionCompleteTest, Attribute_NotValidBefore) {
    NotValidBefore nvb;
    nvb.Type = TransactionAttributeType::NotValidBefore;
    nvb.Height = 5000;
    
    tx_->Attributes.push_back(std::make_shared<NotValidBefore>(nvb));
    
    EXPECT_EQ(tx_->Attributes.size(), 1);
    auto nvbAttr = std::dynamic_pointer_cast<NotValidBefore>(tx_->Attributes[0]);
    EXPECT_NE(nvbAttr, nullptr);
    EXPECT_EQ(nvbAttr->Height, 5000);
}

TEST_F(TransactionCompleteTest, Attribute_Conflicts) {
    ConflictsAttribute conflicts;
    conflicts.Type = TransactionAttributeType::Conflicts;
    
    UInt256 conflictingTx;
    conflictingTx.Fill(0xAB);
    conflicts.Hash = conflictingTx;
    
    tx_->Attributes.push_back(std::make_shared<ConflictsAttribute>(conflicts));
    
    EXPECT_EQ(tx_->Attributes.size(), 1);
    auto conflictAttr = std::dynamic_pointer_cast<ConflictsAttribute>(tx_->Attributes[0]);
    EXPECT_NE(conflictAttr, nullptr);
    EXPECT_EQ(conflictAttr->Hash, conflictingTx);
}

// ============================================================================
// Fee Calculation Tests
// ============================================================================

TEST_F(TransactionCompleteTest, Fee_SystemFee) {
    tx_->SystemFee = 10000000; // 0.1 GAS
    
    EXPECT_EQ(tx_->SystemFee, 10000000);
    
    // System fee should not exceed max
    const int64_t MAX_SYSTEM_FEE = 900000000000; // 9000 GAS
    tx_->SystemFee = MAX_SYSTEM_FEE + 1;
    
    EXPECT_FALSE(tx_->IsSystemFeeValid(MAX_SYSTEM_FEE));
}

TEST_F(TransactionCompleteTest, Fee_NetworkFee) {
    // Network fee = size * feePerByte + verification cost
    tx_->NetworkFee = 5000000; // 0.05 GAS
    
    EXPECT_EQ(tx_->NetworkFee, 5000000);
}

TEST_F(TransactionCompleteTest, Fee_Calculation) {
    // Setup transaction
    tx_->Nonce = 12345;
    tx_->SystemFee = 10000000;
    tx_->ValidUntilBlock = 10000;
    tx_->Script = ByteVector(100, 0);
    
    // Add signer
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    tx_->Signers.push_back(signer);
    
    // Add witness
    auto witness = CreateWitness(*keys_[0], tx_->GetHash());
    tx_->Witnesses.push_back(witness);
    
    // Calculate network fee
    int64_t feePerByte = 1000;
    auto size = tx_->GetSize();
    int64_t expectedNetworkFee = size * feePerByte;
    
    tx_->NetworkFee = expectedNetworkFee;
    
    EXPECT_GT(tx_->NetworkFee, 0);
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST_F(TransactionCompleteTest, Validation_Version) {
    tx_->Version = 1; // Invalid version
    
    EXPECT_FALSE(tx_->IsVersionValid());
    
    tx_->Version = 0; // Valid version
    EXPECT_TRUE(tx_->IsVersionValid());
}

TEST_F(TransactionCompleteTest, Validation_Size) {
    // Max transaction size is 102400 bytes
    const uint32_t MAX_TX_SIZE = 102400;
    
    tx_->Script = ByteVector(MAX_TX_SIZE - 100, 0); // Just under limit
    EXPECT_TRUE(tx_->IsSizeValid());
    
    tx_->Script = ByteVector(MAX_TX_SIZE + 100, 0); // Over limit
    EXPECT_FALSE(tx_->IsSizeValid());
}

TEST_F(TransactionCompleteTest, Validation_Script) {
    // Empty script is invalid
    tx_->Script = ByteVector();
    EXPECT_FALSE(tx_->IsScriptValid());
    
    // Non-empty script is valid
    tx_->Script = ByteVector::FromString("Valid script");
    EXPECT_TRUE(tx_->IsScriptValid());
}

TEST_F(TransactionCompleteTest, Validation_Signers) {
    // No signers is invalid
    EXPECT_FALSE(tx_->AreSignersValid());
    
    // Add valid signer
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    signer.Scopes = WitnessScope::CalledByEntry;
    tx_->Signers.push_back(signer);
    
    EXPECT_TRUE(tx_->AreSignersValid());
    
    // Duplicate signers are invalid
    tx_->Signers.push_back(signer);
    EXPECT_FALSE(tx_->AreSignersValid());
}

TEST_F(TransactionCompleteTest, Validation_Attributes) {
    // Max 16 attributes
    const int MAX_ATTRIBUTES = 16;
    
    for (int i = 0; i < MAX_ATTRIBUTES; ++i) {
        auto attr = std::make_shared<HighPriorityAttribute>();
        tx_->Attributes.push_back(attr);
    }
    
    EXPECT_TRUE(tx_->AreAttributesValid());
    
    // Too many attributes
    auto extraAttr = std::make_shared<HighPriorityAttribute>();
    tx_->Attributes.push_back(extraAttr);
    
    EXPECT_FALSE(tx_->AreAttributesValid());
}

TEST_F(TransactionCompleteTest, Validation_Witnesses) {
    // Add signer
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    tx_->Signers.push_back(signer);
    
    // No witnesses is invalid
    EXPECT_FALSE(tx_->AreWitnessesValid());
    
    // Add matching witness
    auto witness = CreateWitness(*keys_[0], tx_->GetHash());
    tx_->Witnesses.push_back(witness);
    
    EXPECT_TRUE(tx_->AreWitnessesValid());
    
    // Wrong number of witnesses
    auto extraWitness = CreateWitness(*keys_[1], tx_->GetHash());
    tx_->Witnesses.push_back(extraWitness);
    
    EXPECT_FALSE(tx_->AreWitnessesValid());
}

// ============================================================================
// Cosigner Tests
// ============================================================================

TEST_F(TransactionCompleteTest, Cosigner_SingleSigner) {
    // Single signer transaction
    Signer signer;
    signer.Account = GetScriptHash(*keys_[0]);
    signer.Scopes = WitnessScope::CalledByEntry;
    tx_->Signers.push_back(signer);
    
    EXPECT_EQ(tx_->GetSender(), signer.Account);
}

TEST_F(TransactionCompleteTest, Cosigner_MultipleCosigners) {
    // Multiple cosigners
    for (int i = 0; i < 3; ++i) {
        Signer signer;
        signer.Account = GetScriptHash(*keys_[i]);
        signer.Scopes = WitnessScope::CalledByEntry;
        tx_->Signers.push_back(signer);
    }
    
    // First signer is sender
    EXPECT_EQ(tx_->GetSender(), tx_->Signers[0].Account);
    
    // All cosigners present
    EXPECT_EQ(tx_->Signers.size(), 3);
}

// ============================================================================
// Oracle Response Tests
// ============================================================================

TEST_F(TransactionCompleteTest, OracleResponse_Codes) {
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::Success), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::ProtocolNotSupported), 0x10);
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::ConsensusUnreachable), 0x12);
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::NotFound), 0x14);
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::Timeout), 0x16);
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::Forbidden), 0x18);
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::ResponseTooLarge), 0x1a);
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::InsufficientFunds), 0x1c);
    EXPECT_EQ(static_cast<uint8_t>(OracleResponseCode::Error), 0xff);
}

TEST_F(TransactionCompleteTest, OracleResponse_MaxResultSize) {
    OracleResponse oracle;
    oracle.Type = TransactionAttributeType::OracleResponse;
    oracle.Code = OracleResponseCode::Success;
    
    // Max result size is 0xFFFF (65535) bytes
    const size_t MAX_RESULT_SIZE = 0xFFFF;
    oracle.Result = ByteVector(MAX_RESULT_SIZE, 0);
    
    EXPECT_EQ(oracle.Result.Size(), MAX_RESULT_SIZE);
    
    // Larger result should be truncated or rejected
    oracle.Result = ByteVector(MAX_RESULT_SIZE + 1, 0);
    EXPECT_FALSE(oracle.IsValid());
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(TransactionCompleteTest, Performance_Serialization) {
    // Create complex transaction
    tx_->Nonce = rand();
    tx_->SystemFee = 10000000;
    tx_->NetworkFee = 5000000;
    tx_->ValidUntilBlock = 100000;
    tx_->Script = ByteVector(1000, 0xAB);
    
    // Add signers
    for (int i = 0; i < 5; ++i) {
        Signer signer;
        signer.Account = GetScriptHash(*keys_[i]);
        signer.Scopes = WitnessScope::CalledByEntry;
        tx_->Signers.push_back(signer);
    }
    
    // Add witnesses
    for (const auto& signer : tx_->Signers) {
        Witness witness;
        witness.InvocationScript = ByteVector(64, 0);
        witness.VerificationScript = ByteVector(35, 0);
        tx_->Witnesses.push_back(witness);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Serialize/deserialize 1000 times
    for (int i = 0; i < 1000; ++i) {
        auto serialized = tx_->Serialize();
        Transaction deserialized;
        deserialized.Deserialize(serialized);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
}

TEST_F(TransactionCompleteTest, Performance_Hashing) {
    tx_->Nonce = rand();
    tx_->Script = ByteVector(1000, 0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Calculate hash 10000 times
    for (int i = 0; i < 10000; ++i) {
        auto hash = tx_->GetHash();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
}