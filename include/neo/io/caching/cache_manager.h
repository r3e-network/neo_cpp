#pragma once

#include <neo/io/caching/block_cache.h>
#include <neo/io/caching/contract_cache.h>
#include <neo/io/caching/ecpoint_cache.h>
#include <neo/io/caching/transaction_cache.h>

#include <memory>

namespace neo::io::caching
{
/**
 * @brief A singleton manager for all caches.
 */
class CacheManager
{
   public:
    /**
     * @brief Gets the singleton instance of the CacheManager.
     * @return The singleton instance.
     */
    static CacheManager& GetInstance()
    {
        static CacheManager instance;
        return instance;
    }

    /**
     * @brief Gets the ECPoint cache.
     * @return The ECPoint cache.
     */
    ECPointCache& GetECPointCache() { return *ecpointCache_; }

    /**
     * @brief Gets the block cache.
     * @return The block cache.
     */
    BlockCache& GetBlockCache() { return *blockCache_; }

    /**
     * @brief Gets the transaction cache.
     * @return The transaction cache.
     */
    TransactionCache& GetTransactionCache() { return *transactionCache_; }

    /**
     * @brief Gets the contract cache.
     * @return The contract cache.
     */
    ContractCache& GetContractCache() { return *contractCache_; }

    /**
     * @brief Clears all caches.
     */
    void ClearAll()
    {
        ecpointCache_->Clear();
        blockCache_->Clear();
        transactionCache_->Clear();
        contractCache_->Clear();
    }

   private:
    /**
     * @brief Constructs a CacheManager.
     */
    CacheManager()
        : ecpointCache_(std::make_unique<ECPointCache>(1000)),
          blockCache_(std::make_unique<BlockCache>(1000)),
          transactionCache_(std::make_unique<TransactionCache>(10000)),
          contractCache_(std::make_unique<ContractCache>(1000))
    {
    }

    /**
     * @brief Destructs a CacheManager.
     */
    ~CacheManager() = default;

    // Delete copy constructor and assignment operator
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    std::unique_ptr<ECPointCache> ecpointCache_;
    std::unique_ptr<BlockCache> blockCache_;
    std::unique_ptr<TransactionCache> transactionCache_;
    std::unique_ptr<ContractCache> contractCache_;
};
}  // namespace neo::io::caching
