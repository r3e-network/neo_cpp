#pragma once

#include <functional>
#include <memory>
#include <neo/persistence/istore.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/store_view.h>
#include <optional>
#include <unordered_map>

namespace neo::persistence
{
/**
 * @brief Enum for tracking the state of an item in the cache.
 */
enum class TrackState
{
    None,
    Added,
    Changed,
    Deleted
};

/**
 * @brief Base class for data caches.
 */
class DataCache : public StoreView
{
  public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~DataCache() = default;

    /**
     * @brief Gets a storage item from the cache.
     * @param key The key to look up.
     * @return The storage item.
     * @throws std::out_of_range if the key is not found.
     */
    virtual StorageItem& Get(const StorageKey& key) = 0;

    /**
     * @brief Gets a pointer to a storage item from the cache.
     * @param key The key to look up.
     * @return Pointer to the storage item, or nullptr if not found.
     */
    virtual std::shared_ptr<StorageItem> TryGet(const StorageKey& key) override = 0;

    /**
     * @brief Gets a storage item from the cache and marks it as changed.
     * If the item doesn't exist, the factory function is called to create it.
     * @param key The key to look up.
     * @param factory Optional factory function to create the item if it doesn't exist.
     * @return Pointer to the storage item, or nullptr if not found and no factory provided.
     */
    virtual std::shared_ptr<StorageItem>
    GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory = nullptr) override = 0;

    /**
     * @brief Creates a snapshot of the cache.
     * @return The snapshot.
     */
    virtual std::shared_ptr<StoreView> CreateSnapshot() override = 0;

    /**
     * @brief Gets the current block index.
     * @return The current block index.
     */
    virtual uint32_t GetCurrentBlockIndex() const = 0;
};

/**
 * @brief A cache for a store.
 */
class StoreCache : public DataCache
{
  public:
    /**
     * @brief Constructs a StoreCache.
     * @param store The store to cache.
     */
    explicit StoreCache(IStore& store);

    /**
     * @brief Constructs a StoreCache from a store snapshot.
     * @param snapshot The store snapshot.
     */
    explicit StoreCache(std::shared_ptr<IStoreSnapshot> snapshot);

    /**
     * @brief Tries to get a storage item from the cache.
     * @param key The key to look up.
     * @return The storage item if found, std::nullopt otherwise.
     */
    std::optional<StorageItem> TryGet(const StorageKey& key) const override;

    /**
     * @brief Gets a pointer to a storage item from the cache.
     * @param key The key to look up.
     * @return Pointer to the storage item, or nullptr if not found.
     */
    std::shared_ptr<StorageItem> TryGet(const StorageKey& key) override;

    /**
     * @brief Adds a storage item to the cache.
     * @param key The key.
     * @param item The storage item.
     */
    void Add(const StorageKey& key, const StorageItem& item) override;

    /**
     * @brief Gets a storage item from the cache.
     * @param key The key to look up.
     * @return The storage item.
     * @throws std::out_of_range if the key is not found.
     */
    StorageItem& Get(const StorageKey& key) override;

    /**
     * @brief Gets a storage item from the cache and marks it as changed.
     * If the item doesn't exist, the factory function is called to create it.
     * @param key The key to look up.
     * @param factory Optional factory function to create the item if it doesn't exist.
     * @return Pointer to the storage item, or nullptr if not found and no factory provided.
     */
    std::shared_ptr<StorageItem> GetAndChange(const StorageKey& key,
                                              std::function<std::shared_ptr<StorageItem>()> factory = nullptr) override;

    /**
     * @brief Deletes a storage item from the cache.
     * @param key The key.
     */
    void Delete(const StorageKey& key) override;

    /**
     * @brief Finds all storage items with keys that start with the specified prefix.
     * @param prefix The prefix to search for. If nullptr, all storage items are returned.
     * @return The storage items found.
     */
    std::vector<std::pair<StorageKey, StorageItem>> Find(const StorageKey* prefix = nullptr) const override;

    /**
     * @brief Creates an iterator for storage items with the specified prefix.
     * @param prefix The prefix.
     * @return The storage iterator.
     */
    std::unique_ptr<StorageIterator> Seek(const StorageKey& prefix) const override;

    /**
     * @brief Creates a snapshot of the cache.
     * @return The snapshot.
     */
    std::shared_ptr<StoreView> CreateSnapshot() override;

    /**
     * @brief Commits the changes to the underlying store.
     */
    void Commit() override;

    /**
     * @brief Gets the current block index.
     * @return The current block index.
     */
    uint32_t GetCurrentBlockIndex() const override;

    /**
     * @brief Checks if a key exists in the cache.
     * @param key The key to check.
     * @return True if the key exists, false otherwise.
     */
    bool Contains(const StorageKey& key) const;

    /**
     * @brief Gets the track state of a key.
     * @param key The key.
     * @return The track state.
     */
    TrackState GetTrackState(const StorageKey& key) const;

    /**
     * @brief Updates an existing storage item in the cache.
     * @param key The key.
     * @param item The storage item.
     */
    void Update(const StorageKey& key, const StorageItem& item);

    /**
     * @brief Gets the number of items in the cache.
     * @return The number of items.
     */
    size_t Count() const;

    /**
     * @brief Gets all tracked items.
     * @return Vector of tracked items with their states.
     */
    std::vector<std::pair<StorageKey, std::pair<StorageItem, TrackState>>> GetTrackedItems() const;

    /**
     * @brief Gets all changed items.
     * @return Vector of changed items.
     */
    std::vector<std::pair<StorageKey, StorageItem>> GetChangedItems() const;

    /**
     * @brief Gets all deleted items.
     * @return Vector of deleted items.
     */
    std::vector<StorageKey> GetDeletedItems() const;

    /**
     * @brief Tries to get a storage item with output parameter.
     * @param key The key.
     * @param item The output item.
     * @return True if found, false otherwise.
     */
    bool TryGet(const StorageKey& key, StorageItem& item) const;

    /**
     * @brief Gets the underlying store.
     * @return The underlying store.
     */
    std::shared_ptr<IStoreSnapshot> GetStore() const;

    /**
     * @brief Checks if the cache is read-only.
     * @return True if read-only, false otherwise.
     */
    bool IsReadOnly() const;

  private:
    IStore& store_;
    std::shared_ptr<IStoreSnapshot> snapshot_;
    std::unordered_map<StorageKey, std::pair<StorageItem, TrackState>> items_;
};

// ClonedCache is now defined as a template in cloned_cache.h
}  // namespace neo::persistence
