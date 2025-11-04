/**
 * @file file_store.h
 * @brief Lightweight file-backed key/value store used when RocksDB/LevelDB are unavailable.
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/persistence/istore.h>

#include <mutex>
#include <string>
#include <unordered_map>

namespace neo::persistence
{
/**
 * @brief Simple file-backed implementation of IStore.
 *
 * The store keeps the data resident in memory and writes the entire map to disk
 * on each mutating operation. It is not as performant as LevelDB/RocksDB but
 * provides basic persistence for development and testing environments.
 */
class FileStore : public IStore
{
   public:
    explicit FileStore(std::string path);
    ~FileStore() override = default;

    void Put(const io::ByteVector& key, const io::ByteVector& value) override;
    std::optional<io::ByteVector> TryGet(const io::ByteVector& key) const override;
    bool Contains(const io::ByteVector& key) const override;
    void Delete(const io::ByteVector& key) override;
    std::vector<std::pair<io::ByteVector, io::ByteVector>> Find(
        const io::ByteVector* prefix = nullptr, SeekDirection direction = SeekDirection::Forward) const override;

   private:
    void Load();
    void Flush();

    std::string path_;
    mutable std::mutex mutex_;
    std::unordered_map<io::ByteVector, io::ByteVector> data_;
    bool dirty_{false};
};

/**
 * @brief Provider for FileStore instances.
 */
class FileStoreProvider : public IStoreProvider
{
   public:
    explicit FileStoreProvider(std::string basePath = "./data/file-store");

    std::string GetName() const override;
    std::unique_ptr<IStore> GetStore(const std::string& path) override;

   private:
    std::string ResolvePath(const std::string& path) const;

    std::string basePath_;
};
}  // namespace neo::persistence
