/**
 * @file transaction_storage.h
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/fixed8.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/data_cache.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace neo::ledger
{
/**
 * @brief Handles transaction storage and retrieval.
 */
class TransactionStorage
{
   public:
    /**
     * @brief Constructs a TransactionStorage.
     * @param dataCache The data cache.
     */
    explicit TransactionStorage(std::shared_ptr<persistence::DataCache> dataCache);

    /**
     * @brief Gets a transaction by hash.
     * @param hash The hash of the transaction.
     * @return The transaction, or nullptr if not found.
     */
    std::shared_ptr<Transaction> GetTransaction(const io::UInt256& hash) const;

    /**
     * @brief Adds a transaction to storage.
     * @param transaction The transaction to add.
     * @param snapshot The snapshot to use.
     * @return True if the transaction was added, false otherwise.
     */
    bool AddTransaction(const Transaction& transaction, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Checks if a transaction exists.
     * @param hash The hash of the transaction.
     * @return True if the transaction exists, false otherwise.
     */
    bool ContainsTransaction(const io::UInt256& hash) const;

    /**
     * @brief Gets the unspent transaction outputs for a transaction.
     * @param hash The hash of the transaction.
     * @return The unspent transaction outputs.
     */
    std::vector<TransactionOutput> GetUnspentOutputs(const io::UInt256& hash) const;

    /**
     * @brief Gets the unspent transaction outputs for an address.
     * @param scriptHash The script hash of the address.
     * @return The unspent transaction outputs.
     */
    std::vector<TransactionOutput> GetUnspentOutputs(const io::UInt160& scriptHash) const;

    /**
     * @brief Gets the balance of an address.
     * @param scriptHash The script hash of the address.
     * @param assetId The asset ID.
     * @return The balance.
     */
    io::Fixed8 GetBalance(const io::UInt160& scriptHash, const io::UInt256& assetId) const;

   private:
    std::shared_ptr<persistence::DataCache> dataCache_;
    mutable std::mutex mutex_;
    mutable std::unordered_map<io::UInt256, std::shared_ptr<Transaction>> transactions_;
    mutable std::unordered_map<io::UInt256, std::vector<TransactionOutput>> unspentOutputs_;
    mutable std::unordered_map<io::UInt160, std::vector<TransactionOutput>> addressOutputs_;
};
}  // namespace neo::ledger
