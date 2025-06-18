#pragma once

#include <neo/network/p2p/payloads/block.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/ledger/verify_result.h>
#include <neo/persistence/data_cache.h>
#include <neo/config/protocol_settings.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>

namespace neo::ledger
{
    // Use Neo3Transaction from network namespace
    using Neo3Transaction = network::p2p::payloads::Neo3Transaction;
    
    // Forward declarations
    class NeoSystem;
    class TransactionVerificationContext;

    /**
     * @brief Reason for transaction removal from pool.
     */
    enum class TransactionRemovalReason
    {
        CapacityExceeded,
        Conflicted,
        InvalidOrExpired,
        BlockPersisted
    };

    /**
     * @brief Event args for transaction removal.
     */
    struct TransactionRemovedEventArgs
    {
        std::vector<std::shared_ptr<Neo3Transaction>> transactions;
        TransactionRemovalReason reason;
    };

    /**
     * @brief Pool item that wraps a transaction with priority information.
     * This matches the C# PoolItem structure exactly.
     */
    class PoolItem
    {
    public:
        std::shared_ptr<Neo3Transaction> tx;
        uint64_t timestamp;
        uint64_t fee_per_byte;
        
        explicit PoolItem(std::shared_ptr<Neo3Transaction> transaction);
        
        // Comparison operators for sorting (higher fee per byte = higher priority)
        bool operator<(const PoolItem& other) const;
        bool operator>(const PoolItem& other) const;
        bool operator==(const PoolItem& other) const;
        int CompareTo(const PoolItem& other) const;
        int CompareTo(std::shared_ptr<Neo3Transaction> transaction) const;
    };

    /**
     * @brief Memory pool for caching verified transactions before being written into blocks.
     * This matches the C# MemoryPool.cs functionality exactly.
     */
    class MemoryPool
    {
    public:
        // Event handlers
        using TransactionAddedHandler = std::function<void(std::shared_ptr<Neo3Transaction>)>;
        using TransactionRemovedHandler = std::function<void(const TransactionRemovedEventArgs&)>;

        /**
         * @brief Constructs a memory pool.
         * @param system The Neo system.
         */
        explicit MemoryPool(std::shared_ptr<NeoSystem> system);

        /**
         * @brief Destructor.
         */
        ~MemoryPool();

        // Disable copy and move
        MemoryPool(const MemoryPool&) = delete;
        MemoryPool& operator=(const MemoryPool&) = delete;
        MemoryPool(MemoryPool&&) = delete;
        MemoryPool& operator=(MemoryPool&&) = delete;

        /**
         * @brief Gets the capacity of the memory pool.
         */
        int GetCapacity() const { return capacity_; }

        /**
         * @brief Gets the total count of transactions in the pool.
         */
        int GetCount() const;

        /**
         * @brief Gets the count of verified transactions.
         */
        int GetVerifiedCount() const { return static_cast<int>(unsorted_transactions_.size()); }

        /**
         * @brief Gets the count of unverified transactions.
         */
        int GetUnverifiedCount() const { return static_cast<int>(unverified_transactions_.size()); }

        /**
         * @brief Checks if the pool contains a transaction with the specified hash.
         * @param hash The transaction hash.
         * @return True if the pool contains the transaction, false otherwise.
         */
        bool ContainsKey(const io::UInt256& hash) const;

        /**
         * @brief Tries to get a transaction by hash.
         * @param hash The transaction hash.
         * @param tx Output parameter for the transaction.
         * @return True if the transaction was found, false otherwise.
         */
        bool TryGetValue(const io::UInt256& hash, std::shared_ptr<Neo3Transaction>& tx) const;

        /**
         * @brief Gets all verified transactions.
         * @return Vector of verified transactions.
         */
        std::vector<std::shared_ptr<Neo3Transaction>> GetVerifiedTransactions() const;

        /**
         * @brief Gets all transactions (alias for GetVerifiedTransactions).
         * @return Vector of verified transactions.
         */
        std::vector<std::shared_ptr<Neo3Transaction>> GetTransactions() const { return GetVerifiedTransactions(); }

        /**
         * @brief Gets a transaction by hash from the pool.
         * @param hash The transaction hash.
         * @return The transaction if found, nullptr otherwise.
         */
        std::shared_ptr<Neo3Transaction> GetTransaction(const io::UInt256& hash) const;

        /**
         * @brief Adds a transaction to the pool (alias for TryAdd).
         * @param tx The transaction to add.
         * @return The verification result.
         */
        VerifyResult AddTransaction(std::shared_ptr<Neo3Transaction> tx);

        /**
         * @brief Gets sorted verified transactions.
         * @param count Maximum number of transactions to return (-1 for all).
         * @return Vector of sorted verified transactions.
         */
        std::vector<std::shared_ptr<Neo3Transaction>> GetSortedVerifiedTransactions(int count = -1) const;

        /**
         * @brief Gets both verified and unverified transactions.
         * @param verified_transactions Output parameter for verified transactions.
         * @param unverified_transactions Output parameter for unverified transactions.
         */
        void GetVerifiedAndUnverifiedTransactions(
            std::vector<std::shared_ptr<Neo3Transaction>>& verified_transactions,
            std::vector<std::shared_ptr<Neo3Transaction>>& unverified_transactions) const;

        /**
         * @brief Checks if a transaction can fit in the pool.
         * @param tx The transaction to check.
         * @return True if the transaction can fit, false otherwise.
         */
        bool CanTransactionFitInPool(std::shared_ptr<Neo3Transaction> tx) const;

        /**
         * @brief Tries to add a transaction to the pool.
         * @param tx The transaction to add.
         * @param snapshot The data cache snapshot.
         * @return The verification result.
         */
        VerifyResult TryAdd(std::shared_ptr<Neo3Transaction> tx, std::shared_ptr<persistence::DataCache> snapshot);

        /**
         * @brief Updates the pool after a block is persisted.
         * @param block The persisted block.
         * @param snapshot The data cache snapshot.
         */
        void UpdatePoolForBlockPersisted(std::shared_ptr<network::p2p::payloads::Block> block, 
                                        std::shared_ptr<persistence::DataCache> snapshot);

        /**
         * @brief Re-verifies unverified transactions.
         * @param count Maximum number of transactions to reverify.
         * @param milliseconds_timeout Timeout for reverification.
         * @param snapshot The data cache snapshot.
         */
        void ReVerifyTopUnverifiedTransactionsIfNeeded(int count, double milliseconds_timeout, 
                                                      std::shared_ptr<persistence::DataCache> snapshot);

        /**
         * @brief Re-verifies top unverified transactions if needed (simpler interface).
         * @param count Maximum number of transactions to reverify.
         * @param snapshot The data cache snapshot.
         * @return True if more transactions need reverification.
         */
        bool ReVerifyTopUnverifiedTransactionsIfNeeded(int count, 
                                                      std::shared_ptr<persistence::DataCache> snapshot);

        /**
         * @brief Tries to remove an unverified transaction.
         * @param hash The transaction hash.
         * @return True if the transaction was removed, false otherwise.
         */
        bool TryRemoveUnverified(const io::UInt256& hash);

        /**
         * @brief Invalidates all verified transactions (moves them to unverified).
         */
        void InvalidateVerifiedTransactions();

        /**
         * @brief Invalidates all transactions in the pool.
         */
        void InvalidateAllTransactions();

        // Event registration
        void RegisterTransactionAddedHandler(TransactionAddedHandler handler);
        void RegisterTransactionRemovedHandler(TransactionRemovedHandler handler);

    private:
        // Core processing methods
        std::shared_ptr<PoolItem> GetLowestFeeTransaction() const;
        std::shared_ptr<PoolItem> GetLowestFeeTransaction(
            std::unordered_map<io::UInt256, std::shared_ptr<PoolItem>>*& unsorted_pool,
            std::set<std::shared_ptr<PoolItem>>*& sorted_pool) const;
        
        bool CheckConflicts(std::shared_ptr<Neo3Transaction> tx, 
                          std::vector<std::shared_ptr<PoolItem>>& conflicts_to_remove) const;
        
        std::vector<std::shared_ptr<Neo3Transaction>> RemoveOverCapacity();
        
        bool TryRemoveVerified(const io::UInt256& hash, std::shared_ptr<PoolItem>& item);
        bool TryRemoveUnverified(const io::UInt256& hash, std::shared_ptr<PoolItem>& item);
        
        void RemoveConflictsOfVerified(std::shared_ptr<PoolItem> item);

        // Event firing
        void FireTransactionAdded(std::shared_ptr<Neo3Transaction> tx);
        void FireTransactionRemoved(const TransactionRemovedEventArgs& args);

        // Member variables
        std::shared_ptr<NeoSystem> system_;
        int capacity_;
        
        // Timing configuration
        double max_milliseconds_to_reverify_tx_;
        double max_milliseconds_to_reverify_tx_per_idle_;
        
        // Thread synchronization
        mutable std::shared_mutex tx_rw_lock_;
        
        // Verified transactions
        std::unordered_map<io::UInt256, std::shared_ptr<PoolItem>> unsorted_transactions_;
        std::set<std::shared_ptr<PoolItem>> sorted_transactions_;
        
        // Unverified transactions  
        std::unordered_map<io::UInt256, std::shared_ptr<PoolItem>> unverified_transactions_;
        std::set<std::shared_ptr<PoolItem>> unverified_sorted_transactions_;
        
        // Conflicts tracking
        std::unordered_map<io::UInt256, std::unordered_set<io::UInt256>> conflicts_;
        
        // Verification context
        std::unique_ptr<TransactionVerificationContext> verification_context_;
        
        // Event handlers
        std::vector<TransactionAddedHandler> transaction_added_handlers_;
        std::vector<TransactionRemovedHandler> transaction_removed_handlers_;
        
        // Constants
        static constexpr int BLOCKS_TILL_REBROADCAST = 10;
    };

} // namespace neo::ledger 