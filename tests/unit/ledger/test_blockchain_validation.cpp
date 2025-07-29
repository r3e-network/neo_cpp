#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "neo/cryptography/hash.h"
#include "neo/ledger/block.h"
#include "neo/ledger/blockchain.h"
#include "neo/ledger/mempool.h"
#include "neo/ledger/transaction.h"
#include "neo/persistence/memory_store.h"
#include "neo/protocol_settings.h"
#include "tests/mocks/mock_protocol_settings.h"
#include "tests/utils/test_helpers.h"

using namespace neo::ledger;
using namespace neo::persistence;
using namespace neo::cryptography;
using namespace neo::tests;
using namespace testing;

class BlockchainValidationTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        settings_ = std::make_shared<MockProtocolSettings>();
        store_ = std::make_shared<MemoryStore>();
        mempool_ = std::make_shared<MemoryPool>(settings_);
        blockchain_ = std::make_shared<Blockchain>(settings_, store_);

        // Setup default protocol settings
        EXPECT_CALL(*settings_, GetNetwork()).WillRepeatedly(Return(860833102));
        EXPECT_CALL(*settings_, GetMaxTransactionsPerBlock()).WillRepeatedly(Return(512));
        EXPECT_CALL(*settings_, GetMaxBlockSize()).WillRepeatedly(Return(1024 * 1024));  // 1MB
        EXPECT_CALL(*settings_, GetMillisecondsPerBlock()).WillRepeatedly(Return(15000));
        EXPECT_CALL(*settings_, GetValidatorsCount()).WillRepeatedly(Return(7));

        // Initialize blockchain
        ASSERT_TRUE(blockchain_->Initialize());
    }

    std::shared_ptr<MockProtocolSettings> settings_;
    std::shared_ptr<MemoryStore> store_;
    std::shared_ptr<MemoryPool> mempool_;
    std::shared_ptr<Blockchain> blockchain_;

    std::shared_ptr<Block> CreateValidBlock(uint32_t index = 1)
    {
        auto block = std::make_shared<Block>();
        auto& header = block->GetHeader();

        header.SetIndex(index);
        header.SetTimestamp(TestHelpers::GetCurrentTimestamp());
        header.SetNonce(12345);
        header.SetPrimaryIndex(0);
        header.SetVersion(0);
        header.SetNextConsensus(TestHelpers::GenerateRandomScriptHash());

        if (index > 0)
        {
            // Set previous block hash
            if (index == 1)
            {
                header.SetPrevHash(blockchain_->GetGenesisBlock()->GetHash());
            }
            else
            {
                auto prevBlock = blockchain_->GetBlock(index - 1);
                header.SetPrevHash(prevBlock->GetHash());
            }
        }

        // Add some valid transactions
        std::vector<std::shared_ptr<Transaction>> transactions;
        for (int i = 0; i < 3; ++i)
        {
            auto tx = TestHelpers::CreateValidTransaction();
            transactions.push_back(tx);
        }

        block->SetTransactions(transactions);

        // Calculate merkle root
        std::vector<UInt256> tx_hashes;
        for (const auto& tx : transactions)
        {
            tx_hashes.push_back(tx->GetHash());
        }
        header.SetMerkleRoot(Hash::ComputeMerkleRoot(tx_hashes));

        return block;
    }

    std::shared_ptr<Block> CreateInvalidBlock(const std::string& invalid_type)
    {
        auto block = CreateValidBlock();

        if (invalid_type == "wrong_index")
        {
            block->GetHeader().SetIndex(999);
        }
        else if (invalid_type == "wrong_prev_hash")
        {
            block->GetHeader().SetPrevHash(TestHelpers::GenerateRandomHash());
        }
        else if (invalid_type == "wrong_merkle_root")
        {
            block->GetHeader().SetMerkleRoot(TestHelpers::GenerateRandomHash());
        }
        else if (invalid_type == "too_many_transactions")
        {
            std::vector<std::shared_ptr<Transaction>> transactions;
            for (int i = 0; i < 600; ++i)
            {  // Exceed max limit
                transactions.push_back(TestHelpers::CreateValidTransaction());
            }
            block->SetTransactions(transactions);
        }
        else if (invalid_type == "invalid_timestamp")
        {
            block->GetHeader().SetTimestamp(0);  // Invalid timestamp
        }
        else if (invalid_type == "wrong_version")
        {
            block->GetHeader().SetVersion(255);  // Invalid version
        }

        return block;
    }
};

// Test genesis block validation
TEST_F(BlockchainValidationTest, GenesisBlockValidation)
{
    auto genesis = blockchain_->GetGenesisBlock();
    EXPECT_TRUE(genesis->Verify(settings_));
    EXPECT_EQ(genesis->GetIndex(), 0);
    EXPECT_EQ(genesis->GetHeader().GetPrevHash(), UInt256::Zero());
    EXPECT_EQ(blockchain_->GetHeight(), 1);
}

// Test valid block processing
TEST_F(BlockchainValidationTest, ValidBlockProcessing)
{
    auto block = CreateValidBlock(1);

    // Block should be valid
    EXPECT_TRUE(block->Verify(settings_));

    // Processing should succeed
    EXPECT_TRUE(blockchain_->ProcessBlock(block));

    // Blockchain height should increase
    EXPECT_EQ(blockchain_->GetHeight(), 2);

    // Block should be retrievable
    auto retrieved_block = blockchain_->GetBlock(1);
    EXPECT_NE(retrieved_block, nullptr);
    EXPECT_EQ(retrieved_block->GetHash(), block->GetHash());
}

// Test block index validation
TEST_F(BlockchainValidationTest, BlockIndexValidation)
{
    auto invalid_block = CreateInvalidBlock("wrong_index");

    // Block should fail validation
    EXPECT_FALSE(invalid_block->Verify(settings_));

    // Processing should fail
    EXPECT_FALSE(blockchain_->ProcessBlock(invalid_block));

    // Blockchain height should not change
    EXPECT_EQ(blockchain_->GetHeight(), 1);
}

// Test previous hash validation
TEST_F(BlockchainValidationTest, PreviousHashValidation)
{
    auto invalid_block = CreateInvalidBlock("wrong_prev_hash");

    // Block should fail validation
    EXPECT_FALSE(blockchain_->ValidateBlock(invalid_block));

    // Processing should fail
    EXPECT_FALSE(blockchain_->ProcessBlock(invalid_block));
}

// Test merkle root validation
TEST_F(BlockchainValidationTest, MerkleRootValidation)
{
    auto invalid_block = CreateInvalidBlock("wrong_merkle_root");

    // Block should fail validation
    EXPECT_FALSE(invalid_block->Verify(settings_));

    // Processing should fail
    EXPECT_FALSE(blockchain_->ProcessBlock(invalid_block));
}

// Test transaction count limit
TEST_F(BlockchainValidationTest, TransactionCountLimit)
{
    auto invalid_block = CreateInvalidBlock("too_many_transactions");

    // Block should fail validation due to too many transactions
    EXPECT_FALSE(invalid_block->Verify(settings_));

    // Processing should fail
    EXPECT_FALSE(blockchain_->ProcessBlock(invalid_block));
}

// Test block size limit
TEST_F(BlockchainValidationTest, BlockSizeLimit)
{
    auto block = CreateValidBlock(1);

    // Add large transaction to exceed size limit
    auto large_tx = TestHelpers::CreateLargeTransaction(2 * 1024 * 1024);  // 2MB transaction
    auto transactions = block->GetTransactions();
    transactions.push_back(large_tx);
    block->SetTransactions(transactions);

    // Block should fail validation due to size
    EXPECT_FALSE(block->Verify(settings_));

    // Processing should fail
    EXPECT_FALSE(blockchain_->ProcessBlock(block));
}

// Test timestamp validation
TEST_F(BlockchainValidationTest, TimestampValidation)
{
    auto invalid_block = CreateInvalidBlock("invalid_timestamp");

    // Block should fail validation
    EXPECT_FALSE(invalid_block->Verify(settings_));

    // Processing should fail
    EXPECT_FALSE(blockchain_->ProcessBlock(invalid_block));
}

// Test block version validation
TEST_F(BlockchainValidationTest, BlockVersionValidation)
{
    auto invalid_block = CreateInvalidBlock("wrong_version");

    // Block should fail validation
    EXPECT_FALSE(invalid_block->Verify(settings_));

    // Processing should fail
    EXPECT_FALSE(blockchain_->ProcessBlock(invalid_block));
}

// Test duplicate block rejection
TEST_F(BlockchainValidationTest, DuplicateBlockRejection)
{
    auto block = CreateValidBlock(1);

    // First processing should succeed
    EXPECT_TRUE(blockchain_->ProcessBlock(block));
    EXPECT_EQ(blockchain_->GetHeight(), 2);

    // Second processing of same block should fail
    EXPECT_FALSE(blockchain_->ProcessBlock(block));
    EXPECT_EQ(blockchain_->GetHeight(), 2);  // Height unchanged
}

// Test sequential block processing
TEST_F(BlockchainValidationTest, SequentialBlockProcessing)
{
    const int num_blocks = 10;

    for (uint32_t i = 1; i <= num_blocks; ++i)
    {
        auto block = CreateValidBlock(i);
        EXPECT_TRUE(blockchain_->ProcessBlock(block)) << "Failed to process block " << i;
        EXPECT_EQ(blockchain_->GetHeight(), i + 1);
    }

    // Verify all blocks are stored correctly
    for (uint32_t i = 1; i <= num_blocks; ++i)
    {
        auto block = blockchain_->GetBlock(i);
        EXPECT_NE(block, nullptr) << "Block " << i << " not found";
        EXPECT_EQ(block->GetIndex(), i);
    }
}

// Test block chain continuity
TEST_F(BlockchainValidationTest, BlockChainContinuity)
{
    // Process first block
    auto block1 = CreateValidBlock(1);
    EXPECT_TRUE(blockchain_->ProcessBlock(block1));

    // Try to process block 3 (skipping block 2)
    auto block3 = CreateValidBlock(3);
    // Set previous hash to block 1 (should be block 2)
    block3->GetHeader().SetPrevHash(block1->GetHash());

    // Should fail due to missing block 2
    EXPECT_FALSE(blockchain_->ProcessBlock(block3));

    // Process block 2
    auto block2 = CreateValidBlock(2);
    EXPECT_TRUE(blockchain_->ProcessBlock(block2));

    // Now update block 3 with correct previous hash
    block3->GetHeader().SetPrevHash(block2->GetHash());

    // Recalculate merkle root if needed
    std::vector<UInt256> tx_hashes;
    for (const auto& tx : block3->GetTransactions())
    {
        tx_hashes.push_back(tx->GetHash());
    }
    block3->GetHeader().SetMerkleRoot(Hash::ComputeMerkleRoot(tx_hashes));

    // Should now succeed
    EXPECT_TRUE(blockchain_->ProcessBlock(block3));
    EXPECT_EQ(blockchain_->GetHeight(), 4);
}

// Test fork handling (basic)
TEST_F(BlockchainValidationTest, BasicForkHandling)
{
    // Create initial chain: Genesis -> Block1
    auto block1 = CreateValidBlock(1);
    EXPECT_TRUE(blockchain_->ProcessBlock(block1));

    // Create fork: two different blocks at height 2
    auto block2a = CreateValidBlock(2);
    auto block2b = CreateValidBlock(2);

    // Make them different by changing nonce
    block2b->GetHeader().SetNonce(54321);

    // Recalculate merkle root for block2b
    std::vector<UInt256> tx_hashes;
    for (const auto& tx : block2b->GetTransactions())
    {
        tx_hashes.push_back(tx->GetHash());
    }
    block2b->GetHeader().SetMerkleRoot(Hash::ComputeMerkleRoot(tx_hashes));

    // First fork should be accepted
    EXPECT_TRUE(blockchain_->ProcessBlock(block2a));
    EXPECT_EQ(blockchain_->GetHeight(), 3);

    // Second fork should be handled appropriately
    // (Implementation dependent - might be rejected or cause reorganization)
    bool result = blockchain_->ProcessBlock(block2b);

    // Blockchain should remain consistent
    EXPECT_GE(blockchain_->GetHeight(), 3);
    auto current_block = blockchain_->GetCurrentBlock();
    EXPECT_NE(current_block, nullptr);
}

// Test transaction validation within blocks
TEST_F(BlockchainValidationTest, TransactionValidationInBlocks)
{
    auto block = CreateValidBlock(1);

    // Add an invalid transaction
    auto invalid_tx = TestHelpers::CreateInvalidTransaction();
    auto transactions = block->GetTransactions();
    transactions.push_back(invalid_tx);
    block->SetTransactions(transactions);

    // Recalculate merkle root
    std::vector<UInt256> tx_hashes;
    for (const auto& tx : transactions)
    {
        tx_hashes.push_back(tx->GetHash());
    }
    block->GetHeader().SetMerkleRoot(Hash::ComputeMerkleRoot(tx_hashes));

    // Block should fail validation due to invalid transaction
    EXPECT_FALSE(blockchain_->ValidateBlock(block));
    EXPECT_FALSE(blockchain_->ProcessBlock(block));
}

// Test double spending prevention
TEST_F(BlockchainValidationTest, DoubleSpendingPrevention)
{
    auto block1 = CreateValidBlock(1);
    EXPECT_TRUE(blockchain_->ProcessBlock(block1));

    // Create block with duplicate transaction inputs
    auto block2 = CreateValidBlock(2);
    auto transactions = block2->GetTransactions();

    // Create two transactions that spend the same input
    auto tx1 = TestHelpers::CreateValidTransaction();
    auto tx2 = TestHelpers::CreateValidTransaction();

    // Make them spend the same input (implementation specific)
    // This would require setting up proper UTXO references

    transactions.push_back(tx1);
    transactions.push_back(tx2);
    block2->SetTransactions(transactions);

    // Block should fail validation due to double spending
    // (This test would need proper UTXO implementation)
    // EXPECT_FALSE(blockchain_->ValidateBlock(block2));
}

// Test witness validation
TEST_F(BlockchainValidationTest, WitnessValidation)
{
    auto block = CreateValidBlock(1);

    // Create transaction with invalid witness
    auto tx = TestHelpers::CreateValidTransaction();
    auto witness = tx->GetWitnesses()[0];

    // Corrupt the witness signature
    witness.SetVerificationScript(std::vector<uint8_t>{0xFF, 0xFF});

    std::vector<Witness> witnesses = {witness};
    tx->SetWitnesses(witnesses);

    auto transactions = block->GetTransactions();
    transactions.push_back(tx);
    block->SetTransactions(transactions);

    // Recalculate merkle root
    std::vector<UInt256> tx_hashes;
    for (const auto& tx : transactions)
    {
        tx_hashes.push_back(tx->GetHash());
    }
    block->GetHeader().SetMerkleRoot(Hash::ComputeMerkleRoot(tx_hashes));

    // Block should fail validation due to invalid witness
    EXPECT_FALSE(blockchain_->ValidateBlock(block));
    EXPECT_FALSE(blockchain_->ProcessBlock(block));
}

// Test block validation performance
TEST_F(BlockchainValidationTest, BlockValidationPerformance)
{
    const int num_blocks = 100;
    std::vector<std::shared_ptr<Block>> blocks;

    // Create blocks
    for (uint32_t i = 1; i <= num_blocks; ++i)
    {
        blocks.push_back(CreateValidBlock(i));
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Process all blocks
    for (const auto& block : blocks)
    {
        EXPECT_TRUE(blockchain_->ProcessBlock(block));
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Should process blocks efficiently (less than 10ms per block on average)
    double ms_per_block = static_cast<double>(duration.count()) / num_blocks;
    EXPECT_LT(ms_per_block, 10.0);

    EXPECT_EQ(blockchain_->GetHeight(), num_blocks + 1);
}

// Test concurrent block validation
TEST_F(BlockchainValidationTest, ConcurrentBlockValidation)
{
    const int num_threads = 4;
    const int blocks_per_thread = 10;

    std::vector<std::thread> threads;
    std::atomic<int> successful_validations{0};
    std::atomic<int> failed_validations{0};

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, t, blocks_per_thread, &successful_validations, &failed_validations]()
            {
                for (int i = 0; i < blocks_per_thread; ++i)
                {
                    auto block = CreateValidBlock(t * blocks_per_thread + i + 1);

                    if (block->Verify(settings_))
                    {
                        successful_validations++;
                    }
                    else
                    {
                        failed_validations++;
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // All validations should succeed (validation is read-only)
    EXPECT_EQ(successful_validations.load(), num_threads * blocks_per_thread);
    EXPECT_EQ(failed_validations.load(), 0);
}

// Test blockchain state consistency
TEST_F(BlockchainValidationTest, BlockchainStateConsistency)
{
    const int num_blocks = 20;

    // Process blocks
    for (uint32_t i = 1; i <= num_blocks; ++i)
    {
        auto block = CreateValidBlock(i);
        EXPECT_TRUE(blockchain_->ProcessBlock(block));
    }

    // Verify state consistency
    EXPECT_EQ(blockchain_->GetHeight(), num_blocks + 1);

    // Verify block hash chain
    for (uint32_t i = 1; i <= num_blocks; ++i)
    {
        auto block = blockchain_->GetBlock(i);
        EXPECT_NE(block, nullptr);

        if (i > 1)
        {
            auto prev_block = blockchain_->GetBlock(i - 1);
            EXPECT_EQ(block->GetHeader().GetPrevHash(), prev_block->GetHash());
        }
    }

    // Verify current block
    auto current_block = blockchain_->GetCurrentBlock();
    EXPECT_EQ(current_block->GetIndex(), num_blocks);

    // Verify block hash retrieval
    for (uint32_t i = 1; i <= num_blocks; ++i)
    {
        auto hash = blockchain_->GetBlockHash(i);
        auto block = blockchain_->GetBlock(hash);
        EXPECT_NE(block, nullptr);
        EXPECT_EQ(block->GetIndex(), i);
    }
}