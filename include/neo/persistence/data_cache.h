#pragma once

#include <neo/persistence/istore.h>
#include <neo/persistence/store_view.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>

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
        virtual std::shared_ptr<StorageItem> TryGet(const StorageKey& key) = 0;

        /**
         * @brief Gets a storage item from the cache and marks it as changed.
         * If the item doesn't exist, the factory function is called to create it.
         * @param key The key to look up.
         * @param factory Optional factory function to create the item if it doesn't exist.
         * @return Pointer to the storage item, or nullptr if not found and no factory provided.
         */
        virtual std::shared_ptr<StorageItem> GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory = nullptr) = 0;





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
        std::shared_ptr<StorageItem> GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory = nullptr) override;

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

    private:
        IStore& store_;
        std::unordered_map<StorageKey, std::pair<StorageItem, TrackState>> items_;
    };

    /**
     * @brief A snapshot of a cache.
     */
    class ClonedCache : public DataCache
    {
    public:
        /**
         * @brief Constructs a ClonedCache.
         * @param innerCache The inner cache.
         */
        explicit ClonedCache(DataCache& innerCache);

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
        std::shared_ptr<StorageItem> GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory = nullptr) override;

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
         * @brief Commits the changes to the underlying cache.
         */
        void Commit() override;

        /**
         * @brief Gets the current block index.
         * @return The current block index.
         */
        uint32_t GetCurrentBlockIndex() const override;

    private:
        DataCache& innerCache_;
        std::unordered_map<StorageKey, std::pair<StorageItem, TrackState>> items_;
    };
}
