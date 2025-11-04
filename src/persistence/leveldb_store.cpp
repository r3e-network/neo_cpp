/**
 * @file leveldb_store.cpp
 * @brief LevelDB storage backend implementation
 */

#include <neo/persistence/leveldb_store.h>

#ifdef NEO_HAS_LEVELDB

#include <neo/core/error_recovery.h>
#include <neo/core/exceptions.h>
#include <neo/core/validation.h>

#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

namespace neo::persistence
{
namespace
{
[[nodiscard]] bool SliceHasPrefix(const leveldb::Slice& slice, const std::string& prefix)
{
    if (prefix.empty()) return true;
    if (slice.size() < static_cast<int>(prefix.size())) return false;
    return std::memcmp(slice.data(), prefix.data(), prefix.size()) == 0;
}

[[nodiscard]] io::ByteVector SliceToByteVector(const leveldb::Slice& slice)
{
    return io::ByteVector(reinterpret_cast<const uint8_t*>(slice.data()), static_cast<size_t>(slice.size()));
}
}  // namespace

LevelDbStore::LevelDbStore(const LevelDbConfig& config)
    : config_(config), logger_(core::LoggerFactory::GetLogger("LevelDbStore"))
{
}

LevelDbStore::LevelDbStore(std::string db_path)
    : LevelDbStore(LevelDbConfig{.db_path = std::move(db_path)})
{
}

LevelDbStore::~LevelDbStore() { Close(); }

bool LevelDbStore::Open()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (IsOpen())
    {
        return true;
    }

    try
    {
        std::filesystem::create_directories(config_.db_path);
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to create LevelDB directory {}: {}", config_.db_path, e.what());
        return false;
    }

    cache_.reset(leveldb::NewLRUCache(config_.cache_size));
    if (config_.use_bloom_filter)
    {
        filter_policy_.reset(leveldb::NewBloomFilterPolicy(config_.bloom_bits_per_key));
    }
    else
    {
        filter_policy_.reset();
    }

    leveldb::Options options = GetOptions();
    options.block_cache = cache_.get();
    options.filter_policy = filter_policy_.get();

    leveldb::DB* raw_db = nullptr;
    leveldb::Status status = leveldb::DB::Open(options, config_.db_path, &raw_db);
    if (!status.ok())
    {
        logger_->Error("Failed to open LevelDB at {}: {}", config_.db_path, status.ToString());
        db_.reset();
        cache_.reset();
        filter_policy_.reset();
        return false;
    }

    db_.reset(raw_db);
    logger_->Info("LevelDB opened at {}", config_.db_path);
    return true;
}

void LevelDbStore::Close()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!IsOpen())
    {
        return;
    }

    db_.reset();
    cache_.reset();
    filter_policy_.reset();

    logger_->Info("LevelDB closed");
}

void LevelDbStore::Put(const StorageKey& key, const StorageItem& value)
{
    auto key_bytes = key.ToArray();
    auto value_bytes = value.ToArray();
    Put(key_bytes, value_bytes);
}

std::optional<StorageItem> LevelDbStore::Get(const StorageKey& key) const
{
    auto key_bytes = key.ToArray();
    auto data = TryGet(key_bytes);
    if (!data)
    {
        return std::nullopt;
    }

    StorageItem item;
    item.DeserializeFromArray(std::span<const uint8_t>(data->Data(), data->Size()));
    return item;
}

bool LevelDbStore::Contains(const StorageKey& key) const { return Contains(key.ToArray()); }

void LevelDbStore::Delete(const StorageKey& key) { Delete(key.ToArray()); }

std::vector<std::pair<StorageKey, StorageItem>> LevelDbStore::Find(const io::ByteSpan& keyPrefix) const
{
    const io::ByteVector prefix_vec(keyPrefix);
    const io::ByteVector* prefix_ptr = prefix_vec.Size() > 0 ? &prefix_vec : nullptr;
    auto raw_results = Find(prefix_ptr, SeekDirection::Forward);

    std::vector<std::pair<StorageKey, StorageItem>> results;
    results.reserve(raw_results.size());

    for (auto& entry : raw_results)
    {
        StorageKey storage_key(entry.first);
        StorageItem item;
        item.DeserializeFromArray(std::span<const uint8_t>(entry.second.Data(), entry.second.Size()));
        results.emplace_back(std::move(storage_key), std::move(item));
    }

    return results;
}

void LevelDbStore::Clear()
{
    using namespace neo::core;

    auto operation = [this]() -> bool
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!IsOpen())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, "Database not open");
        }

        leveldb::ReadOptions read_options = GetReadOptions();
        read_options.fill_cache = false;

        leveldb::WriteBatch batch;
        std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(read_options));
        for (it->SeekToFirst(); it->Valid(); it->Next())
        {
            batch.Delete(it->key());
        }

        if (!it->status().ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR,
                                   "Iterator error during LevelDB clear: " + it->status().ToString());
        }

        leveldb::Status status = db_->Write(GetWriteOptions(true), &batch);
        if (!status.ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR,
                                   "LevelDB clear failed: " + status.ToString());
        }

        return true;
    };

    auto result = core::ErrorRecovery::Retry<bool>(operation, core::ErrorRecovery::DatabaseRetryConfig());
    if (!result.success)
    {
        throw StorageException(result.error_code, result.error_message);
    }
}

void LevelDbStore::Put(const io::ByteVector& key, const io::ByteVector& value)
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

        const std::string key_str(reinterpret_cast<const char*>(key.Data()), key.Size());
        const std::string value_str(reinterpret_cast<const char*>(value.Data()), value.Size());

        leveldb::Status status = db_->Put(GetWriteOptions(false), key_str, value_str);
        if (!status.ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR,
                                   "LevelDB Put failed: " + status.ToString());
        }

        write_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    };

    auto result = core::ErrorRecovery::Retry<bool>(operation, core::ErrorRecovery::DatabaseRetryConfig());
    if (!result.success)
    {
        throw StorageException(result.error_code, result.error_message);
    }
}

std::optional<io::ByteVector> LevelDbStore::TryGet(const io::ByteVector& key) const
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

        const std::string key_str(reinterpret_cast<const char*>(key.Data()), key.Size());
        std::string value;
        leveldb::Status status = db_->Get(GetReadOptions(), key_str, &value);

        if (status.IsNotFound())
        {
            return std::nullopt;
        }

        if (!status.ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR,
                                   "LevelDB Get failed: " + status.ToString());
        }

        read_count_.fetch_add(1, std::memory_order_relaxed);
        return io::ByteVector(reinterpret_cast<const uint8_t*>(value.data()), value.size());
    };

    auto result = core::ErrorRecovery::Retry<std::optional<io::ByteVector>>(
        operation, core::ErrorRecovery::DatabaseRetryConfig());
    if (!result.success)
    {
        throw StorageException(result.error_code, result.error_message);
    }

    return result.value.value_or(std::nullopt);
}

bool LevelDbStore::Contains(const io::ByteVector& key) const { return TryGet(key).has_value(); }

void LevelDbStore::Delete(const io::ByteVector& key)
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

        const std::string key_str(reinterpret_cast<const char*>(key.Data()), key.Size());
        leveldb::Status status = db_->Delete(GetWriteOptions(false), key_str);

        if (!status.ok() && !status.IsNotFound())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR,
                                   "LevelDB Delete failed: " + status.ToString());
        }

        delete_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    };

    auto result = core::ErrorRecovery::Retry<bool>(operation, core::ErrorRecovery::DatabaseRetryConfig());
    if (!result.success)
    {
        throw StorageException(result.error_code, result.error_message);
    }
}

std::vector<std::pair<io::ByteVector, io::ByteVector>> LevelDbStore::Find(const io::ByteVector* prefix,
                                                                          SeekDirection direction) const
{
    using namespace neo::core;

    std::vector<std::pair<io::ByteVector, io::ByteVector>> results;

    auto operation = [this, &results, prefix, direction]() -> bool
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!IsOpen())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR, "Database not open");
        }

        leveldb::ReadOptions read_options = GetReadOptions();
        read_options.fill_cache = direction == SeekDirection::Forward;

        std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(read_options));

        const std::string prefix_str =
            prefix ? std::string(reinterpret_cast<const char*>(prefix->Data()), prefix->Size()) : std::string();

        if (direction == SeekDirection::Forward)
        {
            if (prefix)
            {
                for (it->Seek(prefix_str); it->Valid() && SliceHasPrefix(it->key(), prefix_str); it->Next())
                {
                    results.emplace_back(SliceToByteVector(it->key()), SliceToByteVector(it->value()));
                }
            }
            else
            {
                for (it->SeekToFirst(); it->Valid(); it->Next())
                {
                    results.emplace_back(SliceToByteVector(it->key()), SliceToByteVector(it->value()));
                }
            }
        }
        else
        {
            if (prefix)
            {
                it->Seek(prefix_str);
                if (!it->Valid())
                {
                    it->SeekToLast();
                }
                else if (!SliceHasPrefix(it->key(), prefix_str))
                {
                    it->Prev();
                }

                while (it->Valid() && SliceHasPrefix(it->key(), prefix_str))
                {
                    results.emplace_back(SliceToByteVector(it->key()), SliceToByteVector(it->value()));
                    it->Prev();
                }
            }
            else
            {
                for (it->SeekToLast(); it->Valid(); it->Prev())
                {
                    results.emplace_back(SliceToByteVector(it->key()), SliceToByteVector(it->value()));
                }
            }
        }

        if (!it->status().ok())
        {
            throw StorageException(NeoException::ErrorCode::STORAGE_ERROR,
                                   "Iterator error during LevelDB Find: " + it->status().ToString());
        }

        return true;
    };

    auto result = core::ErrorRecovery::Retry<bool>(operation, core::ErrorRecovery::DatabaseRetryConfig());
    if (!result.success)
    {
        throw StorageException(result.error_code, result.error_message);
    }

    return results;
}

void LevelDbStore::WriteBatch::Put(const StorageKey& key, const StorageItem& value)
{
    const auto key_str = store_->ToDbKey(key);
    const auto value_str = store_->ToDbValue(value);
    batch_.Put(key_str, value_str);
}

void LevelDbStore::WriteBatch::Delete(const StorageKey& key)
{
    const auto key_str = store_->ToDbKey(key);
    batch_.Delete(key_str);
}

bool LevelDbStore::WriteBatch::Commit()
{
    std::lock_guard<std::mutex> lock(store_->mutex_);

    if (!store_->IsOpen())
    {
        throw core::StorageException(core::NeoException::ErrorCode::STORAGE_ERROR, "Database not open");
    }

    leveldb::Status status = store_->db_->Write(store_->GetWriteOptions(false), &batch_);
    if (!status.ok())
    {
        throw core::StorageException(core::NeoException::ErrorCode::STORAGE_ERROR,
                                     "LevelDB batch commit failed: " + status.ToString());
    }

    store_->write_count_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void LevelDbStore::WriteBatch::Clear() { batch_.Clear(); }

std::unique_ptr<LevelDbStore::WriteBatch> LevelDbStore::CreateWriteBatch()
{
    return std::make_unique<WriteBatch>(this);
}

std::string LevelDbStore::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!IsOpen())
    {
        return "LevelDB not open";
    }

    std::string stats;
    if (db_->GetProperty("leveldb.stats", &stats))
    {
        return stats;
    }

    return "LevelDB statistics unavailable";
}

void LevelDbStore::Compact()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!IsOpen()) return;

    db_->CompactRange(nullptr, nullptr);
}

bool LevelDbStore::Backup(const std::string& backup_path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!IsOpen()) return false;

    try
    {
        const std::filesystem::path path(backup_path);
        if (path.empty())
        {
            throw std::invalid_argument("backup_path cannot be empty");
        }

        if (std::filesystem::exists(path))
        {
            std::filesystem::remove_all(path);
        }
        std::filesystem::create_directories(path);

        leveldb::Options backup_options;
        backup_options.create_if_missing = true;
        backup_options.error_if_exists = false;

        leveldb::DB* backup_db_raw = nullptr;
        leveldb::Status status = leveldb::DB::Open(backup_options, path.string(), &backup_db_raw);
        if (!status.ok())
        {
            logger_->Error("Failed to open LevelDB backup at {}: {}", backup_path, status.ToString());
            return false;
        }

        std::unique_ptr<leveldb::DB> backup_db(backup_db_raw);

        const leveldb::Snapshot* snapshot = db_->GetSnapshot();
        struct SnapshotGuard
        {
            leveldb::DB* db{nullptr};
            const leveldb::Snapshot* snapshot{nullptr};
            ~SnapshotGuard()
            {
                if (db != nullptr && snapshot != nullptr)
                {
                    db->ReleaseSnapshot(snapshot);
                }
            }
        } snapshot_guard{db_.get(), snapshot};

        leveldb::ReadOptions read_options = GetReadOptions();
        read_options.snapshot = snapshot;
        read_options.fill_cache = false;

        std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(read_options));
        for (it->SeekToFirst(); it->Valid(); it->Next())
        {
            status = backup_db->Put(leveldb::WriteOptions(), it->key(), it->value());
            if (!status.ok())
            {
                logger_->Error("Failed to write LevelDB backup at {}: {}", backup_path, status.ToString());
                return false;
            }
        }

        if (!it->status().ok())
        {
            logger_->Error("Iterator error during LevelDB backup at {}: {}", backup_path, it->status().ToString());
            return false;
        }

        backup_db->CompactRange(nullptr, nullptr);
        logger_->Info("LevelDB backup completed at {}", backup_path);
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("LevelDB backup failed: {}", e.what());
        return false;
    }
}

std::string LevelDbStore::ToDbKey(const StorageKey& key) const
{
    auto bytes = key.ToArray();
    return std::string(reinterpret_cast<const char*>(bytes.Data()), bytes.Size());
}

StorageKey LevelDbStore::FromDbKey(const leveldb::Slice& key) const
{
    io::ByteVector bytes(reinterpret_cast<const uint8_t*>(key.data()), static_cast<size_t>(key.size()));
    return StorageKey(bytes);
}

std::string LevelDbStore::ToDbValue(const StorageItem& item) const
{
    auto bytes = item.ToArray();
    return std::string(reinterpret_cast<const char*>(bytes.Data()), bytes.Size());
}

StorageItem LevelDbStore::FromDbValue(const leveldb::Slice& value) const
{
    StorageItem item;
    item.DeserializeFromArray(
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(value.data()), static_cast<size_t>(value.size())));
    return item;
}

leveldb::Options LevelDbStore::GetOptions() const
{
    leveldb::Options options;
    options.create_if_missing = true;
    options.paranoid_checks = config_.paranoid_checks;
    options.write_buffer_size = config_.write_buffer_size;
    options.max_open_files = static_cast<int>(config_.max_open_files);
    options.block_size = config_.block_size;
    options.compression = leveldb::kNoCompression;
    return options;
}

leveldb::ReadOptions LevelDbStore::GetReadOptions() const
{
    leveldb::ReadOptions options;
    options.verify_checksums = config_.paranoid_checks;
    options.fill_cache = true;
    return options;
}

leveldb::WriteOptions LevelDbStore::GetWriteOptions(bool sync) const
{
    leveldb::WriteOptions options;
    options.sync = sync || config_.sync_writes;
    return options;
}

LevelDbStoreProvider::LevelDbStoreProvider(LevelDbConfig config) : config_(std::move(config)) {}

std::string LevelDbStoreProvider::GetName() const { return "LevelDB"; }

std::unique_ptr<IStore> LevelDbStoreProvider::GetStore(const std::string& path)
{
    LevelDbConfig store_config = config_;
    store_config.db_path = ResolvePath(path);

    auto store = std::make_unique<LevelDbStore>(store_config);
    if (!store->Open())
    {
        throw core::StorageException(core::NeoException::ErrorCode::STORAGE_ERROR,
                                     "Failed to open LevelDB store at: " + store_config.db_path);
    }

    return store;
}

std::string LevelDbStoreProvider::ResolvePath(const std::string& path) const
{
    if (path.empty()) return config_.db_path;

    const std::filesystem::path provided(path);
    if (provided.is_absolute())
    {
        return provided.string();
    }

    const std::filesystem::path base(config_.db_path);
    if (base.has_parent_path())
    {
        return (base.parent_path() / provided).string();
    }

    return (std::filesystem::current_path() / provided).string();
}

std::shared_ptr<IStoreProvider> CreateLevelDbStoreProvider(const LevelDbConfig& config)
{
    return std::make_shared<LevelDbStoreProvider>(config);
}

}  // namespace neo::persistence

#else  // NEO_HAS_LEVELDB

#include <neo/core/logging.h>

namespace neo::persistence
{

std::shared_ptr<IStoreProvider> CreateLevelDbStoreProvider(const LevelDbConfig&)
{
    auto logger = core::LoggerFactory::GetLogger("LevelDbStore");
    logger->Warning("LevelDB backend not available - provider request ignored");
    return nullptr;
}

}  // namespace neo::persistence

#endif  // NEO_HAS_LEVELDB
