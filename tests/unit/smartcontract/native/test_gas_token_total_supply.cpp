#include <gtest/gtest.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/trigger_type.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::smartcontract;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;

class GasTokenTotalSupplyTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MemoryStore> store;
    std::shared_ptr<DataCache> snapshot;
    std::shared_ptr<GasToken> gasToken;
    std::shared_ptr<ApplicationEngine> engine;

    void SetUp() override
    {
        store = std::make_shared<MemoryStore>();
        snapshot = std::make_shared<StoreCache>(*store);
        gasToken = GasToken::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, nullptr, 0LL);
    }
};

// Test that GetTotalSupply returns 0 when storage is empty (not a hardcoded constant)
TEST_F(GasTokenTotalSupplyTest, TestEmptyStorageReturnsZero)
{
    // Fresh snapshot should have no data
    auto totalSupply = gasToken->GetTotalSupply(std::static_pointer_cast<StoreView>(snapshot));

    // Should return 0, not some hardcoded constant
    EXPECT_EQ(totalSupply, 0);
}

// Test that GetTotalSupply returns correct value after initialization
TEST_F(GasTokenTotalSupplyTest, TestInitializedTotalSupply)
{
    // Initialize the GAS token (this would normally happen during blockchain initialization)
    // For Neo N3, initial GAS supply is 52,000,000 GAS
    const int64_t initial_supply = 52000000LL * 100000000LL;  // Convert to Fixed8

    // Create the total supply storage key
    StorageKey key(gasToken->GetId(), ByteVector{11});  // Prefix 11 is for TotalSupply in NEP-17

    // Store the total supply
    std::ostringstream stream;
    BinaryWriter writer(stream);
    writer.Write(initial_supply);
    std::string data = stream.str();
    StorageItem item(ByteVector(reinterpret_cast<const uint8_t*>(data.data()), data.size()));

    snapshot->Add(key, item);
    snapshot->Commit();

    // Now GetTotalSupply should return the stored value
    auto totalSupply = gasToken->GetTotalSupply(std::static_pointer_cast<StoreView>(snapshot));
    EXPECT_EQ(totalSupply, initial_supply);
}

// Test that GetTotalSupply is consistent across multiple calls
TEST_F(GasTokenTotalSupplyTest, TestConsistentReads)
{
    // Set up some supply
    const int64_t test_supply = 12345678900000000LL;

    StorageKey key(gasToken->GetId(), ByteVector{11});

    std::ostringstream stream2;
    BinaryWriter writer(stream2);
    writer.Write(test_supply);
    std::string data2 = stream2.str();
    StorageItem item(ByteVector(reinterpret_cast<const uint8_t*>(data2.data()), data2.size()));

    snapshot->Add(key, item);
    snapshot->Commit();

    // Multiple reads should return the same value
    for (int i = 0; i < 10; ++i)
    {
        auto totalSupply = gasToken->GetTotalSupply(std::static_pointer_cast<StoreView>(snapshot));
        EXPECT_EQ(totalSupply, test_supply);
    }
}

// Test that GetTotalSupply handles malformed storage gracefully
TEST_F(GasTokenTotalSupplyTest, TestMalformedStorageData)
{
    // Create storage with invalid data
    StorageKey key(gasToken->GetId(), ByteVector{11});

    // Store invalid data (not enough bytes for int64)
    StorageItem item(ByteVector{1, 2, 3});  // Only 3 bytes, need 8

    snapshot->Add(key, item);
    snapshot->Commit();

    // Should handle gracefully (implementation dependent - might throw or return 0)
    try
    {
        auto totalSupply = gasToken->GetTotalSupply(std::static_pointer_cast<StoreView>(snapshot));
        // If it doesn't throw, it should return 0
        EXPECT_EQ(totalSupply, 0);
    }
    catch (const std::exception& e)
    {
        // Exception is acceptable for malformed data
        SUCCEED();
    }
}

// Test boundary values
TEST_F(GasTokenTotalSupplyTest, TestBoundaryValues)
{
    StorageKey key(gasToken->GetId(), ByteVector{11});

    // Test maximum possible supply
    {
        std::ostringstream stream3;
        BinaryWriter writer(stream3);
        writer.Write(INT64_MAX);
        std::string data3 = stream3.str();
        StorageItem item(ByteVector(reinterpret_cast<const uint8_t*>(data3.data()), data3.size()));

        snapshot->Add(key, item);
        snapshot->Commit();

        auto totalSupply = gasToken->GetTotalSupply(std::static_pointer_cast<StoreView>(snapshot));
        EXPECT_EQ(totalSupply, INT64_MAX);
    }

    // Test minimum possible supply (0)
    {
        std::ostringstream stream4;
        BinaryWriter writer(stream4);
        writer.Write(int64_t(0));
        std::string data4 = stream4.str();
        StorageItem item(ByteVector(reinterpret_cast<const uint8_t*>(data4.data()), data4.size()));

        snapshot->Add(key, item);
        snapshot->Commit();

        auto totalSupply = gasToken->GetTotalSupply(std::static_pointer_cast<StoreView>(snapshot));
        EXPECT_EQ(totalSupply, 0);
    }
}

// Test that the hardcoded TOTAL_SUPPLY constant is NOT used
TEST_F(GasTokenTotalSupplyTest, TestNoHardcodedConstant)
{
    // This test verifies our fix - GetTotalSupply should NOT return
    // a hardcoded constant when storage is empty

    // Create multiple fresh snapshots and verify they all return 0
    for (int i = 0; i < 5; ++i)
    {
        auto freshStore = std::make_shared<MemoryStore>();
        auto freshSnapshot = std::make_shared<StoreCache>(*freshStore);
        auto totalSupply = gasToken->GetTotalSupply(std::static_pointer_cast<StoreView>(freshSnapshot));

        // Should be 0, not some hardcoded value like 30_000_000 * Fixed8.One
        EXPECT_EQ(totalSupply, 0) << "Iteration " << i << " returned non-zero for empty storage";
    }
}