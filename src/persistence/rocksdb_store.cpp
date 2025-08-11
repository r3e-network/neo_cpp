#include <neo/core/error_recovery.h>
#include <neo/core/exceptions.h>
#include <neo/core/validation.h>
#include <neo/persistence/rocksdb_store.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>

#include <stdexcept>

// Check if RocksDB is available - use feature test
#if defined(NEO_HAS_ROCKSDB) && __has_include(<rocksdb/db.h>)
#define NEO_ROCKSDB_AVAILABLE 1
#include <rocksdb/convenience.h>
#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/statistics.h>
#include <rocksdb/table.h>
#include <rocksdb/write_batch.h>
#if __has_include(<rocksdb/checkpoint.h>)
#include <rocksdb/checkpoint.h>
#define NEO_HAS_ROCKSDB_CHECKPOINT 1
#endif
#include <filesystem>
#else
#define NEO_ROCKSDB_AVAILABLE 0
#endif

namespace neo::persistence
{

#if NEO_ROCKSDB_AVAILABLE
// Production-ready RocksDB store implementation
RocksDbStore::RocksDbStore(const RocksDbConfig& config)
    : config_(config), logger_(core::LoggerFactory::GetLogger("RocksDbStore"))
{
    using namespace neo::core;

    // Validate configuration
    VALIDATE_NOT_EMPTY(config_.db_path);
    VALIDATE_RANGE(config_.write_buffer_size, static_cast<size_t>(1024 * 1024),
                   static_cast<size_t>(1024ULL * 1024 * 1024));
    VALIDATE_RANGE(config_.max_write_buffer_number, 1, 16);
    VALIDATE_RANGE(config_.block_cache_size, static_cast<size_t>(1024 * 1024),
                   static_cast<size_t>(16ULL * 1024 * 1024 * 1024));

    logger_->Info("Initializing RocksDB store at path: {}", config_.db_path);
}

RocksDbStore::~RocksDbStore() { Close(); }

bool RocksDbStore::Open()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (IsOpen())
    {
        logger_->Warning("RocksDB is already open");
        return true;
    }

    try
    {
        // Create directory if it doesn't exist
        std::filesystem::create_directories(config_.db_path);

        // Initialize options
        options_ = std::make_unique<rocksdb::Options>(GetOptions());
        table_options_ = std::make_unique<rocksdb::BlockBasedTableOptions>();

        // Configure table options for maximum performance
        table_options_->block_size = config_.block_size;
        table_options_->block_cache = rocksdb::NewLRUCache(config_.block_cache_size);
        table_options_->cache_index_and_filter_blocks = true;
        table_options_->pin_l0_filter_and_index_blocks_in_cache = true;

        // Enable memory-mapped files for ultra-fast I/O
        options_->allow_mmap_reads = true;
        options_->allow_mmap_writes = false;  // mmap writes can cause issues on some systems

        // Additional performance optimizations
        options_->use_direct_reads = false;  // Direct I/O can conflict with mmap
        options_->use_direct_io_for_flush_and_compaction = false;
        options_->max_open_files = -1;  // Keep all files open for fast access

        if (config_.use_bloom_filter)
        {
            table_options_->filter_policy.reset(rocksdb::NewBloomFilterPolicy(config_.bloom_bits_per_key, false));
        }

        options_->table_factory.reset(rocksdb::NewBlockBasedTableFactory(*table_options_));

        // Enable statistics
        options_->statistics = rocksdb::CreateDBStatistics();

        // Try to detect existing column families first
        rocksdb::DB* db_ptr;
        std::vector<std::string> existing_cf_names;
        rocksdb::Status list_status = rocksdb::DB::ListColumnFamilies(*options_, config_.db_path, &existing_cf_names);

        rocksdb::Status status;
        if (list_status.ok() && existing_cf_names.size() > 1)
        {
            // Database exists with column families - open with existing ones
            logger_->Info("Found existing RocksDB with {} column families", existing_cf_names.size());

            std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
            for (const auto& cf_name : existing_cf_names)
            {
                column_families.emplace_back(cf_name, *options_);
                logger_->Debug("Opening existing column family: {}", cf_name);
            }

            status = rocksdb::DB::Open(*options_, config_.db_path, column_families, &cf_handles_, &db_ptr);
        }
        else
        {
            // New database or no column families - create fresh
            logger_->Info("Creating new RocksDB database");
            status = rocksdb::DB::Open(*options_, config_.db_path, &db_ptr);

            if (status.ok())
            {
                db_.reset(db_ptr);
                cf_handles_.clear();
                cf_handles_.push_back(db_->DefaultColumnFamily());

                // Create additional column families
                std::vector<std::string> cf_names = {"blocks", "transactions", "contracts", "storage"};
                for (const auto& cf_name : cf_names)
                {
                    rocksdb::ColumnFamilyHandle* cf;
                    rocksdb::Status cf_status = db_->CreateColumnFamily(*options_, cf_name, &cf);
                    if (cf_status.ok())
                    {
                        cf_handles_.push_back(cf);
                        logger_->Info("Created column family: {}", cf_name);
                    }
                    else
                    {
                        logger_->Warning("Failed to create column family {}: {}", cf_name, cf_status.ToString());
                    }
                }
            }
        }

        if (!status.ok())
        {
            throw neo::core::StorageException(neo::core::NeoException::ErrorCode::STORAGE_ERROR,
                                              "Failed to open RocksDB: " + status.ToString());
        }

        db_.reset(db_ptr);

        // Set up column family handles - use what we have available
        if (!cf_handles_.empty())
        {
            default_cf_ = cf_handles_[0];  // Always the default CF

            // Assign other column families if available
            blocks_cf_ = cf_handles_.size() > 1 ? cf_handles_[1] : default_cf_;
            transactions_cf_ = cf_handles_.size() > 2 ? cf_handles_[2] : default_cf_;
            contracts_cf_ = cf_handles_.size() > 3 ? cf_handles_[3] : default_cf_;
            storage_cf_ = cf_handles_.size() > 4 ? cf_handles_[4] : default_cf_;
        }
        else
        {
            // Fallback - all use default
            default_cf_ = db_->DefaultColumnFamily();
            blocks_cf_ = default_cf_;
            transactions_cf_ = default_cf_;
            contracts_cf_ = default_cf_;
            storage_cf_ = default_cf_;
        }

        logger_->Info("RocksDB opened successfully with {} column families", cf_handles_.size());
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to open RocksDB: {}", e.what());
        return false;
    }
}

void RocksDbStore::Close()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!IsOpen()) return;

    logger_->Info("Closing RocksDB store");

    // Close column family handles
    for (auto* handle : cf_handles_)
    {
        if (handle)
        {
            db_->DestroyColumnFamilyHandle(handle);
        }
    }
    cf_handles_.clear();

    default_cf_ = nullptr;
    blocks_cf_ = nullptr;
    transactions_cf_ = nullptr;
    contracts_cf_ = nullptr;
    storage_cf_ = nullptr;

    // Close database
    db_.reset();
    options_.reset();
    table_options_.reset();

    logger_->Info("RocksDB closed");
}

void RocksDbStore::Put(const io::ByteVector& key, const io::ByteVector& value)
{
    using namespace neo::core;

    VALIDATE_NOT_EMPTY(key);

    auto operation = [this, &key, &value]() -> bool
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!IsOpen())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, "Database not open");
        }

        auto cf = GetColumnFamily(key);
        auto write_options = GetWriteOptions(false);

        rocksdb::Status status = db_->Put(write_options, cf, ToDbKey(key), ToDbValue(value));

        if (!status.ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, "RocksDB Put failed: " + status.ToString());
        }

        write_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    };

    auto result = core::ErrorRecovery::Retry<bool>(operation, core::ErrorRecovery::DatabaseRetryConfig());
    if (!result.success)
    {
        throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, result.error_message);
    }
}

void RocksDbStore::Delete(const io::ByteVector& key)
{
    using namespace neo::core;

    VALIDATE_NOT_EMPTY(key);

    auto operation = [this, &key]() -> bool
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!IsOpen())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, "Database not open");
        }

        auto cf = GetColumnFamily(key);
        auto write_options = GetWriteOptions(false);

        rocksdb::Status status = db_->Delete(write_options, cf, ToDbKey(key));

        if (!status.ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR,
                                   "RocksDB Delete failed: " + status.ToString());
        }

        delete_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    };

    auto result = core::ErrorRecovery::Retry<bool>(operation, core::ErrorRecovery::DatabaseRetryConfig());
    if (!result.success)
    {
        throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, result.error_message);
    }
}

std::optional<io::ByteVector> RocksDbStore::TryGet(const io::ByteVector& key) const
{
    using namespace neo::core;

    if (key.Size() == 0) return std::nullopt;

    auto operation = [this, &key]() -> std::optional<io::ByteVector>
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!IsOpen())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, "Database not open");
        }

        auto cf = GetColumnFamily(key);
        auto read_options = GetReadOptions();

        std::string value;
        rocksdb::Status status = db_->Get(read_options, cf, ToDbKey(key), &value);

        read_count_.fetch_add(1, std::memory_order_relaxed);

        if (status.IsNotFound())
        {
            return std::nullopt;
        }

        if (!status.ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, "RocksDB Get failed: " + status.ToString());
        }

        return FromDbValue(value);
    };

    auto result = core::ErrorRecovery::Retry<std::optional<io::ByteVector>>(operation,
                                                                            core::ErrorRecovery::DatabaseRetryConfig());

    if (!result.success)
    {
        logger_->Error("Get operation failed: {}", result.error_message);
        return std::nullopt;
    }

    return result.value.value_or(std::nullopt);
}

bool RocksDbStore::Contains(const io::ByteVector& key) const
{
    auto value = TryGet(key);
    return value.has_value();
}

std::vector<std::pair<io::ByteVector, io::ByteVector>> RocksDbStore::Find(const io::ByteVector* prefix,
                                                                          SeekDirection direction) const
{
    using namespace neo::core;

    auto operation = [this, prefix, direction]() -> std::vector<std::pair<io::ByteVector, io::ByteVector>>
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!IsOpen())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, "Database not open");
        }

        std::vector<std::pair<io::ByteVector, io::ByteVector>> result;
        auto read_options = GetReadOptions();

        // Use appropriate column family
        auto cf = prefix ? GetColumnFamily(*prefix) : default_cf_;
        std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_options, cf));

        if (prefix == nullptr)
        {
            if (direction == SeekDirection::Forward)
                it->SeekToFirst();
            else
                it->SeekToLast();
        }
        else
        {
            std::string db_prefix = ToDbKey(*prefix);
            if (direction == SeekDirection::Forward)
                it->Seek(db_prefix);
            else
                it->SeekForPrev(db_prefix);
        }

        while (it->Valid())
        {
            if (prefix != nullptr)
            {
                std::string key_str = it->key().ToString();
                std::string prefix_str = ToDbKey(*prefix);

                if (direction == SeekDirection::Forward)
                {
                    if (key_str.size() < prefix_str.size() || key_str.substr(0, prefix_str.size()) != prefix_str) break;
                }
                else
                {
                    if (key_str.find(prefix_str) != 0) break;
                }
            }

            auto key = FromDbKey(it->key());
            auto value = FromDbValue(it->value());
            result.emplace_back(std::move(key), std::move(value));

            if (direction == SeekDirection::Forward)
                it->Next();
            else
                it->Prev();

            // Limit results to prevent memory issues
            if (result.size() >= 10000) break;
        }

        if (!it->status().ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR,
                                   "RocksDB iterator error: " + it->status().ToString());
        }

        return result;
    };

    auto result = core::ErrorRecovery::Retry<std::vector<std::pair<io::ByteVector, io::ByteVector>>>(
        operation, core::ErrorRecovery::DatabaseRetryConfig());

    if (!result.success)
    {
        logger_->Error("Find operation failed: {}", result.error_message);
        return {};
    }

    return result.value.value_or(std::vector<std::pair<io::ByteVector, io::ByteVector>>{});
}

std::unique_ptr<RocksDbStore::WriteBatch> RocksDbStore::CreateWriteBatch()
{
    return std::make_unique<WriteBatch>(this);
}

std::string RocksDbStore::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!IsOpen() || !options_ || !options_->statistics) return "Database not open or statistics not enabled";

    return options_->statistics->ToString();
}

std::string RocksDbStore::GetProperty(const std::string& property) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!IsOpen()) return "Database not open";

    std::string value;
    if (db_->GetProperty(default_cf_, property, &value)) return value;

    return "Property not found";
}

void RocksDbStore::Compact()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!IsOpen()) return;

    logger_->Info("Starting RocksDB compaction");

    for (auto* cf : cf_handles_)
    {
        if (cf)
        {
            rocksdb::Status status = db_->CompactRange(rocksdb::CompactRangeOptions(), cf, nullptr, nullptr);
            if (!status.ok())
            {
                logger_->Warning("Compaction failed for column family: {}", status.ToString());
            }
        }
    }

    logger_->Info("RocksDB compaction completed");
}

bool RocksDbStore::CreateCheckpoint(const std::string& checkpoint_path)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!IsOpen()) return false;

    try
    {
        std::filesystem::create_directories(checkpoint_path);

#ifdef NEO_HAS_ROCKSDB_CHECKPOINT
        rocksdb::Checkpoint* checkpoint;
        rocksdb::Status status = rocksdb::Checkpoint::Create(db_.get(), &checkpoint);

        if (!status.ok())
        {
            logger_->Error("Failed to create checkpoint object: {}", status.ToString());
            return false;
        }

        std::unique_ptr<rocksdb::Checkpoint> checkpoint_ptr(checkpoint);
        status = checkpoint_ptr->CreateCheckpoint(checkpoint_path);

        if (!status.ok())
        {
            logger_->Error("Failed to create checkpoint: {}", status.ToString());
            return false;
        }
#else
        logger_->Error("Checkpoint functionality not available - RocksDB version too old");
        return false;
#endif

        logger_->Info("Checkpoint created at: {}", checkpoint_path);
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Exception during checkpoint creation: {}", e.what());
        return false;
    }
}

void RocksDbStore::Flush()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!IsOpen()) return;

    rocksdb::FlushOptions flush_options;
    flush_options.wait = true;

    for (auto* cf : cf_handles_)
    {
        if (cf)
        {
            db_->Flush(flush_options, cf);
        }
    }

    logger_->Debug("RocksDB flush completed");
}

rocksdb::ColumnFamilyHandle* RocksDbStore::GetColumnFamily(const io::ByteVector& key) const
{
    if (key.Size() == 0 || cf_handles_.empty()) return default_cf_;

    // Route keys to appropriate column families based on prefix
    uint8_t prefix = key[0];

    switch (prefix)
    {
        case 0x5F:  // Block prefix
            return blocks_cf_ ? blocks_cf_ : default_cf_;
        case 0x4F:  // Transaction prefix
            return transactions_cf_ ? transactions_cf_ : default_cf_;
        case 0x50:  // Contract prefix
            return contracts_cf_ ? contracts_cf_ : default_cf_;
        case 0x70:  // Storage prefix
            return storage_cf_ ? storage_cf_ : default_cf_;
        default:
            return default_cf_;
    }
}

std::string RocksDbStore::ToDbKey(const io::ByteVector& key) const
{
    return std::string(reinterpret_cast<const char*>(key.Data()), key.Size());
}

io::ByteVector RocksDbStore::FromDbKey(const rocksdb::Slice& key) const
{
    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(key.data()), key.size()));
}

std::string RocksDbStore::ToDbValue(const io::ByteVector& value) const
{
    return std::string(reinterpret_cast<const char*>(value.Data()), value.Size());
}

io::ByteVector RocksDbStore::FromDbValue(const rocksdb::Slice& value) const
{
    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
}

rocksdb::Options RocksDbStore::GetOptions() const
{
    rocksdb::Options options;

    // Basic options
    options.create_if_missing = true;
    options.create_missing_column_families = true;

    // Performance tuning
    options.write_buffer_size = config_.write_buffer_size;
    options.max_write_buffer_number = config_.max_write_buffer_number;
    options.target_file_size_base = config_.target_file_size_base;
    options.max_background_compactions = config_.max_background_compactions;
    options.max_background_flushes = config_.max_background_flushes;
    options.num_levels = config_.num_levels;
    options.max_open_files = static_cast<int>(config_.max_open_files);

    // Compression
    if (config_.compression_enabled)
    {
        options.compression = rocksdb::kLZ4Compression;
        options.compression_opts.level = config_.compression_level;
    }
    else
    {
        options.compression = rocksdb::kNoCompression;
    }

    // Point lookup optimization
    if (config_.optimize_for_point_lookup)
    {
        options.OptimizeForPointLookup(config_.optimize_for_point_lookup_cache_size);
    }

    // WAL settings
    options.WAL_ttl_seconds = 3600;    // 1 hour
    options.WAL_size_limit_MB = 1024;  // 1GB

    return options;
}

rocksdb::ReadOptions RocksDbStore::GetReadOptions() const
{
    rocksdb::ReadOptions options;
    options.verify_checksums = config_.verify_checksums;
    options.fill_cache = config_.fill_cache;
    return options;
}

rocksdb::WriteOptions RocksDbStore::GetWriteOptions(bool sync) const
{
    rocksdb::WriteOptions options;
    options.sync = sync || config_.sync_writes;
    options.disableWAL = config_.disable_wal;
    return options;
}

// WriteBatch implementation
void RocksDbStore::WriteBatch::Put(const io::ByteVector& key, const io::ByteVector& value)
{
    auto cf = store_->GetColumnFamily(key);
    batch_.Put(cf, store_->ToDbKey(key), store_->ToDbValue(value));
}

void RocksDbStore::WriteBatch::Delete(const io::ByteVector& key)
{
    auto cf = store_->GetColumnFamily(key);
    batch_.Delete(cf, store_->ToDbKey(key));
}

bool RocksDbStore::WriteBatch::Commit()
{
    std::lock_guard<std::mutex> lock(store_->mutex_);

    if (!store_->IsOpen()) return false;

    auto write_options = store_->GetWriteOptions(false);
    rocksdb::Status status = store_->db_->Write(write_options, &batch_);

    if (status.ok())
    {
        store_->write_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    return false;
}

void RocksDbStore::WriteBatch::Clear() { batch_.Clear(); }

size_t RocksDbStore::WriteBatch::GetDataSize() const { return batch_.GetDataSize(); }

// Store provider implementation
class RocksDbStoreProvider : public IStoreProvider
{
   private:
    RocksDbConfig config_;

   public:
    explicit RocksDbStoreProvider(const RocksDbConfig& config = RocksDbConfig{}) : config_(config) {}

    std::string GetName() const override { return "RocksDB"; }

    std::unique_ptr<IStore> GetStore(const std::string& path) override
    {
        RocksDbConfig store_config = config_;
        if (!path.empty())
        {
            store_config.db_path = path;
        }

        auto store = std::unique_ptr<RocksDbStore>(new RocksDbStore(store_config));
        if (!store->Open())
        {
            throw neo::core::StorageException(neo::core::NeoException::ErrorCode::STORAGE_ERROR,
                                              "Failed to open RocksDB store at: " + store_config.db_path);
        }

        return store;
    }

    void DeleteStore(const std::string& path)
    {
        try
        {
            std::filesystem::remove_all(path);
        }
        catch (const std::exception& e)
        {
            throw neo::core::StorageException(neo::core::NeoException::ErrorCode::STORAGE_ERROR,
                                              "Failed to delete RocksDB store: " + std::string(e.what()));
        }
    }
};

// Factory function for RocksDB store provider
std::shared_ptr<IStoreProvider> CreateRocksDbStoreProvider(const RocksDbConfig& config)
{
    return std::shared_ptr<IStoreProvider>(new RocksDbStoreProvider(config));
}

}  // namespace neo::persistence

#else
// RocksDB not available - provide fallback implementation
namespace neo::persistence
{
RocksDbStore::RocksDbStore(const RocksDbConfig& config)
    : config_(config), logger_(core::LoggerFactory::GetLogger("RocksDbStore"))
{
    logger_->Error("RocksDB support not compiled in");
    throw neo::core::StorageException(neo::core::NeoException::ErrorCode::STORAGE_ERROR,
                                      "RocksDB support not compiled in");
}

RocksDbStore::~RocksDbStore() = default;

bool RocksDbStore::Open() { return false; }
void RocksDbStore::Close() {}
void RocksDbStore::Put(const io::ByteVector& key, const io::ByteVector& value)
{
    throw neo::core::StorageException(neo::core::NeoException::ErrorCode::STORAGE_ERROR,
                                      "RocksDB support not compiled in");
}
void RocksDbStore::Delete(const io::ByteVector& key)
{
    throw neo::core::StorageException(neo::core::NeoException::ErrorCode::STORAGE_ERROR,
                                      "RocksDB support not compiled in");
}
std::optional<io::ByteVector> RocksDbStore::TryGet(const io::ByteVector& key) const { return std::nullopt; }
bool RocksDbStore::Contains(const io::ByteVector& key) const { return false; }
std::vector<std::pair<io::ByteVector, io::ByteVector>> RocksDbStore::Find(const io::ByteVector* prefix,
                                                                          SeekDirection direction) const
{
    return {};
}

// Fallback provider
class RocksDbStoreProvider : public IStoreProvider
{
   public:
    explicit RocksDbStoreProvider(const RocksDbConfig& config = RocksDbConfig{}) {}

    std::string GetName() const override { return "RocksDB (disabled)"; }

    std::unique_ptr<IStore> GetStore(const std::string& path) override
    {
        throw neo::core::StorageException(neo::core::NeoException::ErrorCode::STORAGE_ERROR,
                                          "RocksDB support not compiled in");
    }

    void DeleteStore(const std::string& path)
    {
        throw neo::core::StorageException(neo::core::NeoException::ErrorCode::STORAGE_ERROR,
                                          "RocksDB support not compiled in");
    }
};

std::shared_ptr<IStoreProvider> CreateRocksDbStoreProvider(const RocksDbConfig& config)
{
    return std::shared_ptr<IStoreProvider>(new RocksDbStoreProvider(config));
}

}  // namespace neo::persistence
#endif