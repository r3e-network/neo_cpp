#pragma once

#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <memory>
#include <vector>

namespace neo::persistence
{
    /**
     * @brief Interface for storage view operations
     */
    class IStoreView
    {
    public:
        /**
         * @brief Virtual destructor
         */
        virtual ~IStoreView() = default;
        
        /**
         * @brief Get storage item by key
         * @param key Storage key
         * @return Storage item or nullptr if not found
         */
        virtual std::shared_ptr<StorageItem> Get(const StorageKey& key) = 0;
        
        /**
         * @brief Put storage item
         * @param key Storage key
         * @param item Storage item
         */
        virtual void Put(const StorageKey& key, const StorageItem& item) = 0;
        
        /**
         * @brief Delete storage item
         * @param key Storage key
         */
        virtual void Delete(const StorageKey& key) = 0;
        
        /**
         * @brief Check if key exists
         * @param key Storage key
         * @return True if key exists
         */
        virtual bool Contains(const StorageKey& key) const = 0;
        
        /**
         * @brief Get all items with given prefix
         * @param prefix Key prefix
         * @return Vector of key-value pairs
         */
        virtual std::vector<std::pair<StorageKey, StorageItem>> GetItemsWithPrefix(const StorageKey& prefix) const = 0;
    };
}