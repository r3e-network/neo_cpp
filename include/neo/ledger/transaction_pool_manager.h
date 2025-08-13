/**
 * @file transaction_pool_manager.h
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/pool_item.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace neo::ledger
{
/**
 * @brief Advanced transaction pool manager with monitoring and optimization
 *
 * This class provides enterprise-grade transaction pool management with:
 * - Priority-based transaction ordering
 * - Real-time monitoring and metrics
 * - Automatic cleanup and optimization
 * - Transaction dependency tracking
 * - Fee-based prioritization
 * - Conflict detection
 */
class TransactionPoolManager
{
   public:
    /**
     * @brief Transaction priority levels
     */
    enum class Priority
    {
        Low = 0,
        Normal = 1,
        High = 2,
        Critical = 3
    };

    /**
     * @brief Pool statistics
     */
    struct PoolStats
    {
        size_t total_transactions;
        size_t verified_count;
        size_t unverified_count;
        size_t pending_count;
        size_t rejected_count;
        uint64_t total_fees;
        double average_fee;
        std::chrono::milliseconds average_validation_time;
        size_t memory_usage_bytes;
        double throughput_tps;  // Transactions per second
    };

    /**
     * @brief Transaction metadata for tracking
     */
    struct TransactionMetadata
    {
        io::UInt256 hash;
        Priority priority;
        uint64_t fee;
        std::chrono::steady_clock::time_point received_time;
        std::chrono::steady_clock::time_point validated_time;
        std::vector<io::UInt256> dependencies;  // Other transactions this depends on
        bool is_verified;
        uint32_t retry_count;
        std::string source_peer;  // Network peer that sent this transaction
    };

    /**
     * @brief Configuration for the pool manager
     */
    struct Configuration
    {
        size_t max_pool_size = 100000;
        size_t max_unverified_size = 10000;
        size_t max_transaction_size = 102400;           // 100KB
        std::chrono::seconds transaction_timeout{300};  // 5 minutes
        std::chrono::seconds cleanup_interval{60};      // 1 minute
        uint64_t min_fee_threshold = 0;
        bool enable_priority_queue = true;
        bool enable_conflict_detection = true;
        bool enable_metrics = true;
        uint32_t max_retry_attempts = 3;
    };

   private:
    // Core memory pool
    std::unique_ptr<MemoryPool> memory_pool_;

    // Configuration
    Configuration config_;

    // Transaction metadata tracking
    std::unordered_map<io::UInt256, TransactionMetadata> metadata_;

    // Priority queue for transaction ordering
    struct TransactionComparator
    {
        bool operator()(const TransactionMetadata& a, const TransactionMetadata& b) const
        {
            // Higher priority first, then higher fee
            if (a.priority != b.priority) return static_cast<int>(a.priority) < static_cast<int>(b.priority);
            return a.fee < b.fee;
        }
    };
    std::priority_queue<TransactionMetadata, std::vector<TransactionMetadata>, TransactionComparator> priority_queue_;

    // Conflict detection
    std::unordered_map<std::string, std::vector<io::UInt256>> conflict_groups_;  // Track conflicting transactions

    // Metrics and monitoring
    std::atomic<size_t> total_received_{0};
    std::atomic<size_t> total_validated_{0};
    std::atomic<size_t> total_rejected_{0};
    std::atomic<uint64_t> total_fees_{0};
    std::chrono::steady_clock::time_point start_time_;

    // Thread management
    std::thread cleanup_thread_;
    std::atomic<bool> running_{false};
    mutable std::shared_mutex mutex_;

    // Callbacks
    std::function<bool(const network::p2p::payloads::Neo3Transaction&)> validator_;
    std::function<void(const io::UInt256&, const std::string&)> on_transaction_added_;
    std::function<void(const io::UInt256&, const std::string&)> on_transaction_removed_;
    std::function<void(const PoolStats&)> on_stats_updated_;

   public:
    /**
     * @brief Constructor with configuration
     * @param config Pool manager configuration
     */
    explicit TransactionPoolManager(const Configuration& config);

    /**
     * @brief Default constructor with default configuration
     */
    TransactionPoolManager();

    /**
     * @brief Destructor - ensures cleanup thread is stopped
     */
    ~TransactionPoolManager();

    /**
     * @brief Start the pool manager and background tasks
     */
    void Start();

    /**
     * @brief Stop the pool manager and background tasks
     */
    void Stop();

    /**
     * @brief Add a transaction to the pool
     * @param transaction The transaction to add
     * @param priority Transaction priority
     * @param source_peer Source network peer
     * @return True if successfully added, false otherwise
     */
    bool AddTransaction(const network::p2p::payloads::Neo3Transaction& transaction,
                        Priority priority = Priority::Normal, const std::string& source_peer = "");

    /**
     * @brief Remove a transaction from the pool
     * @param hash Transaction hash
     * @param reason Removal reason for logging
     * @return True if removed, false if not found
     */
    bool RemoveTransaction(const io::UInt256& hash, const std::string& reason = "");

    /**
     * @brief Get a transaction by hash
     * @param hash Transaction hash
     * @return Optional containing the transaction if found
     */
    std::optional<network::p2p::payloads::Neo3Transaction> GetTransaction(const io::UInt256& hash) const;

    /**
     * @brief Get transactions ready for inclusion in a block
     * @param max_count Maximum number of transactions to return
     * @param max_size Maximum total size in bytes
     * @return Vector of transactions ordered by priority and fee
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetTransactionsForBlock(size_t max_count = 1000,
                                                                                 size_t max_size = 1024 * 1024) const;

    /**
     * @brief Check if a transaction exists in the pool
     * @param hash Transaction hash
     * @return True if transaction exists
     */
    bool ContainsTransaction(const io::UInt256& hash) const;

    /**
     * @brief Get current pool statistics
     * @return Pool statistics snapshot
     */
    PoolStats GetStatistics() const;

    /**
     * @brief Clear all transactions from the pool
     * @param reason Reason for clearing
     */
    void Clear(const std::string& reason = "Manual clear");

    /**
     * @brief Validate all unverified transactions
     * @return Number of transactions validated
     */
    size_t ValidateUnverifiedTransactions();

    /**
     * @brief Remove expired transactions
     * @return Number of transactions removed
     */
    size_t RemoveExpiredTransactions();

    /**
     * @brief Detect and handle conflicting transactions
     * @return Number of conflicts resolved
     */
    size_t DetectAndResolveConflicts();

    /**
     * @brief Get transaction metadata
     * @param hash Transaction hash
     * @return Optional containing metadata if found
     */
    std::optional<TransactionMetadata> GetTransactionMetadata(const io::UInt256& hash) const;

    /**
     * @brief Set transaction validator callback
     * @param validator Validation function
     */
    void SetValidator(std::function<bool(const network::p2p::payloads::Neo3Transaction&)> validator);

    /**
     * @brief Set transaction added callback
     * @param callback Callback function
     */
    void SetOnTransactionAdded(std::function<void(const io::UInt256&, const std::string&)> callback);

    /**
     * @brief Set transaction removed callback
     * @param callback Callback function
     */
    void SetOnTransactionRemoved(std::function<void(const io::UInt256&, const std::string&)> callback);

    /**
     * @brief Set statistics updated callback
     * @param callback Callback function
     */
    void SetOnStatsUpdated(std::function<void(const PoolStats&)> callback);

    /**
     * @brief Get pool configuration
     * @return Current configuration
     */
    const Configuration& GetConfiguration() const { return config_; }

    /**
     * @brief Update pool configuration
     * @param config New configuration
     */
    void UpdateConfiguration(const Configuration& config);

   private:
    /**
     * @brief Background cleanup thread function
     */
    void CleanupThread();

    /**
     * @brief Calculate transaction priority based on fee and other factors
     * @param transaction The transaction
     * @return Calculated priority
     */
    Priority CalculatePriority(const network::p2p::payloads::Neo3Transaction& transaction) const;

    /**
     * @brief Check if transaction conflicts with existing ones
     * @param transaction The transaction to check
     * @return Vector of conflicting transaction hashes
     */
    std::vector<io::UInt256> CheckConflicts(const network::p2p::payloads::Neo3Transaction& transaction) const;

    /**
     * @brief Update internal metrics
     */
    void UpdateMetrics();

    /**
     * @brief Calculate current memory usage
     * @return Memory usage in bytes
     */
    size_t CalculateMemoryUsage() const;

    /**
     * @brief Check if transaction is in unverified pool (C# compatibility)
     * @param hash Transaction hash
     * @return True if in unverified pool
     */
    bool IsInUnverifiedPool(const io::UInt256& hash) const;
};

}  // namespace neo::ledger