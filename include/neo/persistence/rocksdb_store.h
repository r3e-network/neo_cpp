#pragma once

#include <neo/core/logging.h>
#include <neo/persistence/istore.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>

#include <memory>
#include <mutex>
#include <string>

#ifdef NEO_HAS_ROCKSDB
#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <rocksdb/write_batch.h>
#endif

namespace neo::persistence
{
/**
 * @brief RocksDB configuration options
 */
struct RocksDbConfig
{
    std::string db_path{"./data/rocksdb"};

    // Performance options
    size_t write_buffer_size{128 * 1024 * 1024};  // 128MB
    int max_write_buffer_number{4};
    size_t target_file_size_base{128 * 1024 * 1024};  // 128MB
    int max_background_compactions{4};
    int max_background_flushes{2};

    // Cache options
    size_t block_cache_size{1024 * 1024 * 1024};  // 1GB
    size_t block_size{16 * 1024};                 // 16KB

    // Compression
    bool compression_enabled{true};
    int compression_level{-1};  // Default compression

    // Bloom filter
    bool use_bloom_filter{true};
    int bloom_bits_per_key{10};

    // Write options
    bool sync_writes{false};
    bool disable_wal{false};

    // Read options
    bool verify_checksums{true};
    bool fill_cache{true};

    // Advanced options
    int num_levels{7};
    uint64_t max_open_files{5000};
    bool optimize_for_point_lookup{false};
    size_t optimize_for_point_lookup_cache_size{0};
};

#ifdef NEO_HAS_ROCKSDB
/**
 * @brief RocksDB-based persistent storage implementation
 *
 * RocksDB provides better performance than LevelDB for many workloads,
 * especially for SSD-based storage and concurrent access patterns.
 */
class RocksDbStore : public IStore
{
   private:
    RocksDbConfig config_;
    std::unique_ptr<rocksdb::DB> db_;
    std::unique_ptr<rocksdb::Options> options_;
    std::unique_ptr<rocksdb::BlockBasedTableOptions> table_options_;
    std::shared_ptr<core::Logger> logger_;
    mutable std::mutex mutex_;

    // Column families for different data types
    std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;

    // Batch operations
    std::unique_ptr<rocksdb::WriteBatch> current_batch_;
    std::mutex batch_mutex_;
    size_t batch_size_{0};
    static constexpr size_t MAX_BATCH_SIZE = 1000;
    rocksdb::ColumnFamilyHandle* default_cf_{nullptr};
    rocksdb::ColumnFamilyHandle* blocks_cf_{nullptr};
    rocksdb::ColumnFamilyHandle* transactions_cf_{nullptr};
    rocksdb::ColumnFamilyHandle* contracts_cf_{nullptr};
    rocksdb::ColumnFamilyHandle* storage_cf_{nullptr};

    // Statistics
    mutable std::atomic<uint64_t> read_count_{0};
    mutable std::atomic<uint64_t> write_count_{0};
    mutable std::atomic<uint64_t> delete_count_{0};

   public:
    /**
     * @brief Construct a new RocksDB store
     * @param config Configuration options
     */
    explicit RocksDbStore(const RocksDbConfig& config);

    ~RocksDbStore() override;

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

    // IStore interface implementation
    void Put(const io::ByteVector& key, const io::ByteVector& value) override;
    void Delete(const io::ByteVector& key) override;
    std::optional<io::ByteVector> TryGet(const io::ByteVector& key) const override;
    bool Contains(const io::ByteVector& key) const override;
    std::vector<std::pair<io::ByteVector, io::ByteVector>> Find(
        const io::ByteVector* prefix = nullptr, SeekDirection direction = SeekDirection::Forward) const override;

    /**
     * @brief Batch write operations for efficiency
     */
    class WriteBatch
    {
       private:
        rocksdb::WriteBatch batch_;
        RocksDbStore* store_;

       public:
        explicit WriteBatch(RocksDbStore* store) : store_(store) {}

        void Put(const io::ByteVector& key, const io::ByteVector& value);
        void Delete(const io::ByteVector& key);
        bool Commit();
        void Clear();
        size_t GetDataSize() const;
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
     * @brief Get detailed database properties
     */
    std::string GetProperty(const std::string& property) const;

    /**
     * @brief Compact the database
     */
    void Compact();

    /**
     * @brief Create a checkpoint (online backup)
     * @param checkpoint_path Path to checkpoint directory
     * @return true if successful
     */
    bool CreateCheckpoint(const std::string& checkpoint_path);

    /**
     * @brief Flush all memtables to disk
     */
    void Flush();

    /**
     * @brief Get column family for storage key
     */
    rocksdb::ColumnFamilyHandle* GetColumnFamily(const io::ByteVector& key) const;

   private:
    /**
     * @brief Initialize column families
     */
    bool InitializeColumnFamilies();

    /**
     * @brief Convert ByteVector to RocksDB key
     */
    std::string ToDbKey(const io::ByteVector& key) const;

    /**
     * @brief Convert RocksDB key to ByteVector
     */
    io::ByteVector FromDbKey(const rocksdb::Slice& key) const;

    /**
     * @brief Convert ByteVector to RocksDB value
     */
    std::string ToDbValue(const io::ByteVector& value) const;

    /**
     * @brief Convert RocksDB value to ByteVector
     */
    io::ByteVector FromDbValue(const rocksdb::Slice& value) const;

    /**
     * @brief Get RocksDB options
     */
    rocksdb::Options GetOptions() const;

    /**
     * @brief Get read options
     */
    rocksdb::ReadOptions GetReadOptions() const;

    /**
     * @brief Get write options
     */
    rocksdb::WriteOptions GetWriteOptions(bool sync = false) const;
};

// Alias for compatibility
using RocksDBStore = RocksDbStore;
using LevelDBStore = RocksDbStore;  // Temporary alias until LevelDB is implemented

#endif  // NEO_HAS_ROCKSDB

}  // namespace neo::persistence