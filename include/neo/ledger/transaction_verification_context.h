#pragma once

#include <neo/ledger/transaction.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <unordered_set>
#include <unordered_map>
#include <memory>

namespace neo::ledger
{
    /**
     * @brief Enumeration for verification results.
     */
    enum class VerifyResult : uint8_t
    {
        Succeed = 0,
        AlreadyExists = 1,
        AlreadyInPool = 2,
        OutOfMemory = 3,
        UnableToVerify = 4,
        Invalid = 5,
        InvalidScript = 6,
        InvalidAttribute = 7,
        InvalidSignature = 8,
        OverSize = 9,
        Expired = 10,
        InsufficientFunds = 11,
        PolicyFail = 12,
        HasConflicts = 13,
        Unknown = 14
    };

    /**
     * @brief Enumeration for transaction removal reasons.
     */
    enum class TransactionRemovalReason : uint8_t
    {
        Expired = 0,
        LowPriority = 1,
        InvalidTransaction = 2,
        Replaced = 3,
        BlockPersisted = 4
    };

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
        // Track used transaction outputs (prevhash:index -> transaction hash)
        std::unordered_map<std::string, io::UInt256> used_outputs_;
        
        // Track account conflicts (account -> transaction hash)
        std::unordered_map<io::UInt160, io::UInt256> account_conflicts_;
        
        // Set of transaction hashes in this context
        std::unordered_set<io::UInt256> transaction_hashes_;

        // Helper methods
        std::string MakeOutputKey(const io::UInt256& prev_hash, uint32_t index) const;
        bool HasOutputConflict(std::shared_ptr<Transaction> transaction) const;
        bool HasAccountConflict(std::shared_ptr<Transaction> transaction) const;
    };

    /**
     * @brief Event arguments for transaction removal.
     */
    class TransactionRemovedEventArgs
    {
    public:
        /**
         * @brief Constructor.
         * @param transaction The removed transaction.
         * @param reason The removal reason.
         */
        TransactionRemovedEventArgs(std::shared_ptr<Transaction> transaction, TransactionRemovalReason reason);

        /**
         * @brief Gets the removed transaction.
         * @return The transaction.
         */
        std::shared_ptr<Transaction> GetTransaction() const;

        /**
         * @brief Gets the removal reason.
         * @return The removal reason.
         */
        TransactionRemovalReason GetReason() const;

    private:
        std::shared_ptr<Transaction> transaction_;
        TransactionRemovalReason reason_;
    };
}
