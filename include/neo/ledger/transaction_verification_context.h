#pragma once

#include <neo/ledger/transaction.h>
#include <neo/ledger/verify_result.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <unordered_set>
#include <unordered_map>
#include <memory>

namespace neo::ledger
{

    /**
     * @brief Context for transaction verification to track conflicts and state.
     * 
     * ## Overview
     * The TransactionVerificationContext tracks transaction verification state
     * to detect conflicts and ensure proper validation during block processing
     * and mempool operations.
     * 
     * ## API Reference
     * - **Conflict Detection**: CheckTransaction(), AddTransaction()
     * - **State Management**: Reset(), Clear()
     * - **Validation**: IsConflicted()
     * 
     * ## Usage Examples
     * ```cpp
     * TransactionVerificationContext context;
     * if (context.CheckTransaction(tx)) {
     *     context.AddTransaction(tx);
     * }
     * ```
     */
    class TransactionVerificationContext
    {
    public:
        /**
         * @brief Default constructor
         */
        TransactionVerificationContext();

        /**
         * @brief Destructor
         */
        ~TransactionVerificationContext();

        /**
         * @brief Checks if a transaction can be added without conflicts.
         * @param transaction The transaction to check
         * @return True if the transaction can be added, false if there are conflicts
         */
        bool CheckTransaction(std::shared_ptr<Transaction> transaction);

        /**
         * @brief Adds a transaction to the verification context.
         * @param transaction The transaction to add
         */
        void AddTransaction(std::shared_ptr<Transaction> transaction);

        /**
         * @brief Checks if there are any conflicts for the given transaction.
         * @param transaction The transaction to check
         * @return True if there are conflicts, false otherwise
         */
        bool IsConflicted(std::shared_ptr<Transaction> transaction) const;

        /**
         * @brief Resets the verification context.
         */
        void Reset();

        /**
         * @brief Clears all tracked state.
         */
        void Clear();

        /**
         * @brief Gets the number of transactions in the context.
         * @return Number of transactions
         */
        size_t GetTransactionCount() const;

    private:
        // Track account conflicts (account -> transaction hash)
        std::unordered_map<io::UInt160, io::UInt256> account_conflicts_;
        
        // Set of transaction hashes in this context
        std::unordered_set<io::UInt256> transaction_hashes_;

        // Helper methods (Neo N3 uses account-based model)
        bool HasOutputConflict(std::shared_ptr<Transaction> transaction) const;
        bool HasAccountConflict(std::shared_ptr<Transaction> transaction) const;
    };

    // TransactionRemovedEventArgs is defined in memory_pool.h
}
