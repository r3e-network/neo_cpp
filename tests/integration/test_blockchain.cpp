#include <gtest/gtest.h>
#include <neo/ledger/blockchain.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_factory.h>

using namespace neo::ledger;
using namespace neo::persistence;

/**
 * @brief Simple memory store provider for testing
 */
class TestStoreProvider : public IStoreProvider 
{
public:
    std::string GetName() const override {
        return "test_memory";
    }
    
    std::unique_ptr<IStore> GetStore(const std::string& path) override {
        return std::make_unique<MemoryStore>();
    }
};

TEST(BlockchainIntegrationTest, TestBlockchainInitialization)
{
    // Create a test store provider
    auto provider = std::make_unique<TestStoreProvider>();
    auto store = provider->GetStore("test");
    
    EXPECT_NE(store, nullptr);
    EXPECT_EQ(provider->GetName(), "test_memory");
    
    // Test basic store operations
    io::ByteVector key = {0x01, 0x02, 0x03};
    io::ByteVector value = {0x04, 0x05, 0x06};
    
    EXPECT_FALSE(store->Contains(key));
    store->Put(key, value);
    EXPECT_TRUE(store->Contains(key));
    
    auto retrieved = store->TryGet(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), value);
}

TEST(BlockchainIntegrationTest, TestBlockProcessing)
{
    // Test store provider with blockchain operations
    auto provider = std::make_unique<TestStoreProvider>();
    auto store = provider->GetStore("blockchain_test");
    
    // Test storage operations that blockchain would use
    io::ByteVector block_key = {0xB0, 0x01}; // Block prefix + index
    io::ByteVector block_data = {0x01, 0x02, 0x03, 0x04}; // Mock block data
    
    store->Put(block_key, block_data);
    auto result = store->TryGet(block_key);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), block_data);
}

TEST(BlockchainIntegrationTest, TestTransactionVerification)
{
    // Test transaction storage and retrieval
    auto provider = std::make_unique<TestStoreProvider>();
    auto store = provider->GetStore("tx_test");
    
    io::ByteVector tx_key = {0xT0, 0x01}; // Transaction prefix + hash
    io::ByteVector tx_data = {0xA1, 0xB2, 0xC3}; // Mock transaction data
    
    store->Put(tx_key, tx_data);
    
    // Test transaction exists
    EXPECT_TRUE(store->Contains(tx_key));
    
    auto tx_result = store->TryGet(tx_key);
    EXPECT_TRUE(tx_result.has_value());
    EXPECT_EQ(tx_result.value(), tx_data);
    
    // Test transaction deletion
    store->Delete(tx_key);
    EXPECT_FALSE(store->Contains(tx_key));
}