#include <gtest/gtest.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>

using namespace neo::persistence;

class MemoryClonedCacheTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        memory_store_ = std::make_shared<MemoryStore>();
        InitializeCaches();
    }

    void InitializeCaches()
    {
        auto snapshot_unique = memory_store_->GetSnapshot();
        auto raw = dynamic_cast<MemorySnapshot*>(snapshot_unique.release());
        ASSERT_NE(raw, nullptr);
        snapshot_ = std::shared_ptr<MemorySnapshot>(raw);
        std::shared_ptr<IStoreSnapshot> base = snapshot_;
        snapshot_cache_ = std::make_shared<StoreCache>(base);
        data_cache_ = std::static_pointer_cast<StoreCache>(snapshot_cache_->CreateSnapshot());
    }

    StorageKey MakeKey(uint8_t suffix) { return StorageKey(0x42, neo::io::ByteVector{suffix}); }

    StorageItem MakeItem(std::initializer_list<uint8_t> bytes)
    {
        StorageItem item;
        item.SetValue(neo::io::ByteVector(bytes));
        return item;
    }

    std::shared_ptr<MemoryStore> memory_store_;
    std::shared_ptr<MemorySnapshot> snapshot_;
    std::shared_ptr<StoreCache> snapshot_cache_;
    std::shared_ptr<StoreCache> data_cache_;
};

TEST_F(MemoryClonedCacheTest, SingleSnapshotCacheBehavior)
{
    auto key = MakeKey(0x01);
    auto value = MakeItem({0x03, 0x04});
    EXPECT_FALSE(data_cache_->Contains(key));
    data_cache_->Add(key, value);
    EXPECT_TRUE(data_cache_->Contains(key));

    data_cache_->Commit();
    EXPECT_TRUE(snapshot_cache_->Contains(key));

    snapshot_cache_->Commit();
    InitializeCaches();
    EXPECT_TRUE(data_cache_->Contains(key));

    data_cache_->Delete(key);
    EXPECT_FALSE(data_cache_->Contains(key));

    data_cache_->Commit();
    EXPECT_FALSE(snapshot_cache_->Contains(key));

    snapshot_cache_->Commit();
    InitializeCaches();
    EXPECT_FALSE(data_cache_->Contains(key));
}
