#include <gtest/gtest.h>
#include <neo/io/byte_vector.h>
#include <neo/persistence/store_factory.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <unordered_map>

namespace
{
class TempDirectory
{
   public:
    explicit TempDirectory(const std::string& prefix)
    {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() / (prefix + "_" + std::to_string(now));
        std::filesystem::create_directories(path_);
    }

    ~TempDirectory()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    const std::filesystem::path& Path() const { return path_; }

   private:
    std::filesystem::path path_;
};

neo::io::ByteVector MakeKey(std::initializer_list<uint8_t> data) { return neo::io::ByteVector(data); }
neo::io::ByteVector MakeValue(std::initializer_list<uint8_t> data) { return neo::io::ByteVector(data); }
}  // namespace

namespace neo::persistence::tests
{
TEST(StoreFactorySelection, MemoryProviderFallback)
{
    auto memory_provider = StoreFactory::get_store_provider("memory");
    ASSERT_NE(memory_provider, nullptr);
    EXPECT_EQ("MemoryStore", memory_provider->GetName());

    auto unknown_provider = StoreFactory::get_store_provider("unknown-backend");
    ASSERT_NE(unknown_provider, nullptr);
    EXPECT_EQ("MemoryStore", unknown_provider->GetName());

    // Memory provider returns new stores on demand; ensure basic put/get works.
    auto memory_store = memory_provider->GetStore({});
    ASSERT_NE(memory_store, nullptr);

    const auto key = MakeKey({0x01, 0x02});
    const auto value = MakeValue({0xAA, 0xBB});
    memory_store->Put(key, value);

    const auto roundtrip = memory_store->TryGet(key);
    ASSERT_TRUE(roundtrip.has_value());
    EXPECT_EQ(value, *roundtrip);
}

#ifdef NEO_HAS_LEVELDB
TEST(StoreFactorySelection, LevelDbProviderRoundTrip)
{
    TempDirectory temp_dir("neo_leveldb_store_factory");
    auto leveldb_provider = StoreFactory::get_store_provider("leveldb");
    ASSERT_NE(leveldb_provider, nullptr);
    EXPECT_EQ("LevelDB", leveldb_provider->GetName());

    auto store = leveldb_provider->GetStore(temp_dir.Path().string());
    ASSERT_NE(store, nullptr);

    const auto key = MakeKey({0x10, 0x20});
    const auto value = MakeValue({0x0A, 0x0B, 0x0C});
    store->Put(key, value);

    auto fetched = store->TryGet(key);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(value, *fetched);

    // Close and reopen to ensure the data persisted on disk.
    store.reset();
    auto reopened = leveldb_provider->GetStore(temp_dir.Path().string());
    ASSERT_NE(reopened, nullptr);

    fetched = reopened->TryGet(key);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(value, *fetched);
}
#endif  // NEO_HAS_LEVELDB

#ifdef NEO_HAS_ROCKSDB
TEST(StoreFactorySelection, RocksDbProviderRoundTrip)
{
    TempDirectory temp_dir("neo_rocksdb_store_factory");
    std::unordered_map<std::string, std::string> config{
        {"db_path", temp_dir.Path().string()},
        {"compression_enabled", "false"},
        {"use_bloom_filter", "false"},
    };

    auto rocksdb_provider = StoreFactory::get_store_provider("rocksdb", config);
    ASSERT_NE(rocksdb_provider, nullptr);
    EXPECT_EQ("RocksDB", rocksdb_provider->GetName());

    auto store = rocksdb_provider->GetStore({});
    ASSERT_NE(store, nullptr);

    const auto key = MakeKey({0x30, 0x40});
    const auto value = MakeValue({0xDE, 0xAD, 0xBE, 0xEF});
    store->Put(key, value);

    auto fetched = store->TryGet(key);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(value, *fetched);

    // Close first store/provider to release handles cleanly.
    store.reset();
    rocksdb_provider.reset();

    auto reopened_provider = StoreFactory::get_store_provider("rocksdb", config);
    ASSERT_NE(reopened_provider, nullptr);

    auto reopened = reopened_provider->GetStore({});
    ASSERT_NE(reopened, nullptr);

    fetched = reopened->TryGet(key);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(value, *fetched);

    reopened.reset();
    reopened_provider.reset();
}
#else
TEST(StoreFactorySelection, RocksDbProviderRoundTrip)
{
    GTEST_SKIP() << "RocksDB backend not compiled in.";
}
#endif  // NEO_HAS_ROCKSDB

TEST(StoreFactorySelection, FileStoreRoundTrip)
{
    TempDirectory temp_dir("neo_file_store_factory");
    const auto file_path = (temp_dir.Path() / "filestore_unit.dat").string();

    std::unordered_map<std::string, std::string> config{
        {"db_path", file_path},
    };

    auto file_provider = StoreFactory::get_store_provider("file", config);
    ASSERT_NE(file_provider, nullptr);
    EXPECT_EQ("FileStoreProvider", file_provider->GetName());

    auto store = file_provider->GetStore({});
    ASSERT_NE(store, nullptr);

    const auto key = MakeKey({0x55});
    const auto value = MakeValue({0xFA, 0xCE});
    store->Put(key, value);

    auto fetched = store->TryGet(key);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(value, *fetched);

    store.reset();
    file_provider.reset();

    auto reopened_provider = StoreFactory::get_store_provider("file", config);
    ASSERT_NE(reopened_provider, nullptr);

    auto reopened = reopened_provider->GetStore({});
    ASSERT_NE(reopened, nullptr);

    fetched = reopened->TryGet(key);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(value, *fetched);
}
}  // namespace neo::persistence::tests
