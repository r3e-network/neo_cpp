#pragma once

#include <functional>
#include <memory>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <optional>
#include <unordered_map>
#include <vector>

namespace neo::persistence
{
/**
 * @brief Iterator for storage items.
 */
class StorageIterator
{
  public:
    virtual ~StorageIterator() = default;

    /**
     * @brief Checks if the iterator is valid.
     * @return True if valid, false otherwise.
     */
    virtual bool Valid() const = 0;

    /**
     * @brief Gets the current key.
     * @return The current key.
     */
    virtual StorageKey Key() const = 0;

    /**
     * @brief Gets the current value.
     * @return The current value.
     */
    virtual StorageItem Value() const = 0;

    /**
     * @brief Moves to the next item.
     */
    virtual void Next() = 0;
};
}  // namespace neo::persistence

namespace neo::persistence
{
/**
 * @brief Represents a view of a storage.
 */
class StoreView
{
  public:
    /**
     * @brief Constructs an empty StoreView.
     */
    StoreView() = default;

    /**
     * @brief Virtual destructor.
     */
    virtual ~StoreView() = default;

    /**
     * @brief Gets a storage item by key.
     * @param key The key.
     * @return The storage item, or std::nullopt if not found.
     */
    virtual std::optional<StorageItem> TryGet(const StorageKey& key) const = 0;

    /**
     * @brief Gets a pointer to a storage item by key.
     * @param key The key.
     * @return Pointer to the storage item, or nullptr if not found.
     */
    virtual std::shared_ptr<StorageItem> TryGet(const StorageKey& key) = 0;

    /**
     * @brief Gets a storage item by key, creating it if it doesn't exist.
     * @param key The key.
     * @param factory Optional factory function to create new item.
     * @return Pointer to the storage item.
     */
    virtual std::shared_ptr<StorageItem>
    GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory = nullptr) = 0;

    /**
     * @brief Adds or updates a storage item.
     * @param key The key.
     * @param item The storage item.
     */
    virtual void Add(const StorageKey& key, const StorageItem& item) = 0;

    /**
     * @brief Deletes a storage item.
     * @param key The key.
     */
    virtual void Delete(const StorageKey& key) = 0;

    /**
     * @brief Finds storage items by prefix.
     * @param prefix The prefix.
     * @return The storage items.
     */
    virtual std::vector<std::pair<StorageKey, StorageItem>> Find(const StorageKey* prefix = nullptr) const = 0;

    /**
     * @brief Creates an iterator for storage items with the specified prefix.
     * @param prefix The prefix.
     * @return The storage iterator.
     */
    virtual std::unique_ptr<StorageIterator> Seek(const StorageKey& prefix) const = 0;

    /**
     * @brief Commits the changes.
     */
    virtual void Commit() = 0;

    /**
     * @brief Creates a snapshot of the store view.
     * @return The snapshot.
     */
    virtual std::shared_ptr<StoreView> CreateSnapshot() = 0;
};
}  // namespace neo::persistence
