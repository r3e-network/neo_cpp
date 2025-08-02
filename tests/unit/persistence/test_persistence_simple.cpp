#include <gtest/gtest.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/storage_key.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <vector>
#include <memory>

using namespace neo::persistence;
using namespace neo::io;

class PersistenceSimpleTest : public ::testing::Test {
protected:
    std::unique_ptr<MemoryStore> store;
    
    void SetUp() override {
        store = std::make_unique<MemoryStore>();
    }
    
    void TearDown() override {
        store.reset();
    }
};

TEST_F(PersistenceSimpleTest, MemoryStoreBasics)
{
    // Test memory store creation
    ASSERT_NE(store, nullptr);
    
    // Test getting snapshot
    auto snapshot = store->GetSnapshot();
    ASSERT_NE(snapshot, nullptr);
}

TEST_F(PersistenceSimpleTest, StoreAndRetrieveData)
{
    // Create a storage key
    StorageKey key(0x01, std::vector<uint8_t>{0x01, 0x02, 0x03});
    
    // Create test data
    std::vector<uint8_t> testData = {0x10, 0x20, 0x30, 0x40};
    
    // Get snapshot for writing
    auto writeSnapshot = store->GetSnapshot();
    ASSERT_NE(writeSnapshot, nullptr);
    
    // Write data
    writeSnapshot->Put(key.ToArray(), testData);
    writeSnapshot->Commit();
    
    // Get new snapshot for reading
    auto readSnapshot = store->GetSnapshot();
    ASSERT_NE(readSnapshot, nullptr);
    
    // Read data back
    auto readData = readSnapshot->TryGet(key.ToArray());
    ASSERT_TRUE(readData.has_value());
    EXPECT_EQ(readData.value(), testData);
}

TEST_F(PersistenceSimpleTest, DeleteData)
{
    // Create a storage key
    StorageKey key(0x02, std::vector<uint8_t>{0x04, 0x05});
    
    // Create test data
    std::vector<uint8_t> testData = {0x50, 0x60};
    
    // Write data
    auto writeSnapshot = store->GetSnapshot();
    writeSnapshot->Put(key.ToArray(), testData);
    writeSnapshot->Commit();
    
    // Verify data exists
    auto verifySnapshot = store->GetSnapshot();
    ASSERT_TRUE(verifySnapshot->TryGet(key.ToArray()).has_value());
    
    // Delete data
    auto deleteSnapshot = store->GetSnapshot();
    deleteSnapshot->Delete(key.ToArray());
    deleteSnapshot->Commit();
    
    // Verify data is deleted
    auto finalSnapshot = store->GetSnapshot();
    EXPECT_FALSE(finalSnapshot->TryGet(key.ToArray()).has_value());
}

TEST_F(PersistenceSimpleTest, MultipleKeys)
{
    std::vector<std::pair<StorageKey, std::vector<uint8_t>>> testPairs;
    
    // Create multiple key-value pairs
    for (int i = 0; i < 5; ++i) {
        StorageKey key(0x03, std::vector<uint8_t>{static_cast<uint8_t>(i)});
        std::vector<uint8_t> data = {static_cast<uint8_t>(i * 10), 
                                     static_cast<uint8_t>(i * 10 + 1)};
        testPairs.push_back({key, data});
    }
    
    // Write all data
    auto writeSnapshot = store->GetSnapshot();
    for (const auto& [key, data] : testPairs) {
        writeSnapshot->Put(key.ToArray(), data);
    }
    writeSnapshot->Commit();
    
    // Read and verify all data
    auto readSnapshot = store->GetSnapshot();
    for (const auto& [key, expectedData] : testPairs) {
        auto actualData = readSnapshot->TryGet(key.ToArray());
        ASSERT_TRUE(actualData.has_value());
        EXPECT_EQ(actualData.value(), expectedData);
    }
}

TEST_F(PersistenceSimpleTest, SnapshotIsolation)
{
    StorageKey key(0x04, std::vector<uint8_t>{0x01});
    std::vector<uint8_t> initialData = {0x01, 0x02};
    std::vector<uint8_t> modifiedData = {0x03, 0x04};
    
    // Write initial data
    auto initSnapshot = store->GetSnapshot();
    initSnapshot->Put(key.ToArray(), initialData);
    initSnapshot->Commit();
    
    // Get two snapshots
    auto snapshot1 = store->GetSnapshot();
    auto snapshot2 = store->GetSnapshot();
    
    // Modify data in snapshot1
    snapshot1->Put(key.ToArray(), modifiedData);
    
    // Data should still be initial in snapshot2
    auto data2 = snapshot2->TryGet(key.ToArray());
    ASSERT_TRUE(data2.has_value());
    EXPECT_EQ(data2.value(), initialData);
    
    // Commit snapshot1
    snapshot1->Commit();
    
    // New snapshot should see modified data
    auto snapshot3 = store->GetSnapshot();
    auto data3 = snapshot3->TryGet(key.ToArray());
    ASSERT_TRUE(data3.has_value());
    EXPECT_EQ(data3.value(), modifiedData);
}