#pragma once

#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>

namespace neo::persistence
{
/**
 * @brief Adapter class to provide convenient Put/Add methods for DataCache
 * 
 * This adapter wraps DataCache to provide the Put method that was used
 * in the original implementation but isn't part of the base DataCache interface.
 */
class DataCacheAdapter
{
private:
    DataCache* cache_;
    
public:
    explicit DataCacheAdapter(DataCache* cache) : cache_(cache) {}
    
    /**
     * @brief Put a value into the cache with the specified key
     * @param key The storage key
     * @param value The value to store
     */
    void Put(const StorageKey& key, const std::vector<uint8_t>& value)
    {
        if (!cache_) return;
        
        // Use GetAndChange to create or update the entry
        auto item = cache_->GetAndChange(key, [&value]() {
            auto new_item = std::make_shared<StorageItem>();
            new_item->SetValue(value);
            return new_item;
        });
        
        if (item)
        {
            item->SetValue(value);
        }
    }
    
    /**
     * @brief Put a value into the cache with raw key bytes
     * @param contract_id The contract ID
     * @param key_bytes The raw key bytes
     * @param value The value to store
     */
    void Put(int32_t contract_id, const std::vector<uint8_t>& key_bytes, const std::vector<uint8_t>& value)
    {
        StorageKey key(contract_id, key_bytes);
        Put(key, value);
    }
    
    /**
     * @brief Get a value from the cache
     * @param key The storage key
     * @return The stored value, or empty optional if not found
     */
    std::optional<std::vector<uint8_t>> Get(const StorageKey& key)
    {
        if (!cache_) return std::nullopt;
        
        auto item = cache_->TryGet(key);
        if (item)
        {
            return item->GetValue();
        }
        return std::nullopt;
    }
    
    /**
     * @brief Get a value from the cache with raw key bytes
     * @param contract_id The contract ID
     * @param key_bytes The raw key bytes
     * @return The stored value, or empty optional if not found
     */
    std::optional<std::vector<uint8_t>> Get(int32_t contract_id, const std::vector<uint8_t>& key_bytes)
    {
        StorageKey key(contract_id, key_bytes);
        return Get(key);
    }
    
    /**
     * @brief Check if a key exists in the cache
     * @param key The storage key
     * @return True if the key exists
     */
    bool Contains(const StorageKey& key)
    {
        if (!cache_) return false;
        // Use TryGet to check existence
        return cache_->TryGet(key) != nullptr;
    }
    
    /**
     * @brief Delete a key from the cache
     * @param key The storage key
     */
    void Delete(const StorageKey& key)
    {
        if (!cache_) return;
        cache_->Delete(key);
    }
    
    /**
     * @brief Get the underlying DataCache
     * @return Pointer to the DataCache
     */
    DataCache* GetCache() { return cache_; }
};

} // namespace neo::persistence