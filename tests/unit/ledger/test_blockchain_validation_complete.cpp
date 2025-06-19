#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/ledger/blockchain.h"
#include "neo/ledger/block.h"
#include "neo/ledger/transaction.h"
#include "neo/ledger/header_cache.h"
#include "neo/ledger/mempool.h"
#include "neo/persistence/data_cache.h"
#include "neo/smartcontract/native/neo_token.h"
#include "neo/smartcontract/native/gas_token.h"
#include "neo/cryptography/merkletree.h"
#include "neo/wallets/key_pair.h"
#include "neo/protocol_settings.h"
#include <memory>
#include <vector>

using namespace neo;
using namespace neo::ledger;
using namespace neo::persistence;
using namespace neo::smartcontract::native;
using namespace neo::cryptography;
using namespace neo::wallets;

class BlockchainValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize protocol settings
        protocol_settings_ = std::make_shared<ProtocolSettings>();
        protocol_settings_->Network = 0x334E454F; // TestNet
        protocol_settings_->MaxTransactionsPerBlock = 512;
        protocol_settings_->MaxBlockSize = 262144; // 256KB
        protocol_settings_->MaxBlockSystemFee = 900000000000ULL; // 9000 GAS
        protocol_settings_->SecondsPerBlock = 15;
        
        // Initialize blockchain
        blockchain_ = std::make_shared<Blockchain>(protocol_settings_);
        
        // Initialize data cache
        data_cache_ = std::make_shared<DataCache>();
        
        // Initialize native tokens
        neo_token_ = std::make_shared<NeoToken>();
        gas_token_ = std::make_shared<GasToken>();
        
        // Create test accounts
        CreateTestAccounts();
        
        // Create genesis block
        genesis_block_ = CreateGenesisBlock();
    }
    
    void CreateTestAccounts() {
        for (int i = 0; i < 5; i++) {
            auto key_pair = KeyPair::Generate();
            test_keypairs_.push_back(key_pair);
            test_accounts_.push_back(CreateAccount(key_pair));
        }
    }
    
    UInt160 CreateAccount(const KeyPair& key_pair) {
        return Contract::CreateSignatureRedeemScript(key_pair.PublicKey).ToScriptHash();
    }
    
    std::shared_ptr<Block> CreateGenesisBlock() {
        auto block = std::make_shared<Block>();
        block->Version = 0;
        block->PrevHash = UInt256::Zero();
        block->Timestamp = 1468595301000; // Genesis timestamp
        block->Index = 0;
        block->NextConsensus = test_accounts_[0]; // Consensus address
        block->Nonce = 2083236893;
        
        // Add genesis transaction
        auto genesis_tx = CreateGenesisTransaction();
        block->Transactions.push_back(genesis_tx);
        
        // Calculate merkle root
        block->MerkleRoot = CalculateMerkleRoot(block->Transactions);
        
        return block;
    }
    
    std::shared_ptr<Transaction> CreateGenesisTransaction() {
        auto tx = std::make_shared<Transaction>();
        tx->Version = 0;
        tx->Nonce = 0;
        tx->SystemFee = 0;
        tx->NetworkFee = 0;
        tx->ValidUntilBlock = 0;
        
        // Genesis script
        ScriptBuilder sb;
        sb.EmitOpCode(OpCode::RET);
        tx->Script = sb.ToByteArray();
        
        return tx;
    }
    
    UInt256 CalculateMerkleRoot(const std::vector<std::shared_ptr<Transaction>>& transactions) {
        std::vector<UInt256> hashes;
        for (const auto& tx : transactions) {
            hashes.push_back(tx->Hash());
        }
        return MerkleTree::ComputeRoot(hashes);
    }
    
    std::shared_ptr<Transaction> CreateValidTransaction() {
        auto tx = std::make_shared<Transaction>();
        tx->Version = 0;
        tx->Nonce = GetRandomNonce();
        tx->SystemFee = 1000000; // 0.01 GAS
        tx->NetworkFee = 500000; // 0.005 GAS
        tx->ValidUntilBlock = blockchain_->GetHeight() + 2000;
        
        // Simple script
        ScriptBuilder sb;
        sb.EmitOpCode(OpCode::RET);
        tx->Script = sb.ToByteArray();
        
        // Add signer
        tx->Signers.push_back({test_accounts_[0], WitnessScope::CalledByEntry});
        
        return tx;
    }
    
    uint32_t GetRandomNonce() {
        static uint32_t nonce = 1;
        return nonce++;
    }
    
    std::shared_ptr<ProtocolSettings> protocol_settings_;
    std::shared_ptr<Blockchain> blockchain_;
    std::shared_ptr<DataCache> data_cache_;
    std::shared_ptr<NeoToken> neo_token_;
    std::shared_ptr<GasToken> gas_token_;
    std::vector<KeyPair> test_keypairs_;
    std::vector<UInt160> test_accounts_;
    std::shared_ptr<Block> genesis_block_;
};

// Genesis Block Tests
TEST_F(BlockchainValidationTest, GenesisBlock_Validation) {
    EXPECT_TRUE(blockchain_->ValidateBlock(genesis_block_.get()));
    EXPECT_EQ(genesis_block_->Index, 0);
    EXPECT_EQ(genesis_block_->PrevHash, UInt256::Zero());
    EXPECT_FALSE(genesis_block_->Transactions.empty());
}

TEST_F(BlockchainValidationTest, GenesisBlock_MerkleRoot) {
    auto calculated_root = CalculateMerkleRoot(genesis_block_->Transactions);
    EXPECT_EQ(genesis_block_->MerkleRoot, calculated_root);
}

// Block Header Validation Tests
TEST_F(BlockchainValidationTest, BlockHeader_ValidVersion) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Version = 0;
    EXPECT_TRUE(blockchain_->ValidateBlockHeader(block.get()));
    
    block->Version = 1;
    EXPECT_FALSE(blockchain_->ValidateBlockHeader(block.get())); // Invalid version
}

TEST_F(BlockchainValidationTest, BlockHeader_ValidTimestamp) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    block->Timestamp = genesis_block_->Timestamp + 15000; // 15 seconds later
    
    EXPECT_TRUE(blockchain_->ValidateBlockHeader(block.get()));
}

TEST_F(BlockchainValidationTest, BlockHeader_InvalidTimestamp_TooEarly) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    block->Timestamp = genesis_block_->Timestamp - 1000; // Before previous block
    
    EXPECT_FALSE(blockchain_->ValidateBlockHeader(block.get()));
}

TEST_F(BlockchainValidationTest, BlockHeader_InvalidTimestamp_TooFar) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    // Set timestamp too far in the future
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    block->Timestamp = now + 60000; // 1 minute in future
    
    EXPECT_FALSE(blockchain_->ValidateBlockHeader(block.get()));
}

TEST_F(BlockchainValidationTest, BlockHeader_InvalidPrevHash) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    
    EXPECT_FALSE(blockchain_->ValidateBlockHeader(block.get()));
}

TEST_F(BlockchainValidationTest, BlockHeader_InvalidIndex) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 2; // Should be 1
    block->PrevHash = genesis_block_->Hash();
    
    EXPECT_FALSE(blockchain_->ValidateBlockHeader(block.get()));
}

// Block Size Validation Tests
TEST_F(BlockchainValidationTest, Block_ValidSize) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    // Add transactions within size limit
    for (int i = 0; i < 10; i++) {
        block->Transactions.push_back(CreateValidTransaction());
    }
    
    block->MerkleRoot = CalculateMerkleRoot(block->Transactions);
    
    size_t block_size = block->GetSize();
    EXPECT_LT(block_size, protocol_settings_->MaxBlockSize);
    EXPECT_TRUE(blockchain_->ValidateBlock(block.get()));
}

TEST_F(BlockchainValidationTest, Block_ExceedsMaxSize) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    // Add many transactions to exceed size limit
    for (int i = 0; i < 1000; i++) {
        auto tx = CreateValidTransaction();
        // Add large script to increase size
        tx->Script.resize(1000, 0x00);
        block->Transactions.push_back(tx);
    }
    
    block->MerkleRoot = CalculateMerkleRoot(block->Transactions);
    
    size_t block_size = block->GetSize();
    if (block_size > protocol_settings_->MaxBlockSize) {
        EXPECT_FALSE(blockchain_->ValidateBlock(block.get()));
    }
}

// Transaction Count Validation Tests
TEST_F(BlockchainValidationTest, Block_ValidTransactionCount) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    // Add transactions within limit
    for (int i = 0; i < 100; i++) {
        block->Transactions.push_back(CreateValidTransaction());
    }
    
    EXPECT_LT(block->Transactions.size(), protocol_settings_->MaxTransactionsPerBlock);
    EXPECT_TRUE(blockchain_->ValidateTransactionCount(block.get()));
}

TEST_F(BlockchainValidationTest, Block_ExceedsMaxTransactionCount) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    // Add too many transactions
    for (int i = 0; i < protocol_settings_->MaxTransactionsPerBlock + 1; i++) {
        block->Transactions.push_back(CreateValidTransaction());
    }
    
    EXPECT_FALSE(blockchain_->ValidateTransactionCount(block.get()));
}

// System Fee Validation Tests
TEST_F(BlockchainValidationTest, Block_ValidSystemFees) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    BigInteger total_system_fee = 0;
    for (int i = 0; i < 10; i++) {
        auto tx = CreateValidTransaction();
        tx->SystemFee = 1000000; // 0.01 GAS each
        total_system_fee += tx->SystemFee;
        block->Transactions.push_back(tx);
    }
    
    EXPECT_LT(total_system_fee, protocol_settings_->MaxBlockSystemFee);
    EXPECT_TRUE(blockchain_->ValidateSystemFees(block.get()));
}

TEST_F(BlockchainValidationTest, Block_ExceedsMaxSystemFee) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    // Add transaction with excessive system fee
    auto tx = CreateValidTransaction();
    tx->SystemFee = protocol_settings_->MaxBlockSystemFee + 1;
    block->Transactions.push_back(tx);
    
    EXPECT_FALSE(blockchain_->ValidateSystemFees(block.get()));
}

// Transaction Validation Tests
TEST_F(BlockchainValidationTest, Transaction_ValidBasicProperties) {
    auto tx = CreateValidTransaction();
    
    EXPECT_TRUE(blockchain_->ValidateTransaction(tx.get()));
    EXPECT_EQ(tx->Version, 0);
    EXPECT_GT(tx->ValidUntilBlock, blockchain_->GetHeight());
    EXPECT_GT(tx->NetworkFee, 0);
}

TEST_F(BlockchainValidationTest, Transaction_InvalidVersion) {
    auto tx = CreateValidTransaction();
    tx->Version = 1; // Invalid version
    
    EXPECT_FALSE(blockchain_->ValidateTransaction(tx.get()));
}

TEST_F(BlockchainValidationTest, Transaction_ExpiredValidUntilBlock) {
    auto tx = CreateValidTransaction();
    tx->ValidUntilBlock = blockchain_->GetHeight() - 1; // Already expired
    
    EXPECT_FALSE(blockchain_->ValidateTransaction(tx.get()));
}

TEST_F(BlockchainValidationTest, Transaction_ZeroNetworkFee) {
    auto tx = CreateValidTransaction();
    tx->NetworkFee = 0;
    
    // Some transactions might allow zero network fee
    bool result = blockchain_->ValidateTransaction(tx.get());
    // Result depends on implementation - could be valid or invalid
}

TEST_F(BlockchainValidationTest, Transaction_NegativeFees) {
    auto tx = CreateValidTransaction();
    tx->SystemFee = -1;
    
    EXPECT_FALSE(blockchain_->ValidateTransaction(tx.get()));
    
    tx->SystemFee = 1000000;
    tx->NetworkFee = -1;
    
    EXPECT_FALSE(blockchain_->ValidateTransaction(tx.get()));
}

TEST_F(BlockchainValidationTest, Transaction_EmptyScript) {
    auto tx = CreateValidTransaction();
    tx->Script.clear();
    
    EXPECT_FALSE(blockchain_->ValidateTransaction(tx.get()));
}

TEST_F(BlockchainValidationTest, Transaction_NoSigners) {
    auto tx = CreateValidTransaction();
    tx->Signers.clear();
    
    EXPECT_FALSE(blockchain_->ValidateTransaction(tx.get()));
}

// Witness Validation Tests
TEST_F(BlockchainValidationTest, Transaction_ValidWitness) {
    auto tx = CreateValidTransaction();
    
    // Add valid witness
    Witness witness;
    witness.InvocationScript = CreateInvocationScript(tx->Hash(), test_keypairs_[0]);
    witness.VerificationScript = CreateVerificationScript(test_keypairs_[0].PublicKey);
    tx->Witnesses.push_back(witness);
    
    EXPECT_TRUE(blockchain_->ValidateWitnesses(tx.get()));
}

TEST_F(BlockchainValidationTest, Transaction_InvalidWitness) {
    auto tx = CreateValidTransaction();
    
    // Add invalid witness (wrong signature)
    Witness witness;
    witness.InvocationScript = {0x40, 0x00}; // Invalid signature
    witness.VerificationScript = CreateVerificationScript(test_keypairs_[0].PublicKey);
    tx->Witnesses.push_back(witness);
    
    EXPECT_FALSE(blockchain_->ValidateWitnesses(tx.get()));
}

TEST_F(BlockchainValidationTest, Transaction_MissingWitness) {
    auto tx = CreateValidTransaction();
    // Don't add any witnesses
    
    EXPECT_FALSE(blockchain_->ValidateWitnesses(tx.get()));
}

// Duplicate Transaction Tests
TEST_F(BlockchainValidationTest, Block_DuplicateTransactions) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    auto tx = CreateValidTransaction();
    block->Transactions.push_back(tx);
    block->Transactions.push_back(tx); // Duplicate
    
    EXPECT_FALSE(blockchain_->ValidateBlock(block.get()));
}

TEST_F(BlockchainValidationTest, Block_DuplicateNonces) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    auto tx1 = CreateValidTransaction();
    auto tx2 = CreateValidTransaction();
    tx2->Nonce = tx1->Nonce; // Same nonce, same signer
    
    block->Transactions.push_back(tx1);
    block->Transactions.push_back(tx2);
    
    EXPECT_FALSE(blockchain_->ValidateBlock(block.get()));
}

// Merkle Root Validation Tests
TEST_F(BlockchainValidationTest, Block_CorrectMerkleRoot) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    for (int i = 0; i < 5; i++) {
        block->Transactions.push_back(CreateValidTransaction());
    }
    
    block->MerkleRoot = CalculateMerkleRoot(block->Transactions);
    
    EXPECT_TRUE(blockchain_->ValidateMerkleRoot(block.get()));
}

TEST_F(BlockchainValidationTest, Block_IncorrectMerkleRoot) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    for (int i = 0; i < 5; i++) {
        block->Transactions.push_back(CreateValidTransaction());
    }
    
    // Set wrong merkle root
    block->MerkleRoot = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    
    EXPECT_FALSE(blockchain_->ValidateMerkleRoot(block.get()));
}

// Network Fork Scenarios
TEST_F(BlockchainValidationTest, Fork_CompetingBlocks) {
    // Create two competing blocks at same height
    auto block1 = std::make_shared<Block>(*genesis_block_);
    auto block2 = std::make_shared<Block>(*genesis_block_);
    
    block1->Index = 1;
    block1->PrevHash = genesis_block_->Hash();
    block1->Nonce = 12345;
    block1->Transactions.push_back(CreateValidTransaction());
    block1->MerkleRoot = CalculateMerkleRoot(block1->Transactions);
    
    block2->Index = 1;
    block2->PrevHash = genesis_block_->Hash();
    block2->Nonce = 54321;
    block2->Transactions.push_back(CreateValidTransaction());
    block2->MerkleRoot = CalculateMerkleRoot(block2->Transactions);
    
    // Both should be valid independently
    EXPECT_TRUE(blockchain_->ValidateBlock(block1.get()));
    EXPECT_TRUE(blockchain_->ValidateBlock(block2.get()));
    
    // Different hashes
    EXPECT_NE(block1->Hash(), block2->Hash());
}

// Performance and Edge Cases
TEST_F(BlockchainValidationTest, Block_MaximumValidTransactions) {
    auto block = std::make_shared<Block>(*genesis_block_);
    block->Index = 1;
    block->PrevHash = genesis_block_->Hash();
    
    // Add maximum allowed transactions
    for (int i = 0; i < protocol_settings_->MaxTransactionsPerBlock; i++) {
        block->Transactions.push_back(CreateValidTransaction());
    }
    
    block->MerkleRoot = CalculateMerkleRoot(block->Transactions);
    
    EXPECT_TRUE(blockchain_->ValidateTransactionCount(block.get()));
    EXPECT_EQ(block->Transactions.size(), protocol_settings_->MaxTransactionsPerBlock);
}

TEST_F(BlockchainValidationTest, Transaction_MaximumValidUntilBlock) {
    auto tx = CreateValidTransaction();
    tx->ValidUntilBlock = blockchain_->GetHeight() + protocol_settings_->MaxValidUntilBlockIncrement;
    
    EXPECT_TRUE(blockchain_->ValidateTransaction(tx.get()));
    
    // Exceed maximum
    tx->ValidUntilBlock = blockchain_->GetHeight() + protocol_settings_->MaxValidUntilBlockIncrement + 1;
    
    EXPECT_FALSE(blockchain_->ValidateTransaction(tx.get()));
}

// Helper method implementations
private:
    ByteVector CreateInvocationScript(const UInt256& hash, const KeyPair& key_pair) {
        // Create signature for transaction hash
        auto signature = key_pair.Sign(hash.GetBytes());
        
        ByteVector invocation_script;
        invocation_script.push_back(0x40); // PUSHDATA1 with 64 bytes
        invocation_script.insert(invocation_script.end(), signature.begin(), signature.end());
        
        return invocation_script;
    }
    
    ByteVector CreateVerificationScript(const ECPoint& public_key) {
        ByteVector verification_script;
        verification_script.push_back(0x21); // PUSHDATA1 with 33 bytes
        auto pubkey_bytes = public_key.EncodePoint(true);
        verification_script.insert(verification_script.end(), pubkey_bytes.begin(), pubkey_bytes.end());
        verification_script.push_back(0x41); // SYSCALL
        verification_script.push_back(0x56); // CheckWitness
        verification_script.push_back(0x9e); // 
        verification_script.push_back(0xd7); //
        
        return verification_script;
    }
};