#pragma once

#include <neo/ledger/transaction.h>
#include <neo/io/uint256.h>
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
     * @brief Context for verifying transactions to prevent conflicts.
     */
    class TransactionVerificationContext
    {
    public:
        /**
         * @brief Constructor.
         */
        TransactionVerificationContext();

        /**
         * @brief Destructor.
         */
        ~TransactionVerificationContext() = default;

        /**
         * @brief Adds a transaction to the verification context.
         * @param transaction The transaction to add.
         * @return The verification result.
         */
        VerifyResult AddTransaction(std::shared_ptr<Transaction> transaction);

        /**
         * @brief Removes a transaction from the verification context.
         * @param hash The transaction hash.
         * @return True if removed, false if not found.
         */
        bool RemoveTransaction(const io::UInt256& hash);

        /**
         * @brief Checks if a transaction exists in the context.
         * @param hash The transaction hash.
         * @return True if exists, false otherwise.
         */
        bool Contains(const io::UInt256& hash) const;

        /**
         * @brief Gets the number of transactions in the context.
         * @return The number of transactions.
         */
        size_t Count() const;

        /**
         * @brief Clears all transactions from the context.
         */
        void Clear();

        /**
         * @brief Checks if adding a transaction would cause conflicts.
         * @param transaction The transaction to check.
         * @return The verification result.
         */
        VerifyResult CheckTransaction(std::shared_ptr<Transaction> transaction) const;

        /**
         * @brief Gets all transaction hashes in the context.
         * @return A set of transaction hashes.
         */
        std::unordered_set<io::UInt256> GetTransactionHashes() const;

    private:
        std::unordered_set<io::UInt256> transaction_hashes_;
        std::unordered_map<io::UInt256, std::shared_ptr<Transaction>> transactions_;

        /**
         * @brief Checks for conflicts with existing transactions.
         * @param transaction The transaction to check.
         * @return The verification result.
         */
        VerifyResult CheckConflicts(std::shared_ptr<Transaction> transaction) const;

        /**
         * @brief Validates the transaction structure and content.
         * @param transaction The transaction to validate.
         * @return The verification result.
         */
        VerifyResult ValidateTransaction(std::shared_ptr<Transaction> transaction) const;
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
