#pragma once

#include <neo/persistence/istore.h>
#include <neo/io/byte_vector.h>
#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>

#ifdef NEO_HAS_ROCKSDB
namespace rocksdb
{
    class DB;
    class Iterator;
    class Snapshot;
    class WriteBatch;
}
#endif

namespace neo::persistence
{
#ifdef NEO_HAS_ROCKSDB
    /**
     * @brief A RocksDB-based implementation of IStore.
     */
    class RocksDBStore : public IStore
    {
    public:
        /**
         * @brief Constructs a RocksDBStore.
         * @param path The path to the RocksDB database.
         */
        explicit RocksDBStore(const std::string& path);

        /**
         * @brief Destructor.
         */
        ~RocksDBStore() override;

        /**
         * @brief Tries to get a value from the store.
         * @param key The key to look up.
         * @return The value if found, std::nullopt otherwise.
         */
        std::optional<io::ByteVector> TryGet(const io::ByteVector& key) const override;

        /**
         * @brief Checks if the store contains a key.
         * @param key The key to check.
         * @return True if the key exists, false otherwise.
         */
        bool Contains(const io::ByteVector& key) const override;

        /**
         * @brief Finds all key-value pairs with keys that start with the specified prefix.
         * @param prefix The prefix to search for. If nullptr, all key-value pairs are returned.
         * @param direction The direction to seek.
         * @return The key-value pairs found.
         */
        std::vector<std::pair<io::ByteVector, io::ByteVector>> Find(const io::ByteVector* prefix = nullptr, SeekDirection direction = SeekDirection::Forward) const override;

        /**
         * @brief Puts a value in the store.
         * @param key The key.
         * @param value The value.
         */
        void Put(const io::ByteVector& key, const io::ByteVector& value) override;

        /**
         * @brief Puts a value in the store and syncs to disk.
         * @param key The key.
         * @param value The value.
         */
        void PutSync(const io::ByteVector& key, const io::ByteVector& value) override;

        /**
         * @brief Deletes a value from the store.
         * @param key The key.
         */
        void Delete(const io::ByteVector& key) override;

        /**
         * @brief Creates a snapshot of the store.
         * @return The snapshot.
         */
        std::unique_ptr<IStoreSnapshot> CreateSnapshot();

    private:
        std::unique_ptr<rocksdb::DB> db_;
        std::string path_;

        friend class RocksDBSnapshot;
    };

    /**
     * @brief A snapshot of a RocksDBStore.
     */
    class RocksDBSnapshot : public IStoreSnapshot
    {
    public:
        /**
         * @brief Constructs a RocksDBSnapshot.
         * @param store The store to snapshot.
         */
        explicit RocksDBSnapshot(RocksDBStore& store);

        /**
         * @brief Destructor.
         */
        ~RocksDBSnapshot() override;

        /**
         * @brief Tries to get a value from the store.
         * @param key The key to look up.
         * @return The value if found, std::nullopt otherwise.
         */
        std::optional<io::ByteVector> TryGet(const io::ByteVector& key) const override;

        /**
         * @brief Checks if the store contains a key.
         * @param key The key to check.
         * @return True if the key exists, false otherwise.
         */
        bool Contains(const io::ByteVector& key) const override;

        /**
         * @brief Finds all key-value pairs with keys that start with the specified prefix.
         * @param prefix The prefix to search for. If nullptr, all key-value pairs are returned.
         * @param direction The direction to seek.
         * @return The key-value pairs found.
         */
        std::vector<std::pair<io::ByteVector, io::ByteVector>> Find(const io::ByteVector* prefix = nullptr, SeekDirection direction = SeekDirection::Forward) const override;

        /**
         * @brief Puts a value in the store.
         * @param key The key.
         * @param value The value.
         */
        void Put(const io::ByteVector& key, const io::ByteVector& value) override;

        /**
         * @brief Deletes a value from the store.
         * @param key The key.
         */
        void Delete(const io::ByteVector& key) override;

        /**
         * @brief Commits the changes to the store.
         */
        void Commit() override;

        /**
         * @brief Gets the underlying store.
         * @return The underlying store.
         */
        IStore& GetStore() override;

    private:
        RocksDBStore& store_;
        const rocksdb::Snapshot* snapshot_;
        std::unique_ptr<rocksdb::WriteBatch> batch_;
    };

    /**
     * @brief A RocksDB-based implementation of IStoreProvider.
     */
    class RocksDBStoreProvider : public IStoreProvider
    {
    public:
        /**
         * @brief Constructs a RocksDBStoreProvider.
         */
        RocksDBStoreProvider();

        /**
         * @brief Gets the name of the store provider.
         * @return The name of the store provider.
         */
        std::string GetName() const override;

        /**
         * @brief Gets a store.
         * @param path The path to the store.
         * @return The store.
         */
        std::unique_ptr<IStore> GetStore(const std::string& path) override;

    private:
        std::unordered_map<std::string, std::shared_ptr<RocksDBStore>> stores_;
        std::mutex mutex_;
    };
#endif // NEO_HAS_ROCKSDB
}
