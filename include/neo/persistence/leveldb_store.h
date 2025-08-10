#pragma once

#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/write_batch.h>
#include <neo/core/logging.h>
#include <neo/persistence/istore.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>

#include <memory>
#include <mutex>
#include <string>

namespace neo::persistence
{
/**
 * @brief LevelDB configuration options
 */
struct LevelDbConfig
{
    std::string db_path{"./data/leveldb"};
    size_t cache_size{512 * 1024 * 1024};        // 512MB cache
    size_t write_buffer_size{64 * 1024 * 1024};  // 64MB write buffer
    size_t max_open_files{1000};
    size_t block_size{4 * 1024};  // 4KB blocks
    int compression_level{1};     // 0=none, 1=snappy
    bool use_bloom_filter{true};
    int bloom_bits_per_key{10};
    bool paranoid_checks{true};
    bool sync_writes{false};  // Sync on critical writes only
};

/**
 * @brief LevelDB-based persistent storage implementation
 */
class LevelDbStore : public IStore
{
   private:
    LevelDbConfig config_;
    std::unique_ptr<leveldb::DB> db_;
    std::unique_ptr<leveldb::Cache> cache_;
    std::unique_ptr<const leveldb::FilterPolicy> filter_policy_;
    std::shared_ptr<core::Logger> logger_;
    mutable std::mutex mutex_;

    // Statistics
    mutable std::atomic<uint64_t> read_count_{0};
    mutable std::atomic<uint64_t> write_count_{0};
    mutable std::atomic<uint64_t> delete_count_{0};

   public:
    /**
     * @brief Construct a new LevelDB store
     * @param config Configuration options
     */
    explicit LevelDbStore(const LevelDbConfig& config);

    ~LevelDbStore() override;

    /**
     * @brief Open the database
     * @return true if successful
     */
    bool Open();

    /**
     * @brief Close the database
     */
    void Close();

    /**
     * @brief Check if database is open
     */
    bool IsOpen() const { return db_ != nullptr; }

    // StorageKey/StorageItem specific methods
    void Put(const StorageKey& key, const StorageItem& value);
    std::optional<StorageItem> Get(const StorageKey& key) const;
    bool Contains(const StorageKey& key) const;
    void Delete(const StorageKey& key);
    std::vector<std::pair<StorageKey, StorageItem>> Find(const io::ByteSpan& keyPrefix) const;
    void Clear();

    // IStore interface implementation
    void Put(const io::ByteVector& key, const io::ByteVector& value) override;
    std::optional<io::ByteVector> TryGet(const io::ByteVector& key) const override;
    bool Contains(const io::ByteVector& key) const override;
    void Delete(const io::ByteVector& key) override;
    std::vector<std::pair<io::ByteVector, io::ByteVector>> Find(
        const io::ByteVector* prefix = nullptr, SeekDirection direction = SeekDirection::Forward) const override;

    /**
     * @brief Batch write operations for efficiency
     */
    class WriteBatch
    {
       private:
        leveldb::WriteBatch batch_;
        LevelDbStore* store_;

       public:
        explicit WriteBatch(LevelDbStore* store) : store_(store) {}

        void Put(const StorageKey& key, const StorageItem& value);
        void Delete(const StorageKey& key);
        bool Commit();
        void Clear();
    };

    /**
     * @brief Create a new write batch
     */
    std::unique_ptr<WriteBatch> CreateWriteBatch();

    /**
     * @brief Get database statistics
     */
    std::string GetStatistics() const;

    /**
     * @brief Compact the database
     */
    void Compact();

    /**
     * @brief Backup the database
     * @param backup_path Path to backup directory
     * @return true if successful
     */
    bool Backup(const std::string& backup_path);

   private:
    /**
     * @brief Convert StorageKey to LevelDB key
     */
    std::string ToDbKey(const StorageKey& key) const;

    /**
     * @brief Convert LevelDB key to StorageKey
     */
    StorageKey FromDbKey(const leveldb::Slice& key) const;

    /**
     * @brief Convert StorageItem to LevelDB value
     */
    std::string ToDbValue(const StorageItem& item) const;

    /**
     * @brief Convert LevelDB value to StorageItem
     */
    StorageItem FromDbValue(const leveldb::Slice& value) const;

    /**
     * @brief Get LevelDB options
     */
    leveldb::Options GetOptions() const;

    /**
     * @brief Get read options
     */
    leveldb::ReadOptions GetReadOptions() const;

    /**
     * @brief Get write options
     */
    leveldb::WriteOptions GetWriteOptions(bool sync = false) const;
};
}  // namespace neo::persistence