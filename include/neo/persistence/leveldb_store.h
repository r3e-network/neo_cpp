#pragma once

#include <neo/persistence/istore.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <leveldb/db.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace neo::persistence
{
    /**
     * @brief LevelDB implementation of the storage interface.
     * 
     * This class provides persistent storage using Google's LevelDB database,
     * which is optimized for blockchain data access patterns.
     */
    class LevelDBStore : public IStore
    {
    public:
        /**
         * @brief Constructs a LevelDB store with the specified path.
         * @param path The database path
         */
        explicit LevelDBStore(const std::string& path);

        /**
         * @brief Destructor - closes the database
         */
        ~LevelDBStore() override;

        /**
         * @brief Opens the database connection
         * @return true if successful, false otherwise
         */
        bool Start();

        /**
         * @brief Closes the database connection
         */
        void Stop();

        /**
         * @brief Checks if the database is open
         * @return true if open, false otherwise
         */
        bool IsOpen() const;

        // IStore interface implementation
        
        /**
         * @brief Deletes a key-value pair
         * @param key The key to delete
         */
        void Delete(const StorageKey& key) override;

        /**
         * @brief Gets a value by key
         * @param key The key to lookup
         * @return The storage item, or nullptr if not found
         */
        std::shared_ptr<StorageItem> Get(const StorageKey& key) override;

        /**
         * @brief Puts a key-value pair
         * @param key The key
         * @param value The value
         */
        void Put(const StorageKey& key, const StorageItem& value) override;

        /**
         * @brief Puts a key-value pair using raw bytes
         * @param key The key bytes
         * @param value The value bytes
         */
        void PutBytes(const io::ByteVector& key, const io::ByteVector& value) override;

        /**
         * @brief Checks if a key exists
         * @param key The key to check
         * @return true if key exists, false otherwise
         */
        bool Contains(const StorageKey& key) override;

        /**
         * @brief Gets all storage items with keys starting with the prefix
         * @param prefix The key prefix
         * @return Vector of key-value pairs
         */
        std::vector<std::pair<StorageKey, std::shared_ptr<StorageItem>>> 
            Find(const io::ByteVector& prefix) override;

        /**
         * @brief Gets storage items in a key range
         * @param startKey The start key (inclusive)
         * @param endKey The end key (exclusive)
         * @return Vector of key-value pairs
         */
        std::vector<std::pair<StorageKey, std::shared_ptr<StorageItem>>> 
            FindRange(const StorageKey& startKey, const StorageKey& endKey) override;

        /**
         * @brief Creates a snapshot of the current database state
         * @return Snapshot object
         */
        std::unique_ptr<ISnapshot> GetSnapshot() override;

        /**
         * @brief Commits a batch of changes atomically
         * @param batch The batch of changes
         */
        void Commit(const std::vector<std::pair<StorageKey, std::shared_ptr<StorageItem>>>& batch) override;

        /**
         * @brief Gets database statistics
         * @return JSON string with database stats
         */
        std::string GetDatabaseStats() const;

        /**
         * @brief Compacts the database to optimize storage
         */
        void CompactDatabase();

        /**
         * @brief Gets the estimated database size in bytes
         * @return Database size estimate
         */
        size_t GetEstimatedSize() const;

        /**
         * @brief Repairs a potentially corrupted database
         * @param path The database path
         * @return true if repair successful, false otherwise
         */
        static bool RepairDatabase(const std::string& path);

        /**
         * @brief Destroys a database at the specified path
         * @param path The database path
         * @return true if destruction successful, false otherwise
         */
        static bool DestroyDatabase(const std::string& path);

    private:
        std::string dbPath_;
        std::unique_ptr<leveldb::DB> db_;
        mutable std::mutex dbMutex_;
        bool isOpen_;

        /**
         * @brief Converts a StorageKey to LevelDB key format
         * @param key The storage key
         * @return LevelDB key string
         */
        std::string StorageKeyToString(const StorageKey& key) const;

        /**
         * @brief Converts a LevelDB key string back to StorageKey
         * @param keyStr The LevelDB key string
         * @return Storage key
         */
        StorageKey StringToStorageKey(const std::string& keyStr) const;

        /**
         * @brief Converts StorageItem to LevelDB value format
         * @param item The storage item
         * @return LevelDB value string
         */
        std::string StorageItemToString(const StorageItem& item) const;

        /**
         * @brief Converts LevelDB value string back to StorageItem
         * @param valueStr The LevelDB value string
         * @return Storage item
         */
        std::shared_ptr<StorageItem> StringToStorageItem(const std::string& valueStr) const;

        /**
         * @brief Creates default LevelDB options
         * @return LevelDB options
         */
        leveldb::Options CreateDefaultOptions() const;
    };

    /**
     * @brief LevelDB snapshot implementation
     */
    class LevelDBSnapshot : public ISnapshot
    {
    public:
        /**
         * @brief Constructs a snapshot
         * @param db The LevelDB instance
         * @param snapshot The LevelDB snapshot
         */
        LevelDBSnapshot(leveldb::DB* db, const leveldb::Snapshot* snapshot);

        /**
         * @brief Destructor - releases the snapshot
         */
        ~LevelDBSnapshot() override;

        /**
         * @brief Gets a value by key from the snapshot
         * @param key The key to lookup
         * @return The storage item, or nullptr if not found
         */
        std::shared_ptr<StorageItem> Get(const StorageKey& key) override;

        /**
         * @brief Checks if a key exists in the snapshot
         * @param key The key to check
         * @return true if key exists, false otherwise
         */
        bool Contains(const StorageKey& key) override;

        /**
         * @brief Gets all storage items with keys starting with the prefix
         * @param prefix The key prefix
         * @return Vector of key-value pairs
         */
        std::vector<std::pair<StorageKey, std::shared_ptr<StorageItem>>> 
            Find(const io::ByteVector& prefix) override;

    private:
        leveldb::DB* db_;
        const leveldb::Snapshot* snapshot_;
        std::unique_ptr<leveldb::ReadOptions> readOptions_;
    };
} 