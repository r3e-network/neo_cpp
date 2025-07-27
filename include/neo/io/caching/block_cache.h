#pragma once

#include <functional>
#include <neo/io/caching/lru_cache.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>

namespace neo::io::caching
{
/**
 * @brief A cache for Block objects.
 */
class BlockCache
{
  public:
    /**
     * @brief Constructs a BlockCache with the specified capacity.
     * @param capacity The maximum number of items the cache can hold.
     */
    explicit BlockCache(size_t capacity) : hashCache_(capacity), indexCache_(capacity) {}

    /**
     * @brief Adds a block to the cache.
     * @param block The block to add.
     */
    void Add(std::shared_ptr<ledger::Block> block)
    {
        if (!block)
            return;

        // Add to hash cache
        hashCache_.Add(block->GetHash(), block);

        // Add to index cache
        indexCache_.Add(block->GetIndex(), block);
    }

    /**
     * @brief Gets a block by hash.
     * @param hash The hash of the block.
     * @return The block if found, std::nullopt otherwise.
     */
    std::optional<std::shared_ptr<ledger::Block>> GetByHash(const UInt256& hash)
    {
        return hashCache_.Get(hash);
    }

    /**
     * @brief Gets a block by index.
     * @param index The index of the block.
     * @return The block if found, std::nullopt otherwise.
     */
    std::optional<std::shared_ptr<ledger::Block>> GetByIndex(uint32_t index)
    {
        return indexCache_.Get(index);
    }

    /**
     * @brief Tries to get a block by hash.
     * @param hash The hash of the block.
     * @param block The block if found.
     * @return True if the block was found, false otherwise.
     */
    bool TryGetByHash(const UInt256& hash, std::shared_ptr<ledger::Block>& block)
    {
        return hashCache_.TryGet(hash, block);
    }

    /**
     * @brief Tries to get a block by index.
     * @param index The index of the block.
     * @param block The block if found.
     * @return True if the block was found, false otherwise.
     */
    bool TryGetByIndex(uint32_t index, std::shared_ptr<ledger::Block>& block)
    {
        return indexCache_.TryGet(index, block);
    }

    /**
     * @brief Removes a block by hash.
     * @param hash The hash of the block.
     * @return True if the block was removed, false otherwise.
     */
    bool RemoveByHash(const UInt256& hash)
    {
        // Get the block
        auto block = hashCache_.Get(hash);
        if (!block)
            return false;

        // Remove from index cache
        indexCache_.Remove(block.value()->GetIndex());

        // Remove from hash cache
        return hashCache_.Remove(hash);
    }

    /**
     * @brief Removes a block by index.
     * @param index The index of the block.
     * @return True if the block was removed, false otherwise.
     */
    bool RemoveByIndex(uint32_t index)
    {
        // Get the block
        auto block = indexCache_.Get(index);
        if (!block)
            return false;

        // Remove from hash cache
        hashCache_.Remove(block.value()->GetHash());

        // Remove from index cache
        return indexCache_.Remove(index);
    }

    /**
     * @brief Clears the cache.
     */
    void Clear()
    {
        hashCache_.Clear();
        indexCache_.Clear();
    }

    /**
     * @brief Gets the number of items in the cache.
     * @return The number of items in the cache.
     */
    size_t Size() const
    {
        return hashCache_.Size();
    }

    /**
     * @brief Gets the capacity of the cache.
     * @return The capacity of the cache.
     */
    size_t Capacity() const
    {
        return hashCache_.Capacity();
    }

  private:
    LRUCache<UInt256, std::shared_ptr<ledger::Block>> hashCache_;
    LRUCache<uint32_t, std::shared_ptr<ledger::Block>> indexCache_;
};
}  // namespace neo::io::caching
