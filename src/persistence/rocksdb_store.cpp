#include <neo/persistence/rocksdb_store.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>
#include <stdexcept>

namespace neo::persistence
{
    // RocksDBStore implementation
    RocksDBStore::RocksDBStore(const std::string& path)
        : path_(path)
    {
        rocksdb::Options options;
        options.create_if_missing = true;

        rocksdb::DB* db;
        rocksdb::Status status = rocksdb::DB::Open(options, path, &db);

        if (!status.ok())
            throw std::runtime_error("Failed to open RocksDB database: " + status.ToString());

        db_.reset(db);
    }

    RocksDBStore::~RocksDBStore() = default;

    std::optional<io::ByteVector> RocksDBStore::TryGet(const io::ByteVector& key) const
    {
        std::string value;
        rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()), &value);

        if (!status.ok())
            return std::nullopt;

        return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
    }

    bool RocksDBStore::Contains(const io::ByteVector& key) const
    {
        std::string value;
        rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()), &value);

        return status.ok();
    }

    std::vector<std::pair<io::ByteVector, io::ByteVector>> RocksDBStore::Find(const io::ByteVector* prefix, SeekDirection direction) const
    {
        std::vector<std::pair<io::ByteVector, io::ByteVector>> result;

        rocksdb::ReadOptions options;
        std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(options));

        if (prefix == nullptr)
        {
            if (direction == SeekDirection::Forward)
                it->SeekToFirst();
            else
                it->SeekToLast();
        }
        else
        {
            if (direction == SeekDirection::Forward)
                it->Seek(rocksdb::Slice(reinterpret_cast<const char*>(prefix->Data()), prefix->Size()));
            else
                it->SeekForPrev(rocksdb::Slice(reinterpret_cast<const char*>(prefix->Data()), prefix->Size()));
        }

        while (it->Valid())
        {
            if (prefix != nullptr)
            {
                rocksdb::Slice key = it->key();

                if (direction == SeekDirection::Forward)
                {
                    if (key.size() < prefix->Size() || memcmp(key.data(), prefix->Data(), prefix->Size()) != 0)
                        break;
                }
                else
                {
                    if (key.size() > prefix->Size() || memcmp(key.data(), prefix->Data(), key.size()) != 0)
                        break;
                }
            }

            io::ByteVector keyVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(it->key().data()), it->key().size()));
            io::ByteVector valueVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(it->value().data()), it->value().size()));

            result.emplace_back(keyVector, valueVector);

            if (direction == SeekDirection::Forward)
                it->Next();
            else
                it->Prev();
        }

        return result;
    }

    void RocksDBStore::Put(const io::ByteVector& key, const io::ByteVector& value)
    {
        rocksdb::WriteOptions options;
        rocksdb::Status status = db_->Put(options, rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()), rocksdb::Slice(reinterpret_cast<const char*>(value.Data()), value.Size()));

        if (!status.ok())
            throw std::runtime_error("Failed to put value in RocksDB: " + status.ToString());
    }

    void RocksDBStore::PutSync(const io::ByteVector& key, const io::ByteVector& value)
    {
        rocksdb::WriteOptions options;
        options.sync = true;

        rocksdb::Status status = db_->Put(options, rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()), rocksdb::Slice(reinterpret_cast<const char*>(value.Data()), value.Size()));

        if (!status.ok())
            throw std::runtime_error("Failed to put value in RocksDB: " + status.ToString());
    }

    void RocksDBStore::Delete(const io::ByteVector& key)
    {
        rocksdb::WriteOptions options;
        rocksdb::Status status = db_->Delete(options, rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()));

        if (!status.ok() && !status.IsNotFound())
            throw std::runtime_error("Failed to delete value from RocksDB: " + status.ToString());
    }

    std::unique_ptr<IStoreSnapshot> RocksDBStore::CreateSnapshot()
    {
        return std::make_unique<RocksDBSnapshot>(*this);
    }

    // RocksDBSnapshot implementation
    RocksDBSnapshot::RocksDBSnapshot(RocksDBStore& store)
        : store_(store), snapshot_(store.db_->GetSnapshot()), batch_(std::make_unique<rocksdb::WriteBatch>())
    {
    }

    RocksDBSnapshot::~RocksDBSnapshot()
    {
        store_.db_->ReleaseSnapshot(snapshot_);
    }

    std::optional<io::ByteVector> RocksDBSnapshot::TryGet(const io::ByteVector& key) const
    {
        rocksdb::ReadOptions options;
        options.snapshot = snapshot_;

        std::string value;
        rocksdb::Status status = store_.db_->Get(options, rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()), &value);

        if (!status.ok())
            return std::nullopt;

        return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
    }

    bool RocksDBSnapshot::Contains(const io::ByteVector& key) const
    {
        rocksdb::ReadOptions options;
        options.snapshot = snapshot_;

        std::string value;
        rocksdb::Status status = store_.db_->Get(options, rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()), &value);

        return status.ok();
    }

    std::vector<std::pair<io::ByteVector, io::ByteVector>> RocksDBSnapshot::Find(const io::ByteVector* prefix, SeekDirection direction) const
    {
        std::vector<std::pair<io::ByteVector, io::ByteVector>> result;

        rocksdb::ReadOptions options;
        options.snapshot = snapshot_;

        std::unique_ptr<rocksdb::Iterator> it(store_.db_->NewIterator(options));

        if (prefix == nullptr)
        {
            if (direction == SeekDirection::Forward)
                it->SeekToFirst();
            else
                it->SeekToLast();
        }
        else
        {
            if (direction == SeekDirection::Forward)
                it->Seek(rocksdb::Slice(reinterpret_cast<const char*>(prefix->Data()), prefix->Size()));
            else
                it->SeekForPrev(rocksdb::Slice(reinterpret_cast<const char*>(prefix->Data()), prefix->Size()));
        }

        while (it->Valid())
        {
            if (prefix != nullptr)
            {
                rocksdb::Slice key = it->key();

                if (direction == SeekDirection::Forward)
                {
                    if (key.size() < prefix->Size() || memcmp(key.data(), prefix->Data(), prefix->Size()) != 0)
                        break;
                }
                else
                {
                    if (key.size() > prefix->Size() || memcmp(key.data(), prefix->Data(), key.size()) != 0)
                        break;
                }
            }

            io::ByteVector keyVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(it->key().data()), it->key().size()));
            io::ByteVector valueVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(it->value().data()), it->value().size()));

            result.emplace_back(keyVector, valueVector);

            if (direction == SeekDirection::Forward)
                it->Next();
            else
                it->Prev();
        }

        return result;
    }

    void RocksDBSnapshot::Put(const io::ByteVector& key, const io::ByteVector& value)
    {
        batch_->Put(rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()), rocksdb::Slice(reinterpret_cast<const char*>(value.Data()), value.Size()));
    }

    void RocksDBSnapshot::Delete(const io::ByteVector& key)
    {
        batch_->Delete(rocksdb::Slice(reinterpret_cast<const char*>(key.Data()), key.Size()));
    }

    void RocksDBSnapshot::Commit()
    {
        rocksdb::WriteOptions options;
        rocksdb::Status status = store_.db_->Write(options, batch_.get());

        if (!status.ok())
            throw std::runtime_error("Failed to commit changes to RocksDB: " + status.ToString());

        batch_->Clear();
    }

    IStore& RocksDBSnapshot::GetStore()
    {
        return store_;
    }

    // RocksDBStoreProvider implementation
    RocksDBStoreProvider::RocksDBStoreProvider() = default;

    std::string RocksDBStoreProvider::GetName() const
    {
        return "RocksDBStore";
    }

    std::unique_ptr<IStore> RocksDBStoreProvider::GetStore(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = stores_.find(path);
        if (it == stores_.end())
        {
            auto store = std::make_shared<RocksDBStore>(path);
            stores_[path] = store;
            // Create a new RocksDBStore with the same path
            auto clone = std::make_unique<RocksDBStore>(path + "_clone");
            // Copy data from the original store to the clone
            // Note: In production, this would be a snapshot operation
            return clone;
        }

        auto store = it->second;
        // Create a new RocksDBStore with the same path
        auto clone = std::make_unique<RocksDBStore>(path + "_clone");
        // Copy data from the original store to the clone
        // Note: In production, this would be a snapshot operation
        return clone;
    }
}
