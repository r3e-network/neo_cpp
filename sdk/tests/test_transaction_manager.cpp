/**
 * @file test_transaction_manager.cpp
 * @brief Unit tests for SDK transaction manager
 * @author Neo C++ Team
 * @date 2025
 */

#include <gtest/gtest.h>
#include <neo/sdk/transaction/transaction_manager.h>
#include <neo/sdk/wallet/wallet.h>
#include <neo/sdk/core/types.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

using namespace neo::sdk::transaction;
using namespace neo::sdk::core;
using namespace neo::sdk::wallet;
using namespace neo::io;

class TransactionManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<TransactionManager> txManager;
    std::unique_ptr<Wallet> wallet;
    std::string walletPath;
    std::string walletPassword;
    
    void SetUp() override {
        txManager = std::make_unique<TransactionManager>();
        
        // Create a test wallet for signing with secure random password
        walletPath = "test_tx_wallet.json";
        walletPassword = "TxTestWallet_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        wallet = Wallet::Create("TxTestWallet", walletPath, walletPassword);
        
        // Create some test accounts
        wallet->CreateAccount("Account1");
        wallet->CreateAccount("Account2");
    }
    
    void TearDown() override {
        if (std::filesystem::exists(walletPath)) {
            std::filesystem::remove(walletPath);
        }
    }
    
    UInt160 GetTestScriptHash() {
        return UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    }
    
    UInt160 GetTestTokenHash() {
        // NEO token hash (mainnet)
        return UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
    }
};

// Test transaction creation
TEST_F(TransactionManagerTest, CreateBasicTransaction) {
    auto tx = txManager->CreateTransaction();
    
    ASSERT_NE(tx, nullptr);
    EXPECT_EQ(tx->Version, 0);
    EXPECT_GT(tx->Nonce, 0); // Should have random nonce
    EXPECT_EQ(tx->SystemFee, 0);
    EXPECT_EQ(tx->NetworkFee, 0);
    EXPECT_GT(tx->ValidUntilBlock, 0);
}

TEST_F(TransactionManagerTest, SetTransactionProperties) {
    auto tx = txManager->CreateTransaction();
    
    txManager->SetSystemFee(tx, 1000000);
    txManager->SetNetworkFee(tx, 500000);
    txManager->SetValidUntilBlock(tx, 99999);
    
    EXPECT_EQ(tx->SystemFee, 1000000);
    EXPECT_EQ(tx->NetworkFee, 500000);
    EXPECT_EQ(tx->ValidUntilBlock, 99999);
}

// Test signers
TEST_F(TransactionManagerTest, AddSigner) {
    auto tx = txManager->CreateTransaction();
    auto scriptHash = GetTestScriptHash();
    
    txManager->AddSigner(tx, scriptHash, WitnessScope::CalledByEntry);
    
    EXPECT_EQ(tx->Signers.size(), 1);
    EXPECT_EQ(tx->Signers[0].Account, scriptHash);
    EXPECT_EQ(tx->Signers[0].Scopes, WitnessScope::CalledByEntry);
}

TEST_F(TransactionManagerTest, AddMultipleSigners) {
    auto tx = txManager->CreateTransaction();
    
    auto hash1 = UInt160::Parse("0x1111111111111111111111111111111111111111");
    auto hash2 = UInt160::Parse("0x2222222222222222222222222222222222222222");
    auto hash3 = UInt160::Parse("0x3333333333333333333333333333333333333333");
    
    txManager->AddSigner(tx, hash1, WitnessScope::CalledByEntry);
    txManager->AddSigner(tx, hash2, WitnessScope::Global);
    txManager->AddSigner(tx, hash3, WitnessScope::CustomContracts);
    
    EXPECT_EQ(tx->Signers.size(), 3);
    EXPECT_EQ(tx->Signers[0].Account, hash1);
    EXPECT_EQ(tx->Signers[1].Account, hash2);
    EXPECT_EQ(tx->Signers[2].Account, hash3);
}

TEST_F(TransactionManagerTest, AddSignerWithContracts) {
    auto tx = txManager->CreateTransaction();
    auto scriptHash = GetTestScriptHash();
    
    std::vector<UInt160> allowedContracts = {
        UInt160::Parse("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        UInt160::Parse("0xbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")
    };
    
    txManager->AddSigner(tx, scriptHash, WitnessScope::CustomContracts, allowedContracts);
    
    EXPECT_EQ(tx->Signers.size(), 1);
    EXPECT_EQ(tx->Signers[0].AllowedContracts.size(), 2);
    EXPECT_EQ(tx->Signers[0].AllowedContracts[0], allowedContracts[0]);
    EXPECT_EQ(tx->Signers[0].AllowedContracts[1], allowedContracts[1]);
}

// Test attributes
TEST_F(TransactionManagerTest, AddHighPriorityAttribute) {
    auto tx = txManager->CreateTransaction();
    
    txManager->AddHighPriority(tx);
    
    EXPECT_EQ(tx->Attributes.size(), 1);
    EXPECT_EQ(tx->Attributes[0].Type, TransactionAttributeType::HighPriority);
}

TEST_F(TransactionManagerTest, AddOracleResponseAttribute) {
    auto tx = txManager->CreateTransaction();
    
    uint64_t requestId = 12345;
    std::vector<uint8_t> responseData = {0x01, 0x02, 0x03, 0x04};
    
    txManager->AddOracleResponse(tx, requestId, responseData);
    
    EXPECT_EQ(tx->Attributes.size(), 1);
    EXPECT_EQ(tx->Attributes[0].Type, TransactionAttributeType::OracleResponse);
    // Response data should contain request ID and response
    EXPECT_GT(tx->Attributes[0].Data.size(), responseData.size());
}

// Test script building
TEST_F(TransactionManagerTest, BuildSimpleScript) {
    auto script = txManager->CreateScript();
    
    // Add some operations
    txManager->EmitPush(script, int64_t(42));
    txManager->EmitPush(script, std::string("Hello"));
    
    EXPECT_GT(script.size(), 0);
}

TEST_F(TransactionManagerTest, BuildContractCall) {
    auto contractHash = GetTestScriptHash();
    std::string method = "transfer";
    std::vector<ContractParameter> params;
    params.emplace_back(UInt160::Zero()); // from
    params.emplace_back(UInt160::Zero()); // to
    params.emplace_back(int64_t(1000000)); // amount
    params.emplace_back(std::vector<uint8_t>()); // data
    
    auto script = txManager->BuildContractCall(contractHash, method, params);
    
    EXPECT_GT(script.size(), 0);
    // Script should contain method name and parameters
}

TEST_F(TransactionManagerTest, BuildMultiContractCall) {
    std::vector<ContractCall> calls;
    
    // First call
    ContractCall call1;
    call1.contractHash = GetTestScriptHash();
    call1.method = "method1";
    call1.params.emplace_back(int64_t(100));
    calls.push_back(call1);
    
    // Second call
    ContractCall call2;
    call2.contractHash = GetTestTokenHash();
    call2.method = "method2";
    call2.params.emplace_back(std::string("test"));
    calls.push_back(call2);
    
    auto script = txManager->BuildMultiContractCall(calls);
    
    EXPECT_GT(script.size(), 0);
    // Script should be larger than single call
    auto singleScript = txManager->BuildContractCall(call1.contractHash, call1.method, call1.params);
    EXPECT_GT(script.size(), singleScript.size());
}

// Test NEP17 transfer
TEST_F(TransactionManagerTest, CreateNEP17Transfer) {
    auto tokenHash = GetTestTokenHash();
    auto from = UInt160::Parse("0x1111111111111111111111111111111111111111");
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    uint64_t amount = 1000000000; // 10 tokens with 8 decimals
    
    auto tx = txManager->CreateNEP17Transfer(tokenHash, from, to, amount);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_EQ(tx->Signers.size(), 1);
    EXPECT_EQ(tx->Signers[0].Account, from);
    EXPECT_GT(tx->Script.size(), 0);
}

TEST_F(TransactionManagerTest, CreateNEP17TransferWithData) {
    auto tokenHash = GetTestTokenHash();
    auto from = UInt160::Parse("0x1111111111111111111111111111111111111111");
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    uint64_t amount = 1000000000;
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    
    auto tx = txManager->CreateNEP17Transfer(tokenHash, from, to, amount, data);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
}

// Test multi-transfers
TEST_F(TransactionManagerTest, CreateMultiTransfer) {
    std::vector<NEP17Transfer> transfers;
    
    // First transfer
    NEP17Transfer transfer1;
    transfer1.tokenHash = GetTestTokenHash();
    transfer1.from = UInt160::Parse("0x1111111111111111111111111111111111111111");
    transfer1.to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    transfer1.amount = 1000000000;
    transfers.push_back(transfer1);
    
    // Second transfer
    NEP17Transfer transfer2;
    transfer2.tokenHash = GetTestTokenHash();
    transfer2.from = UInt160::Parse("0x1111111111111111111111111111111111111111");
    transfer2.to = UInt160::Parse("0x3333333333333333333333333333333333333333");
    transfer2.amount = 500000000;
    transfers.push_back(transfer2);
    
    auto tx = txManager->CreateMultiTransfer(transfers);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    // Should have signers for all unique 'from' addresses
    EXPECT_GE(tx->Signers.size(), 1);
}

// Test system calls
TEST_F(TransactionManagerTest, CreateContractDeployment) {
    std::vector<uint8_t> nefFile = {0x4E, 0x45, 0x46}; // NEF header
    std::string manifest = "{\"name\":\"TestContract\"}";
    
    auto tx = txManager->CreateContractDeployment(nefFile, manifest);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    EXPECT_GT(tx->SystemFee, 0); // Deployment requires system fee
}

TEST_F(TransactionManagerTest, CreateContractUpdate) {
    auto contractHash = GetTestScriptHash();
    std::vector<uint8_t> nefFile = {0x4E, 0x45, 0x46};
    std::string manifest = "{\"name\":\"UpdatedContract\"}";
    
    auto tx = txManager->CreateContractUpdate(contractHash, nefFile, manifest);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
}

TEST_F(TransactionManagerTest, CreateContractDestroy) {
    auto contractHash = GetTestScriptHash();
    
    auto tx = txManager->CreateContractDestroy(contractHash);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
}

// Test witness
TEST_F(TransactionManagerTest, AddWitness) {
    auto tx = txManager->CreateTransaction();
    
    std::vector<uint8_t> invocation = {0x40}; // Signature placeholder
    std::vector<uint8_t> verification = {0x21}; // Public key placeholder
    
    txManager->AddWitness(tx, invocation, verification);
    
    EXPECT_EQ(tx->Witnesses.size(), 1);
    EXPECT_EQ(tx->Witnesses[0].InvocationScript, invocation);
    EXPECT_EQ(tx->Witnesses[0].VerificationScript, verification);
}

TEST_F(TransactionManagerTest, SignTransaction) {
    auto tx = txManager->CreateTransaction();
    
    // Add signer from wallet account
    auto accounts = wallet->GetAccounts();
    ASSERT_GT(accounts.size(), 0);
    
    auto address = accounts[0]->GetAddress();
    auto scriptHash = ScriptHashFromAddress(address).value_or(UInt160::Zero());
    
    txManager->AddSigner(tx, scriptHash, WitnessScope::CalledByEntry);
    
    // Sign with wallet
    EXPECT_TRUE(txManager->SignTransaction(tx, wallet.get()));
    
    // Should have witness after signing
    EXPECT_GT(tx->Witnesses.size(), 0);
}

// Test fee calculation
TEST_F(TransactionManagerTest, CalculateNetworkFee) {
    auto tx = txManager->CreateTransaction();
    
    // Add some script
    tx->Script = {0x00, 0x01, 0x02, 0x03};
    
    // Add signer
    txManager->AddSigner(tx, GetTestScriptHash(), WitnessScope::CalledByEntry);
    
    auto fee = txManager->CalculateNetworkFee(tx);
    
    EXPECT_GT(fee, 0);
}

TEST_F(TransactionManagerTest, EstimateSystemFee) {
    auto contractHash = GetTestScriptHash();
    std::string method = "transfer";
    std::vector<ContractParameter> params;
    params.emplace_back(UInt160::Zero());
    params.emplace_back(UInt160::Zero());
    params.emplace_back(int64_t(1000000));
    
    // This would normally require RPC connection
    // For testing, we expect a reasonable estimate
    auto fee = txManager->EstimateSystemFee(contractHash, method, params);
    
    // Should return default or estimated value
    EXPECT_GE(fee, 0);
}

// Test transaction validation
TEST_F(TransactionManagerTest, ValidateTransaction) {
    auto tx = txManager->CreateTransaction();
    
    // Invalid transaction (no signers)
    EXPECT_FALSE(txManager->ValidateTransaction(tx));
    
    // Add signer
    txManager->AddSigner(tx, GetTestScriptHash(), WitnessScope::CalledByEntry);
    
    // Still invalid (no script)
    EXPECT_FALSE(txManager->ValidateTransaction(tx));
    
    // Add script
    tx->Script = {0x00, 0x01};
    
    // Now should be valid
    EXPECT_TRUE(txManager->ValidateTransaction(tx));
}

TEST_F(TransactionManagerTest, ValidateTransactionSize) {
    auto tx = txManager->CreateTransaction();
    
    // Create very large script (exceeds max size)
    std::vector<uint8_t> largeScript(1024 * 1024, 0x00); // 1MB
    tx->Script = largeScript;
    
    txManager->AddSigner(tx, GetTestScriptHash(), WitnessScope::CalledByEntry);
    
    // Should be invalid due to size
    EXPECT_FALSE(txManager->ValidateTransaction(tx));
}

// Test script builder
TEST_F(TransactionManagerTest, ScriptBuilderOperations) {
    auto script = txManager->CreateScript();
    
    // Test various push operations
    txManager->EmitPush(script, true);
    txManager->EmitPush(script, false);
    txManager->EmitPush(script, int64_t(-1));
    txManager->EmitPush(script, int64_t(0));
    txManager->EmitPush(script, int64_t(1));
    txManager->EmitPush(script, int64_t(16));
    txManager->EmitPush(script, int64_t(12345));
    txManager->EmitPush(script, std::string("test"));
    txManager->EmitPush(script, UInt160::Zero());
    txManager->EmitPush(script, UInt256::Zero());
    
    EXPECT_GT(script.size(), 0);
}

TEST_F(TransactionManagerTest, ScriptBuilderSysCall) {
    auto script = txManager->CreateScript();
    
    // Emit system call
    txManager->EmitSysCall(script, "System.Contract.Call");
    
    EXPECT_GT(script.size(), 0);
}

// Test transaction serialization
TEST_F(TransactionManagerTest, SerializeTransaction) {
    auto tx = txManager->CreateTransaction();
    
    txManager->AddSigner(tx, GetTestScriptHash(), WitnessScope::CalledByEntry);
    tx->Script = {0x00, 0x01, 0x02};
    
    auto data = txManager->SerializeTransaction(tx);
    
    EXPECT_GT(data.size(), 0);
    
    // Deserialize and verify
    auto tx2 = txManager->DeserializeTransaction(data);
    ASSERT_NE(tx2, nullptr);
    EXPECT_EQ(tx->Version, tx2->Version);
    EXPECT_EQ(tx->Nonce, tx2->Nonce);
    EXPECT_EQ(tx->Script, tx2->Script);
}

// Test oracle request
TEST_F(TransactionManagerTest, CreateOracleRequest) {
    std::string url = "https://api.example.com/data";
    std::string filter = "$.result";
    std::string callback = "handleResponse";
    std::vector<uint8_t> userData = {0x01, 0x02};
    uint64_t gasForResponse = 1000000;
    
    auto tx = txManager->CreateOracleRequest(url, filter, callback, userData, gasForResponse);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    EXPECT_GT(tx->SystemFee, 0); // Oracle requests require system fee
}

// Test voting
TEST_F(TransactionManagerTest, CreateVoteTransaction) {
    auto voter = UInt160::Parse("0x1111111111111111111111111111111111111111");
    auto candidate = "02b3622bf4017bdfe317c58aed5f4c753f206b7db896046fa7d774bbc4bf7f8dc2"; // Public key
    
    auto tx = txManager->CreateVoteTransaction(voter, candidate);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    EXPECT_EQ(tx->Signers.size(), 1);
    EXPECT_EQ(tx->Signers[0].Account, voter);
}

// Test gas claim
TEST_F(TransactionManagerTest, CreateGasClaimTransaction) {
    auto claimer = UInt160::Parse("0x1111111111111111111111111111111111111111");
    
    auto tx = txManager->CreateGasClaimTransaction(claimer);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    EXPECT_EQ(tx->Signers.size(), 1);
    EXPECT_EQ(tx->Signers[0].Account, claimer);
}

// Performance test
TEST_F(TransactionManagerTest, TransactionCreationPerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create 1000 transactions
    for (int i = 0; i < 1000; i++) {
        auto tx = txManager->CreateTransaction();
        txManager->AddSigner(tx, GetTestScriptHash(), WitnessScope::CalledByEntry);
        tx->Script = {0x00, 0x01, 0x02};
        txManager->SerializeTransaction(tx);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should create 1000 transactions in reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
}

// Error handling tests
TEST_F(TransactionManagerTest, InvalidOperations) {
    // Null transaction
    std::shared_ptr<Transaction> nullTx;
    EXPECT_FALSE(txManager->ValidateTransaction(nullTx));
    
    // Empty script
    std::vector<uint8_t> emptyScript;
    auto contractCall = txManager->BuildContractCall(UInt160::Zero(), "", {});
    EXPECT_EQ(contractCall.size(), 0);
    
    // Invalid serialization
    std::vector<uint8_t> invalidData = {0xFF, 0xFF};
    auto tx = txManager->DeserializeTransaction(invalidData);
    EXPECT_EQ(tx, nullptr);
}