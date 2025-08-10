#pragma once

#include <neo/io/caching/lru_cache.h>
#include <neo/io/uint256.h>
#include <neo/ledger/transaction.h>

#include <functional>

namespace neo::io::caching
{
/**
 * @brief A cache for Transaction objects.
 */
class TransactionCache
{
   public:
    /**
     * @brief Constructs a TransactionCache with the specified capacity.
     * @param capacity The maximum number of items the cache can hold.
     */
    explicit TransactionCache(size_t capacity) : cache_(capacity) {}

    /**
     * @brief Adds a transaction to the cache.
     * @param transaction The transaction to add.
     */
    void Add(std::shared_ptr<ledger::Transaction> transaction)
    {
        if (!transaction) return;

        // Add to cache
        cache_.Add(transaction->GetHash(), transaction);
    }

    /**
     * @brief Gets a transaction by hash.
     * @param hash The hash of the transaction.
     * @return The transaction if found, std::nullopt otherwise.
     */
    std::optional<std::shared_ptr<ledger::Transaction>> Get(const UInt256& hash) { return cache_.Get(hash); }

    /**
     * @brief Tries to get a transaction by hash.
     * @param hash The hash of the transaction.
     * @param transaction The transaction if found.
     * @return True if the transaction was found, false otherwise.
     */
    bool TryGet(const UInt256& hash, std::shared_ptr<ledger::Transaction>& transaction)
    {
        return cache_.TryGet(hash, transaction);
    }

    /**
     * @brief Removes a transaction by hash.
     * @param hash The hash of the transaction.
     * @return True if the transaction was removed, false otherwise.
     */
    bool Remove(const UInt256& hash) { return cache_.Remove(hash); }

    /**
     * @brief Clears the cache.
     */
    void Clear() { cache_.Clear(); }

    /**
     * @brief Gets the number of items in the cache.
     * @return The number of items in the cache.
     */
    size_t Size() const { return cache_.Size(); }

    /**
     * @brief Gets the capacity of the cache.
     * @return The capacity of the cache.
     */
    size_t Capacity() const { return cache_.Capacity(); }

   private:
    LRUCache<UInt256, std::shared_ptr<ledger::Transaction>> cache_;
};
}  // namespace neo::io::caching
