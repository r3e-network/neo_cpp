/**
 * @file test_end_to_end_scenarios.cpp
 * @brief End-to-end integration tests for Neo C++
 * Tests complete workflows across multiple components
 */

#include <gtest/gtest.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/block.h>
#include <neo/network/p2p/local_node.h>
#include <neo/consensus/consensus_service.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/cryptography/key_pair.h>
#include <neo/wallets/wallet.h>
#include <neo/wallets/account.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

using namespace neo;
using namespace neo::ledger;
using namespace neo::network::p2p;
using namespace neo::consensus;
using namespace neo::smartcontract;
using namespace neo::smartcontract::native;
using namespace neo::vm;
using namespace neo::cryptography;
using namespace neo::wallets;
using namespace neo::io;

class EndToEndScenariosTest : public ::testing::Test {
protected:
    std::unique_ptr<Blockchain> blockchain_;
    std::unique_ptr<LocalNode> local_node_;
    std::unique_ptr<ConsensusService> consensus_;
    std::unique_ptr<Wallet> wallet_;
    std::vector<std::unique_ptr<Account>> accounts_;
    
    void SetUp() override {
        // Initialize blockchain
        blockchain_ = std::make_unique<Blockchain>();
        blockchain_->Initialize();
        
        // Initialize network node
        local_node_ = std::make_unique<LocalNode>(20333);
        
        // Initialize consensus
        consensus_ = std::make_unique<ConsensusService>(blockchain_.get(), local_node_.get());
        
        // Create test wallet with accounts
        wallet_ = std::make_unique<Wallet>("test_wallet.json");
        for (int i = 0; i < 5; ++i) {
            auto account = wallet_->CreateAccount();
            accounts_.push_back(std::move(account));
        }
    }
    
    void TearDown() override {
        // Clean shutdown
        consensus_->Stop();
        local_node_->Stop();
        blockchain_->Stop();
    }
    
    std::unique_ptr<Transaction> CreateTransferTransaction(
        Account* from,
        const UInt160& to,
        int64_t amount,
        const UInt160& assetId
    ) {
        auto tx = std::make_unique<Transaction>();
        
        // Set transaction fields
        tx->Version = 0;
        tx->Nonce = rand();
        tx->ValidUntilBlock = blockchain_->GetHeight() + 100;
        tx->SystemFee = 1000000; // 0.01 GAS
        tx->NetworkFee = 500000; // 0.005 GAS
        
        // Create transfer script
        ScriptBuilder sb;
        sb.EmitPush(amount);
        sb.EmitPush(to);
        sb.EmitPush(from->GetScriptHash());
        sb.EmitPush(3);
        sb.Emit(OpCode::PACK);
        sb.EmitPush("transfer");
        sb.EmitAppCall(assetId);
        tx->Script = sb.ToArray();
        
        // Add signer
        Signer signer;
        signer.Account = from->GetScriptHash();
        signer.Scopes = WitnessScope::CalledByEntry;
        tx->Signers.push_back(signer);
        
        // Sign transaction
        auto witness = from->Sign(tx->GetHash());
        tx->Witnesses.push_back(witness);
        
        return tx;
    }
};

// ============================================================================
// Token Transfer Scenarios
// ============================================================================

TEST_F(EndToEndScenariosTest, Scenario_SimpleNEOTransfer) {
    // Setup: Give account[0] some NEO
    auto sender = accounts_[0].get();
    auto receiver = accounts_[1].get();
    
    // Mint NEO to sender (would normally come from genesis or existing balance)
    auto snapshot = blockchain_->GetSnapshot();
    NeoToken::Mint(snapshot.get(), sender->GetScriptHash(), 1000, false);
    snapshot->Commit();
    
    // Create transfer transaction
    auto tx = CreateTransferTransaction(
        sender,
        receiver->GetScriptHash(),
        100,
        NeoToken::Hash()
    );
    
    // Verify transaction
    EXPECT_TRUE(blockchain_->VerifyTransaction(tx.get()));
    
    // Add to mempool
    EXPECT_TRUE(blockchain_->AddTransaction(tx.get()));
    
    // Mine block with transaction
    auto block = blockchain_->CreateNewBlock();
    block->Transactions.push_back(*tx);
    EXPECT_TRUE(blockchain_->AddBlock(block.get()));
    
    // Verify balances
    snapshot = blockchain_->GetSnapshot();
    auto senderBalance = NeoToken::BalanceOf(snapshot.get(), sender->GetScriptHash());
    auto receiverBalance = NeoToken::BalanceOf(snapshot.get(), receiver->GetScriptHash());
    
    EXPECT_EQ(senderBalance, 900);
    EXPECT_EQ(receiverBalance, 100);
}

TEST_F(EndToEndScenariosTest, Scenario_GASClaimAndTransfer) {
    auto account = accounts_[0].get();
    
    // Hold NEO to generate GAS
    auto snapshot = blockchain_->GetSnapshot();
    NeoToken::Mint(snapshot.get(), account->GetScriptHash(), 1000, false);
    snapshot->Commit();
    
    // Advance several blocks to generate GAS
    for (int i = 0; i < 10; ++i) {
        auto block = blockchain_->CreateNewBlock();
        EXPECT_TRUE(blockchain_->AddBlock(block.get()));
    }
    
    // Calculate and claim GAS
    snapshot = blockchain_->GetSnapshot();
    auto unclaimed = NeoToken::CalculateBonus(
        snapshot.get(),
        account->GetScriptHash(),
        blockchain_->GetHeight()
    );
    EXPECT_GT(unclaimed, 0);
    
    // Create claim transaction
    auto claimTx = std::make_unique<Transaction>();
    claimTx->Version = 0;
    claimTx->Nonce = rand();
    claimTx->ValidUntilBlock = blockchain_->GetHeight() + 100;
    
    ScriptBuilder sb;
    sb.EmitPush(blockchain_->GetHeight());
    sb.EmitPush(account->GetScriptHash());
    sb.EmitPush(2);
    sb.Emit(OpCode::PACK);
    sb.EmitPush("claimGas");
    sb.EmitAppCall(NeoToken::Hash());
    claimTx->Script = sb.ToArray();
    
    // Process claim
    EXPECT_TRUE(blockchain_->AddTransaction(claimTx.get()));
    
    // Verify GAS balance
    auto gasBalance = GasToken::BalanceOf(snapshot.get(), account->GetScriptHash());
    EXPECT_GT(gasBalance, 0);
}

// ============================================================================
// Smart Contract Deployment and Invocation
// ============================================================================

TEST_F(EndToEndScenariosTest, Scenario_DeployAndInvokeContract) {
    auto deployer = accounts_[0].get();
    
    // Create simple contract bytecode
    ScriptBuilder contractCode;
    contractCode.EmitPush("Hello");
    contractCode.Emit(OpCode::RET);
    
    // Create deployment transaction
    auto deployTx = std::make_unique<Transaction>();
    deployTx->Version = 0;
    deployTx->Nonce = rand();
    deployTx->SystemFee = 1000000000; // 10 GAS for deployment
    deployTx->NetworkFee = 1000000;
    deployTx->ValidUntilBlock = blockchain_->GetHeight() + 100;
    
    // Deployment script
    ScriptBuilder deployScript;
    deployScript.EmitPush(R"({"name":"TestContract","abi":{}})"); // Manifest
    deployScript.EmitPush(contractCode.ToArray()); // NEF
    deployScript.EmitPush(2);
    deployScript.Emit(OpCode::PACK);
    deployScript.EmitPush("deploy");
    deployScript.EmitAppCall(ManagementContract::Hash());
    deployTx->Script = deployScript.ToArray();
    
    // Add signer
    Signer signer;
    signer.Account = deployer->GetScriptHash();
    signer.Scopes = WitnessScope::CalledByEntry;
    deployTx->Signers.push_back(signer);
    
    // Sign and add to blockchain
    auto witness = deployer->Sign(deployTx->GetHash());
    deployTx->Witnesses.push_back(witness);
    
    EXPECT_TRUE(blockchain_->AddTransaction(deployTx.get()));
    
    // Mine block
    auto block = blockchain_->CreateNewBlock();
    EXPECT_TRUE(blockchain_->AddBlock(block.get()));
    
    // Get deployed contract hash
    auto contractHash = Crypto::Hash160(contractCode.ToArray().AsSpan());
    
    // Invoke contract
    auto invokeTx = std::make_unique<Transaction>();
    invokeTx->Version = 0;
    invokeTx->Nonce = rand();
    invokeTx->SystemFee = 100000;
    invokeTx->NetworkFee = 100000;
    invokeTx->ValidUntilBlock = blockchain_->GetHeight() + 100;
    
    ScriptBuilder invokeScript;
    invokeScript.EmitPush(0); // No parameters
    invokeScript.Emit(OpCode::PACK);
    invokeScript.EmitPush("main");
    invokeScript.EmitAppCall(contractHash);
    invokeTx->Script = invokeScript.ToArray();
    
    // Process invocation
    EXPECT_TRUE(blockchain_->AddTransaction(invokeTx.get()));
}

// ============================================================================
// Consensus and Block Production
// ============================================================================

TEST_F(EndToEndScenariosTest, Scenario_ConsensusBlockProduction) {
    // Start consensus service
    consensus_->Start();
    
    // Wait for consensus to produce blocks
    auto initialHeight = blockchain_->GetHeight();
    
    // Wait for at least 3 blocks
    auto timeout = std::chrono::seconds(30);
    auto start = std::chrono::steady_clock::now();
    
    while (blockchain_->GetHeight() < initialHeight + 3) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            FAIL() << "Consensus failed to produce blocks in time";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Verify blocks were produced
    EXPECT_GT(blockchain_->GetHeight(), initialHeight);
    
    // Verify block validity
    for (uint32_t i = initialHeight + 1; i <= blockchain_->GetHeight(); ++i) {
        auto block = blockchain_->GetBlock(i);
        ASSERT_NE(block, nullptr);
        EXPECT_TRUE(blockchain_->VerifyBlock(block.get()));
    }
}

// ============================================================================
// Multi-signature Transaction
// ============================================================================

TEST_F(EndToEndScenariosTest, Scenario_MultiSigTransaction) {
    // Create 2-of-3 multisig account
    std::vector<ECPoint> publicKeys;
    for (int i = 0; i < 3; ++i) {
        publicKeys.push_back(accounts_[i]->GetKey().GetPublicKey());
    }
    
    auto multiSigAccount = wallet_->CreateMultiSigAccount(2, publicKeys);
    
    // Fund multisig account
    auto snapshot = blockchain_->GetSnapshot();
    NeoToken::Mint(snapshot.get(), multiSigAccount->GetScriptHash(), 1000, false);
    snapshot->Commit();
    
    // Create transaction from multisig
    auto tx = std::make_unique<Transaction>();
    tx->Version = 0;
    tx->Nonce = rand();
    tx->ValidUntilBlock = blockchain_->GetHeight() + 100;
    tx->SystemFee = 1000000;
    tx->NetworkFee = 500000;
    
    // Transfer script
    ScriptBuilder sb;
    sb.EmitPush(100);
    sb.EmitPush(accounts_[3]->GetScriptHash());
    sb.EmitPush(multiSigAccount->GetScriptHash());
    sb.EmitPush(3);
    sb.Emit(OpCode::PACK);
    sb.EmitPush("transfer");
    sb.EmitAppCall(NeoToken::Hash());
    tx->Script = sb.ToArray();
    
    // Add signer
    Signer signer;
    signer.Account = multiSigAccount->GetScriptHash();
    signer.Scopes = WitnessScope::CalledByEntry;
    tx->Signers.push_back(signer);
    
    // Get 2 signatures
    auto sig1 = accounts_[0]->GetKey().Sign(tx->GetHash());
    auto sig2 = accounts_[1]->GetKey().Sign(tx->GetHash());
    
    // Create multisig witness
    ScriptBuilder invocation;
    invocation.EmitPush(sig1);
    invocation.EmitPush(sig2);
    
    Witness witness;
    witness.InvocationScript = invocation.ToArray();
    witness.VerificationScript = multiSigAccount->GetContract()->Script;
    tx->Witnesses.push_back(witness);
    
    // Process transaction
    EXPECT_TRUE(blockchain_->VerifyTransaction(tx.get()));
    EXPECT_TRUE(blockchain_->AddTransaction(tx.get()));
}

// ============================================================================
// Oracle Request and Response
// ============================================================================

TEST_F(EndToEndScenariosTest, Scenario_OracleRequestResponse) {
    auto requester = accounts_[0].get();
    
    // Create oracle request transaction
    auto requestTx = std::make_unique<Transaction>();
    requestTx->Version = 0;
    requestTx->Nonce = rand();
    requestTx->SystemFee = 50000000; // 0.5 GAS for oracle
    requestTx->NetworkFee = 1000000;
    requestTx->ValidUntilBlock = blockchain_->GetHeight() + 100;
    
    // Oracle request script
    ScriptBuilder requestScript;
    requestScript.EmitPush(10000000); // GAS for response
    requestScript.EmitPush(ByteVector::FromString("userData"));
    requestScript.EmitPush("onOracleResponse");
    requestScript.EmitPush("$.price");
    requestScript.EmitPush("https://api.example.com/price");
    requestScript.EmitPush(5);
    requestScript.Emit(OpCode::PACK);
    requestScript.EmitPush("request");
    requestScript.EmitAppCall(OracleContract::Hash());
    requestTx->Script = requestScript.ToArray();
    
    // Add signer
    Signer signer;
    signer.Account = requester->GetScriptHash();
    signer.Scopes = WitnessScope::CalledByEntry;
    requestTx->Signers.push_back(signer);
    
    // Sign and submit
    auto witness = requester->Sign(requestTx->GetHash());
    requestTx->Witnesses.push_back(witness);
    
    EXPECT_TRUE(blockchain_->AddTransaction(requestTx.get()));
    
    // Mine block with request
    auto block = blockchain_->CreateNewBlock();
    EXPECT_TRUE(blockchain_->AddBlock(block.get()));
    
    // Simulate oracle response
    auto responseTx = std::make_unique<Transaction>();
    responseTx->Version = 0;
    responseTx->Nonce = rand();
    responseTx->ValidUntilBlock = blockchain_->GetHeight() + 100;
    
    // Add oracle response attribute
    auto oracleResponse = std::make_shared<OracleResponse>();
    oracleResponse->Type = TransactionAttributeType::OracleResponse;
    oracleResponse->Id = 1; // Request ID
    oracleResponse->Code = OracleResponseCode::Success;
    oracleResponse->Result = ByteVector::FromString("42.50");
    responseTx->Attributes.push_back(oracleResponse);
    
    // Process response
    EXPECT_TRUE(blockchain_->VerifyTransaction(responseTx.get()));
}

// ============================================================================
// Network Synchronization
// ============================================================================

TEST_F(EndToEndScenariosTest, Scenario_NetworkSync) {
    // Create second node
    auto remote_node = std::make_unique<LocalNode>(20334);
    
    // Connect nodes
    local_node_->ConnectTo("127.0.0.1", 20334);
    remote_node->ConnectTo("127.0.0.1", 20333);
    
    // Wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Add blocks to first node
    for (int i = 0; i < 5; ++i) {
        auto block = blockchain_->CreateNewBlock();
        EXPECT_TRUE(blockchain_->AddBlock(block.get()));
    }
    
    // Trigger synchronization
    remote_node->StartSync();
    
    // Wait for sync
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Verify both nodes have same height
    EXPECT_EQ(local_node->GetBlockHeight(), remote_node->GetBlockHeight());
}

// ============================================================================
// Performance and Stress Tests
// ============================================================================

TEST_F(EndToEndScenariosTest, Scenario_HighVolumeTransactions) {
    // Create many transactions
    std::vector<std::unique_ptr<Transaction>> transactions;
    
    for (int i = 0; i < 100; ++i) {
        auto tx = std::make_unique<Transaction>();
        tx->Version = 0;
        tx->Nonce = rand();
        tx->ValidUntilBlock = blockchain_->GetHeight() + 1000;
        tx->SystemFee = 100000;
        tx->NetworkFee = 100000;
        
        // Simple script
        ScriptBuilder sb;
        sb.EmitPush(i);
        sb.Emit(OpCode::RET);
        tx->Script = sb.ToArray();
        
        // Add signer
        Signer signer;
        signer.Account = accounts_[i % accounts_.size()]->GetScriptHash();
        signer.Scopes = WitnessScope::CalledByEntry;
        tx->Signers.push_back(signer);
        
        // Sign
        auto witness = accounts_[i % accounts_.size()]->Sign(tx->GetHash());
        tx->Witnesses.push_back(witness);
        
        transactions.push_back(std::move(tx));
    }
    
    // Measure throughput
    auto start = std::chrono::high_resolution_clock::now();
    
    for (auto& tx : transactions) {
        EXPECT_TRUE(blockchain_->AddTransaction(tx.get()));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should process 100 transactions quickly
    EXPECT_LT(duration.count(), 5000); // Under 5 seconds
    
    // Mine block with transactions
    auto block = blockchain_->CreateNewBlock();
    EXPECT_GT(block->Transactions.size(), 0);
    EXPECT_TRUE(blockchain_->AddBlock(block.get()));
}

// ============================================================================
// Recovery and Fault Tolerance
// ============================================================================

TEST_F(EndToEndScenariosTest, Scenario_ChainRecoveryAfterCrash) {
    // Add some blocks
    for (int i = 0; i < 10; ++i) {
        auto block = blockchain_->CreateNewBlock();
        EXPECT_TRUE(blockchain_->AddBlock(block.get()));
    }
    
    auto heightBefore = blockchain_->GetHeight();
    
    // Simulate crash and restart
    blockchain_->Stop();
    blockchain_.reset();
    
    // Recreate blockchain
    blockchain_ = std::make_unique<Blockchain>();
    blockchain_->Initialize();
    
    // Verify blockchain recovered
    EXPECT_EQ(blockchain_->GetHeight(), heightBefore);
    
    // Verify can continue adding blocks
    auto block = blockchain_->CreateNewBlock();
    EXPECT_TRUE(blockchain_->AddBlock(block.get()));
    EXPECT_EQ(blockchain_->GetHeight(), heightBefore + 1);
}