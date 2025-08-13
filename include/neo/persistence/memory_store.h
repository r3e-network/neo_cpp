/**
 * @file memory_store.h
 * @brief Memory Store
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/persistence/istore.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace neo::persistence
{
/**
 * @brief A memory-based implementation of IStore.
 */
class MemoryStore : public IStore
{
   public:
    /**
     * @brief Constructs a MemoryStore.
     */
    MemoryStore();

    /**
     * @brief Copy constructor.
     * @param other The MemoryStore to copy from.
     */
    MemoryStore(const MemoryStore& other);

    /**
     * @brief Tries to get a value from the store.
     * @param key The key to look up.
     * @return The value if found, std::nullopt otherwise.
     */
    std::optional<io::ByteVector> TryGet(const io::ByteVector& key) const override;

    /**
     * @brief Gets a value from the store.
     * @param key The key to look up.
     * @return The value.
     * @throws std::runtime_error if the key is not found.
     */
    io::ByteVector Get(const io::ByteVector& key) const;

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
    std::vector<std::pair<io::ByteVector, io::ByteVector>> Find(
        const io::ByteVector* prefix = nullptr, SeekDirection direction = SeekDirection::Forward) const override;

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
     * @brief Gets a snapshot of the store.
     * @return The snapshot.
     */
    std::unique_ptr<IStoreSnapshot> GetSnapshot();

    /**
     * @brief Seeks all key-value pairs with keys that start with the specified prefix.
     * @param prefix The prefix to search for.
     * @param direction The direction to seek.
     * @return The key-value pairs found.
     */
    std::vector<std::pair<io::ByteVector, io::ByteVector>> Seek(const io::ByteVector& prefix,
                                                                SeekDirection direction = SeekDirection::Forward) const;

   private:
    std::unordered_map<io::ByteVector, io::ByteVector> store_;
    mutable std::mutex mutex_;

    friend class MemorySnapshot;
};

/**
 * @brief A snapshot of a MemoryStore.
 */
class MemorySnapshot : public IStoreSnapshot
{
   public:
    /**
     * @brief Constructs a MemorySnapshot.
     * @param store The store to snapshot.
     */
    explicit MemorySnapshot(MemoryStore& store);

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
    std::vector<std::pair<io::ByteVector, io::ByteVector>> Find(
        const io::ByteVector* prefix = nullptr, SeekDirection direction = SeekDirection::Forward) const override;

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
    MemoryStore& store_;
    std::unordered_map<io::ByteVector, io::ByteVector> snapshot_;
    std::unordered_map<io::ByteVector, io::ByteVector> changes_;
    std::unordered_set<io::ByteVector> deletions_;
};

/**
 * @brief A memory-based implementation of IStoreProvider.
 */
class MemoryStoreProvider : public IStoreProvider
{
   public:
    /**
     * @brief Constructs a MemoryStoreProvider.
     */
    MemoryStoreProvider();

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
    std::unordered_map<std::string, std::shared_ptr<MemoryStore>> stores_;
    std::mutex mutex_;
};
}  // namespace neo::persistence
