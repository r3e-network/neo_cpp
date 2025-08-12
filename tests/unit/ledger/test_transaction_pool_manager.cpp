#include <gtest/gtest.h>
#include <neo/ledger/transaction_pool_manager.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <thread>
#include <chrono>

using namespace neo::ledger;
using namespace neo::network::p2p::payloads;

class TransactionPoolManagerTest : public ::testing::Test
{
protected:
    std::unique_ptr<TransactionPoolManager> pool_manager_;
    TransactionPoolManager::Configuration config_;
    
    void SetUp() override
    {
        config_.max_pool_size = 1000;
        config_.max_unverified_size = 100;
        config_.transaction_timeout = std::chrono::seconds(60);
        config_.cleanup_interval = std::chrono::seconds(5);
        config_.enable_priority_queue = true;
        config_.enable_conflict_detection = true;
        
        pool_manager_ = std::make_unique<TransactionPoolManager>(config_);
    }
    
    void TearDown() override
    {
        if (pool_manager_)
        {
            pool_manager_->Stop();
        }
    }
    
    Neo3Transaction CreateMockTransaction(uint64_t fee = 1000000)
    {
        Neo3Transaction tx;
        tx.SetNetworkFee(fee);
        tx.SetSystemFee(100000);
        tx.SetNonce(std::rand());
        return tx;
    }
};

TEST_F(TransactionPoolManagerTest, InitializationTest)
{
    EXPECT_EQ(pool_manager_->GetConfiguration().max_pool_size, 1000);
    EXPECT_EQ(pool_manager_->GetConfiguration().max_unverified_size, 100);
    
    auto stats = pool_manager_->GetStatistics();
    EXPECT_EQ(stats.total_transactions, 0);
    EXPECT_EQ(stats.verified_count, 0);
    EXPECT_EQ(stats.unverified_count, 0);
}

TEST_F(TransactionPoolManagerTest, AddTransactionTest)
{
    pool_manager_->Start();
    
    auto tx = CreateMockTransaction(1000000);
    bool result = pool_manager_->AddTransaction(tx, TransactionPoolManager::Priority::Normal, "peer1");
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(pool_manager_->ContainsTransaction(tx.GetHash()));
    
    auto stats = pool_manager_->GetStatistics();
    EXPECT_EQ(stats.total_transactions, 1);
    EXPECT_EQ(stats.total_fees, 1000000);
}

TEST_F(TransactionPoolManagerTest, DuplicateTransactionTest)
{
    pool_manager_->Start();
    
    auto tx = CreateMockTransaction();
    pool_manager_->AddTransaction(tx);
    
    // Try to add the same transaction again
    bool result = pool_manager_->AddTransaction(tx);
    EXPECT_FALSE(result);
    
    auto stats = pool_manager_->GetStatistics();
    EXPECT_EQ(stats.total_transactions, 1);
}

TEST_F(TransactionPoolManagerTest, RemoveTransactionTest)
{
    pool_manager_->Start();
    
    auto tx = CreateMockTransaction();
    pool_manager_->AddTransaction(tx);
    
    EXPECT_TRUE(pool_manager_->ContainsTransaction(tx.GetHash()));
    
    bool removed = pool_manager_->RemoveTransaction(tx.GetHash(), "Test removal");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(pool_manager_->ContainsTransaction(tx.GetHash()));
    
    // Try to remove again
    removed = pool_manager_->RemoveTransaction(tx.GetHash());
    EXPECT_FALSE(removed);
}

TEST_F(TransactionPoolManagerTest, PriorityOrderingTest)
{
    pool_manager_->Start();
    
    // Add transactions with different priorities and fees
    auto tx_low = CreateMockTransaction(100000);     // Low fee
    auto tx_normal = CreateMockTransaction(1000000);  // Normal fee
    auto tx_high = CreateMockTransaction(10000000);   // High fee
    auto tx_critical = CreateMockTransaction(100000000); // Critical fee
    
    pool_manager_->AddTransaction(tx_low, TransactionPoolManager::Priority::Low);
    pool_manager_->AddTransaction(tx_normal, TransactionPoolManager::Priority::Normal);
    pool_manager_->AddTransaction(tx_high, TransactionPoolManager::Priority::High);
    pool_manager_->AddTransaction(tx_critical, TransactionPoolManager::Priority::Critical);
    
    // Get transactions for block (should be ordered by priority/fee)
    auto block_txs = pool_manager_->GetTransactionsForBlock(10, 1024*1024);
    
    EXPECT_EQ(block_txs.size(), 4);
    // Critical priority should come first
    EXPECT_EQ(block_txs[0].GetHash(), tx_critical.GetHash());
}

TEST_F(TransactionPoolManagerTest, GetTransactionTest)
{
    pool_manager_->Start();
    
    auto tx = CreateMockTransaction();
    pool_manager_->AddTransaction(tx);
    
    auto retrieved = pool_manager_->GetTransaction(tx.GetHash());
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->GetHash(), tx.GetHash());
    
    // Try to get non-existent transaction
    Neo3Transaction fake_tx;
    fake_tx.SetNonce(99999);
    auto not_found = pool_manager_->GetTransaction(fake_tx.GetHash());
    EXPECT_FALSE(not_found.has_value());
}

TEST_F(TransactionPoolManagerTest, TransactionMetadataTest)
{
    pool_manager_->Start();
    
    auto tx = CreateMockTransaction(5000000);
    pool_manager_->AddTransaction(tx, TransactionPoolManager::Priority::High, "test_peer");
    
    auto metadata = pool_manager_->GetTransactionMetadata(tx.GetHash());
    EXPECT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->hash, tx.GetHash());
    EXPECT_EQ(metadata->priority, TransactionPoolManager::Priority::High);
    EXPECT_EQ(metadata->fee, 5000000);
    EXPECT_EQ(metadata->source_peer, "test_peer");
    EXPECT_FALSE(metadata->is_verified);
    EXPECT_EQ(metadata->retry_count, 0);
}

TEST_F(TransactionPoolManagerTest, ClearPoolTest)
{
    pool_manager_->Start();
    
    // Add multiple transactions
    for (int i = 0; i < 10; i++)
    {
        auto tx = CreateMockTransaction(1000000 * (i + 1));
        pool_manager_->AddTransaction(tx);
    }
    
    auto stats = pool_manager_->GetStatistics();
    EXPECT_EQ(stats.total_transactions, 10);
    
    pool_manager_->Clear("Test clear");
    
    stats = pool_manager_->GetStatistics();
    EXPECT_EQ(stats.total_transactions, 0);
    EXPECT_EQ(stats.total_fees, 0);
}

TEST_F(TransactionPoolManagerTest, ValidationCallbackTest)
{
    pool_manager_->Start();
    
    int validation_count = 0;
    pool_manager_->SetValidator([&validation_count](const Neo3Transaction& tx) {
        validation_count++;
        return tx.GetNetworkFee() >= 1000000;  // Validate if fee >= 1M
    });
    
    auto tx_valid = CreateMockTransaction(2000000);
    auto tx_invalid = CreateMockTransaction(500000);
    
    pool_manager_->AddTransaction(tx_valid);
    pool_manager_->AddTransaction(tx_invalid);
    
    size_t validated = pool_manager_->ValidateUnverifiedTransactions();
    EXPECT_GT(validation_count, 0);
}

TEST_F(TransactionPoolManagerTest, CallbacksTest)
{
    pool_manager_->Start();
    
    bool added_called = false;
    bool removed_called = false;
    bool stats_called = false;
    
    pool_manager_->SetOnTransactionAdded([&added_called](const neo::io::UInt256& hash, const std::string& peer) {
        added_called = true;
    });
    
    pool_manager_->SetOnTransactionRemoved([&removed_called](const neo::io::UInt256& hash, const std::string& reason) {
        removed_called = true;
    });
    
    pool_manager_->SetOnStatsUpdated([&stats_called](const TransactionPoolManager::PoolStats& stats) {
        stats_called = true;
    });
    
    auto tx = CreateMockTransaction();
    pool_manager_->AddTransaction(tx);
    EXPECT_TRUE(added_called);
    
    pool_manager_->RemoveTransaction(tx.GetHash());
    EXPECT_TRUE(removed_called);
}

TEST_F(TransactionPoolManagerTest, ConfigurationUpdateTest)
{
    pool_manager_->Start();
    
    TransactionPoolManager::Configuration new_config;
    new_config.max_pool_size = 2000;
    new_config.min_fee_threshold = 1000000;
    
    pool_manager_->UpdateConfiguration(new_config);
    
    EXPECT_EQ(pool_manager_->GetConfiguration().max_pool_size, 2000);
    EXPECT_EQ(pool_manager_->GetConfiguration().min_fee_threshold, 1000000);
    
    // Test that min fee threshold is enforced
    auto tx_low_fee = CreateMockTransaction(500000);  // Below threshold
    bool result = pool_manager_->AddTransaction(tx_low_fee);
    EXPECT_FALSE(result);  // Should be rejected
    
    auto tx_high_fee = CreateMockTransaction(2000000);  // Above threshold
    result = pool_manager_->AddTransaction(tx_high_fee);
    EXPECT_TRUE(result);  // Should be accepted
}

TEST_F(TransactionPoolManagerTest, GetTransactionsForBlockTest)
{
    pool_manager_->Start();
    
    // Add transactions of different sizes
    std::vector<Neo3Transaction> transactions;
    for (int i = 0; i < 20; i++)
    {
        auto tx = CreateMockTransaction(1000000 * (i + 1));
        transactions.push_back(tx);
        pool_manager_->AddTransaction(tx);
    }
    
    // Get limited number of transactions
    auto block_txs = pool_manager_->GetTransactionsForBlock(10, 1024*1024);
    EXPECT_LE(block_txs.size(), 10);
    
    // Verify transactions are unique
    std::set<neo::io::UInt256> unique_hashes;
    for (const auto& tx : block_txs)
    {
        unique_hashes.insert(tx.GetHash());
    }
    EXPECT_EQ(unique_hashes.size(), block_txs.size());
}

TEST_F(TransactionPoolManagerTest, StatisticsTest)
{
    pool_manager_->Start();
    
    // Add various transactions
    for (int i = 0; i < 5; i++)
    {
        auto tx = CreateMockTransaction(1000000 * (i + 1));
        pool_manager_->AddTransaction(tx);
    }
    
    auto stats = pool_manager_->GetStatistics();
    
    EXPECT_EQ(stats.total_transactions, 5);
    EXPECT_EQ(stats.total_fees, 15000000);  // 1+2+3+4+5 million
    EXPECT_EQ(stats.average_fee, 3000000);  // 15M / 5
    EXPECT_EQ(stats.rejected_count, 0);
    EXPECT_GT(stats.memory_usage_bytes, 0);
}

TEST_F(TransactionPoolManagerTest, ConcurrencyTest)
{
    pool_manager_->Start();
    
    const int num_threads = 10;
    const int txs_per_thread = 100;
    std::vector<std::thread> threads;
    
    // Spawn multiple threads adding transactions
    for (int t = 0; t < num_threads; t++)
    {
        threads.emplace_back([this, t, txs_per_thread]() {
            for (int i = 0; i < txs_per_thread; i++)
            {
                auto tx = CreateMockTransaction(1000000 + t * 1000 + i);
                pool_manager_->AddTransaction(tx);
                
                // Occasionally remove a transaction
                if (i % 10 == 0 && i > 0)
                {
                    pool_manager_->RemoveTransaction(tx.GetHash());
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        thread.join();
    }
    
    // Verify pool is in consistent state
    auto stats = pool_manager_->GetStatistics();
    EXPECT_GT(stats.total_transactions, 0);
    EXPECT_LE(stats.total_transactions, num_threads * txs_per_thread);
}

// Main test runner
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}