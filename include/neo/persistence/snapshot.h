/**
 * @file snapshot.h
 * @brief Snapshot storage component for Neo C++
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#ifndef NEO_PERSISTENCE_SNAPSHOT_H
#define NEO_PERSISTENCE_SNAPSHOT_H

#include <memory>
#include <vector>
#include <unordered_set>
#include <chrono>

namespace neo::persistence
{

// Forward declarations
class StorageKey;
class StorageItem;

/**
 * @class Snapshot
 * @brief Provides a consistent view of storage at a specific point in time
 * 
 * The Snapshot class implements a copy-on-write mechanism for efficient
 * storage management. It allows for isolated changes that can be committed
 * or rolled back without affecting the underlying storage until explicitly
 * committed.
 */
class Snapshot : public std::enable_shared_from_this<Snapshot>
{
public:
    /**
     * @brief Construct a new root snapshot
     * @param blockHeight The block height for this snapshot
     */
    explicit Snapshot(uint32_t blockHeight = 0);
    
    /**
     * @brief Construct a child snapshot
     * @param parent The parent snapshot to inherit from
     */
    explicit Snapshot(std::shared_ptr<Snapshot> parent);
    
    /**
     * @brief Destructor
     */
    ~Snapshot();
    
    // Storage operations
    
    /**
     * @brief Get a storage item by key
     * @param key The storage key
     * @return The storage item if found, nullptr otherwise
     */
    std::shared_ptr<StorageItem> Get(const StorageKey& key) const;
    
    /**
     * @brief Put a storage item
     * @param key The storage key
     * @param value The storage item
     */
    void Put(const StorageKey& key, const std::shared_ptr<StorageItem>& value);
    
    /**
     * @brief Delete a storage item
     * @param key The storage key to delete
     */
    void Delete(const StorageKey& key);
    
    /**
     * @brief Check if a key exists
     * @param key The storage key
     * @return true if the key exists, false otherwise
     */
    bool Contains(const StorageKey& key) const;
    
    // Transaction management
    
    /**
     * @brief Commit all changes to parent snapshot or storage
     * 
     * After committing, this snapshot becomes read-only
     */
    void Commit();
    
    /**
     * @brief Rollback all uncommitted changes
     * 
     * Reverts all modifications made since the last commit
     */
    void Rollback();
    
    /**
     * @brief Create a deep copy of this snapshot
     * @return A new snapshot with the same data
     */
    std::shared_ptr<Snapshot> Clone() const;
    
    /**
     * @brief Create a child snapshot
     * @return A new snapshot that inherits from this one
     */
    std::shared_ptr<Snapshot> CreateChild() const;
    
    // Metadata
    
    /**
     * @brief Get the block height of this snapshot
     * @return The block height
     */
    uint32_t GetBlockHeight() const;
    
    /**
     * @brief Get the timestamp when this snapshot was created
     * @return The timestamp in nanoseconds since epoch
     */
    uint64_t GetTimestamp() const;
    
    /**
     * @brief Check if this snapshot has been committed
     * @return true if committed, false otherwise
     */
    bool IsCommitted() const;
    
    /**
     * @brief Get the number of changes in this snapshot
     * @return The count of modified and deleted items
     */
    size_t GetChangeCount() const;
    
    /**
     * @brief Get all keys that have been changed
     * @return Vector of changed storage keys
     */
    std::vector<StorageKey> GetChangedKeys() const;
    
    // Bulk operations
    
    /**
     * @brief Clear all changes in this snapshot
     * 
     * Removes all local modifications without affecting parent
     */
    void Clear();
    
    /**
     * @brief Get all key-value pairs visible in this snapshot
     * @return Vector of key-value pairs
     */
    std::vector<std::pair<StorageKey, std::shared_ptr<StorageItem>>> GetAll() const;
    
    /**
     * @brief Merge another snapshot into this one
     * @param other The snapshot to merge from
     * @return This snapshot for chaining
     */
    std::shared_ptr<Snapshot> Merge(const std::shared_ptr<Snapshot>& other);
    
    // Utilities
    
    /**
     * @brief Get estimated memory usage
     * @return Memory usage in bytes
     */
    size_t GetMemoryUsage() const;
    
    /**
     * @brief Validate snapshot consistency
     * @throws std::runtime_error if inconsistencies are found
     */
    void Validate() const;
    
    // Factory methods
    
    /**
     * @brief Create a new root snapshot
     * @param blockHeight The block height
     * @return A new snapshot instance
     */
    static std::shared_ptr<Snapshot> Create(uint32_t blockHeight = 0);
    
    /**
     * @brief Create a genesis snapshot
     * @return A new snapshot for the genesis block
     */
    static std::shared_ptr<Snapshot> CreateGenesis();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace neo::persistence

#endif // NEO_PERSISTENCE_SNAPSHOT_H