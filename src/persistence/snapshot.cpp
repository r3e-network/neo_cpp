/**
 * @file snapshot.cpp
 * @brief Implementation of the Snapshot storage component for Neo C++
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/persistence/snapshot.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/core/exceptions.h>

#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <vector>
#include <algorithm>

namespace neo::persistence
{

/**
 * @class SnapshotImpl
 * @brief Internal implementation of the Snapshot class
 */
class Snapshot::Impl
{
public:
    using CacheMap = std::unordered_map<StorageKey, std::shared_ptr<StorageItem>>;
    
    // Parent snapshot for chaining
    std::shared_ptr<Snapshot> parent;
    
    // Local cache for this snapshot
    CacheMap localCache;
    
    // Deleted keys in this snapshot
    std::unordered_set<StorageKey> deletedKeys;
    
    // Read-write lock for thread safety
    mutable std::shared_mutex mutex;
    
    // Block height for this snapshot
    uint32_t blockHeight;
    
    // Timestamp when snapshot was created
    uint64_t timestamp;
    
    // Whether this snapshot has been committed
    bool committed = false;
    
    Impl(uint32_t height) 
        : blockHeight(height)
        , timestamp(std::chrono::system_clock::now().time_since_epoch().count())
    {
    }
};

Snapshot::Snapshot(uint32_t blockHeight)
    : pImpl(std::make_unique<Impl>(blockHeight))
{
}

Snapshot::Snapshot(std::shared_ptr<Snapshot> parent)
    : pImpl(std::make_unique<Impl>(parent ? parent->GetBlockHeight() + 1 : 0))
{
    pImpl->parent = parent;
}

Snapshot::~Snapshot() = default;

std::shared_ptr<StorageItem> Snapshot::Get(const StorageKey& key) const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    
    // Check if key was deleted in this snapshot
    if (pImpl->deletedKeys.find(key) != pImpl->deletedKeys.end())
    {
        return nullptr;
    }
    
    // Check local cache first
    auto it = pImpl->localCache.find(key);
    if (it != pImpl->localCache.end())
    {
        return it->second;
    }
    
    // Check parent snapshot if exists
    if (pImpl->parent)
    {
        return pImpl->parent->Get(key);
    }
    
    return nullptr;
}

void Snapshot::Put(const StorageKey& key, const std::shared_ptr<StorageItem>& value)
{
    std::unique_lock<std::shared_mutex> lock(pImpl->mutex);
    
    if (pImpl->committed)
    {
        throw std::runtime_error("Cannot modify committed snapshot");
    }
    
    // Remove from deleted keys if present
    pImpl->deletedKeys.erase(key);
    
    // Add to local cache
    pImpl->localCache[key] = value;
}

void Snapshot::Delete(const StorageKey& key)
{
    std::unique_lock<std::shared_mutex> lock(pImpl->mutex);
    
    if (pImpl->committed)
    {
        throw std::runtime_error("Cannot modify committed snapshot");
    }
    
    // Remove from local cache
    pImpl->localCache.erase(key);
    
    // Add to deleted keys
    pImpl->deletedKeys.insert(key);
}

bool Snapshot::Contains(const StorageKey& key) const
{
    return Get(key) != nullptr;
}

void Snapshot::Commit()
{
    std::unique_lock<std::shared_mutex> lock(pImpl->mutex);
    
    if (pImpl->committed)
    {
        return; // Already committed
    }
    
    if (pImpl->parent)
    {
        // Merge changes into parent
        for (const auto& [key, value] : pImpl->localCache)
        {
            pImpl->parent->Put(key, value);
        }
        
        for (const auto& key : pImpl->deletedKeys)
        {
            pImpl->parent->Delete(key);
        }
    }
    
    pImpl->committed = true;
}

void Snapshot::Rollback()
{
    std::unique_lock<std::shared_mutex> lock(pImpl->mutex);
    
    if (pImpl->committed)
    {
        throw std::runtime_error("Cannot rollback committed snapshot");
    }
    
    // Clear all local changes
    pImpl->localCache.clear();
    pImpl->deletedKeys.clear();
}

std::shared_ptr<Snapshot> Snapshot::Clone() const
{
    auto clone = std::make_shared<Snapshot>(pImpl->blockHeight);
    
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    
    // Copy parent reference
    clone->pImpl->parent = pImpl->parent;
    
    // Deep copy local cache
    for (const auto& [key, value] : pImpl->localCache)
    {
        clone->pImpl->localCache[key] = std::make_shared<StorageItem>(*value);
    }
    
    // Copy deleted keys
    clone->pImpl->deletedKeys = pImpl->deletedKeys;
    
    // Copy metadata
    clone->pImpl->timestamp = pImpl->timestamp;
    
    return clone;
}

std::shared_ptr<Snapshot> Snapshot::CreateChild() const
{
    return std::make_shared<Snapshot>(std::const_pointer_cast<Snapshot>(shared_from_this()));
}

uint32_t Snapshot::GetBlockHeight() const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    return pImpl->blockHeight;
}

uint64_t Snapshot::GetTimestamp() const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    return pImpl->timestamp;
}

bool Snapshot::IsCommitted() const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    return pImpl->committed;
}

size_t Snapshot::GetChangeCount() const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    return pImpl->localCache.size() + pImpl->deletedKeys.size();
}

std::vector<StorageKey> Snapshot::GetChangedKeys() const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    
    std::vector<StorageKey> keys;
    keys.reserve(pImpl->localCache.size() + pImpl->deletedKeys.size());
    
    // Add modified/added keys
    for (const auto& [key, _] : pImpl->localCache)
    {
        keys.push_back(key);
    }
    
    // Add deleted keys
    for (const auto& key : pImpl->deletedKeys)
    {
        keys.push_back(key);
    }
    
    return keys;
}

void Snapshot::Clear()
{
    std::unique_lock<std::shared_mutex> lock(pImpl->mutex);
    
    if (pImpl->committed)
    {
        throw std::runtime_error("Cannot clear committed snapshot");
    }
    
    pImpl->localCache.clear();
    pImpl->deletedKeys.clear();
}

std::vector<std::pair<StorageKey, std::shared_ptr<StorageItem>>> 
Snapshot::GetAll() const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    
    std::vector<std::pair<StorageKey, std::shared_ptr<StorageItem>>> result;
    
    // Collect from parent first if exists
    if (pImpl->parent)
    {
        auto parentItems = pImpl->parent->GetAll();
        
        // Filter out deleted items and apply local overrides
        for (const auto& [key, value] : parentItems)
        {
            // Skip if deleted in this snapshot
            if (pImpl->deletedKeys.find(key) != pImpl->deletedKeys.end())
            {
                continue;
            }
            
            // Check if overridden in local cache
            auto localIt = pImpl->localCache.find(key);
            if (localIt != pImpl->localCache.end())
            {
                result.emplace_back(key, localIt->second);
            }
            else
            {
                result.emplace_back(key, value);
            }
        }
    }
    
    // Add items that are only in local cache
    for (const auto& [key, value] : pImpl->localCache)
    {
        // Check if not already added from parent
        bool found = false;
        if (pImpl->parent)
        {
            found = std::any_of(result.begin(), result.end(),
                [&key](const auto& pair) { return pair.first == key; });
        }
        
        if (!found)
        {
            result.emplace_back(key, value);
        }
    }
    
    return result;
}

std::shared_ptr<Snapshot> Snapshot::Merge(const std::shared_ptr<Snapshot>& other)
{
    if (!other)
    {
        return shared_from_this();
    }
    
    std::unique_lock<std::shared_mutex> lock(pImpl->mutex);
    std::shared_lock<std::shared_mutex> otherLock(other->pImpl->mutex);
    
    if (pImpl->committed)
    {
        throw std::runtime_error("Cannot merge into committed snapshot");
    }
    
    // Apply all changes from other snapshot
    for (const auto& [key, value] : other->pImpl->localCache)
    {
        pImpl->localCache[key] = value;
        pImpl->deletedKeys.erase(key);
    }
    
    for (const auto& key : other->pImpl->deletedKeys)
    {
        pImpl->localCache.erase(key);
        pImpl->deletedKeys.insert(key);
    }
    
    return shared_from_this();
}

size_t Snapshot::GetMemoryUsage() const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    
    size_t usage = sizeof(Snapshot) + sizeof(Impl);
    
    // Estimate memory usage of local cache
    usage += pImpl->localCache.size() * (sizeof(StorageKey) + sizeof(std::shared_ptr<StorageItem>));
    
    // Estimate memory usage of deleted keys
    usage += pImpl->deletedKeys.size() * sizeof(StorageKey);
    
    // Add estimated storage item sizes
    for (const auto& [_, item] : pImpl->localCache)
    {
        if (item)
        {
            usage += item->GetValue().size();
        }
    }
    
    return usage;
}

void Snapshot::Validate() const
{
    std::shared_lock<std::shared_mutex> lock(pImpl->mutex);
    
    // Check for consistency
    for (const auto& key : pImpl->deletedKeys)
    {
        if (pImpl->localCache.find(key) != pImpl->localCache.end())
        {
            throw std::runtime_error("Inconsistent snapshot: key exists in both cache and deleted set");
        }
    }
    
    // Validate parent chain
    std::shared_ptr<Snapshot> current = pImpl->parent;
    std::unordered_set<Snapshot*> visited;
    visited.insert(const_cast<Snapshot*>(this));
    
    while (current)
    {
        if (visited.find(current.get()) != visited.end())
        {
            throw std::runtime_error("Circular reference detected in snapshot chain");
        }
        visited.insert(current.get());
        current = current->pImpl->parent;
    }
}

// Factory method
std::shared_ptr<Snapshot> Snapshot::Create(uint32_t blockHeight)
{
    return std::make_shared<Snapshot>(blockHeight);
}

std::shared_ptr<Snapshot> Snapshot::CreateGenesis()
{
    return Create(0);
}

} // namespace neo::persistence