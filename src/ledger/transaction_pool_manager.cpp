/**
 * @file transaction_pool_manager.cpp
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/ledger/transaction_pool_manager.h>
#include <neo/logging/logger.h>

#include <algorithm>
#include <sstream>

namespace neo::ledger
{

TransactionPoolManager::TransactionPoolManager() : TransactionPoolManager(Configuration{}) {}

TransactionPoolManager::TransactionPoolManager(const Configuration& config)
    : config_(config),
      memory_pool_(std::make_unique<MemoryPool>(config.max_pool_size, config.max_unverified_size)),
      start_time_(std::chrono::steady_clock::now())
{
    neo::logging::Logger::Instance().Info(
        "TxPool", "Transaction Pool Manager initialized with capacity: " + std::to_string(config.max_pool_size));
}

TransactionPoolManager::~TransactionPoolManager() { Stop(); }

void TransactionPoolManager::Start()
{
    if (running_.exchange(true))
    {
        neo::logging::Logger::Instance().Warning("TxPool", "Transaction Pool Manager already running");
        return;
    }

    neo::logging::Logger::Instance().Info("TxPool", "Starting Transaction Pool Manager");

    // Start cleanup thread
    cleanup_thread_ = std::thread(&TransactionPoolManager::CleanupThread, this);
}

void TransactionPoolManager::Stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    neo::logging::Logger::Instance().Info("TxPool", "Stopping Transaction Pool Manager");

    // Stop cleanup thread
    if (cleanup_thread_.joinable())
    {
        cleanup_thread_.join();
    }

    // Clear all transactions
    Clear("Shutdown");
}

bool TransactionPoolManager::AddTransaction(const network::p2p::payloads::Neo3Transaction& transaction,
                                            Priority priority, const std::string& source_peer)
{
    std::unique_lock lock(mutex_);

    auto hash = transaction.GetHash();

    // Check if transaction already exists (C# pattern: ContainsKey check)
    if (memory_pool_->Contains(hash))
    {
        neo::logging::Logger::Instance().Debug("TxPool", "Transaction already in pool: " + hash.ToString());
        return false;
    }

    // Delegate to MemoryPool's TryAdd for C# consistency
    // The MemoryPool handles sorted/unsorted/unverified pools internally
    if (!memory_pool_->TryAdd(transaction))
    {
        total_rejected_.fetch_add(1);
        neo::logging::Logger::Instance().Warning("TxPool", "MemoryPool rejected transaction: " + hash.ToString());
        return false;
    }

    // Track metadata for monitoring (additional layer on top of MemoryPool)
    TransactionMetadata metadata;
    metadata.hash = hash;
    metadata.priority = priority;
    metadata.fee = transaction.GetNetworkFee();
    metadata.received_time = std::chrono::steady_clock::now();
    metadata.is_verified = false;
    metadata.retry_count = 0;
    metadata.source_peer = source_peer;

    // Neo3 doesn't have inputs - transactions use signers instead
    // Dependencies would be tracked through conflict attributes in Neo3

    // Add to metadata tracking
    metadata_[hash] = metadata;

    // Add to priority queue if enabled (additional optimization layer)
    if (config_.enable_priority_queue)
    {
        priority_queue_.push(metadata);
    }

    // Update metrics
    total_received_.fetch_add(1);
    total_fees_.fetch_add(transaction.GetNetworkFee());

    // Trigger callback (C# pattern: event notification)
    if (on_transaction_added_)
    {
        on_transaction_added_(hash, source_peer);
    }

    neo::logging::Logger::Instance().Info("TxPool", "Added transaction: " + hash.ToString() +
                                                        " (Priority: " + std::to_string(static_cast<int>(priority)) +
                                                        ", Fee: " + std::to_string(transaction.GetNetworkFee()) + ")");

    return true;
}

bool TransactionPoolManager::RemoveTransaction(const io::UInt256& hash, const std::string& reason)
{
    std::unique_lock lock(mutex_);

    auto it = metadata_.find(hash);
    if (it == metadata_.end())
    {
        return false;
    }

    // Remove from metadata
    metadata_.erase(it);

    // Remove from memory pool
    memory_pool_->Remove(hash);

    // Trigger callback
    if (on_transaction_removed_)
    {
        on_transaction_removed_(hash, reason);
    }

    neo::logging::Logger::Instance().Info("TxPool",
                                          "Removed transaction: " + hash.ToString() + " (Reason: " + reason + ")");

    return true;
}

std::optional<network::p2p::payloads::Neo3Transaction> TransactionPoolManager::GetTransaction(
    const io::UInt256& hash) const
{
    std::shared_lock lock(mutex_);

    // C# pattern: Use MemoryPool's GetTransaction method
    auto tx_ptr = memory_pool_->GetTransaction(hash);
    if (tx_ptr != nullptr)
    {
        return *tx_ptr;
    }

    return std::nullopt;
}

std::vector<network::p2p::payloads::Neo3Transaction> TransactionPoolManager::GetTransactionsForBlock(
    size_t max_count, size_t max_size) const
{
    std::shared_lock lock(mutex_);

    // C# pattern: Delegate to MemoryPool's GetTransactionsForBlock
    // The MemoryPool already handles sorted transactions by fee per byte
    auto transactions = memory_pool_->GetTransactionsForBlock(max_count);

    // Additional size filtering if needed
    std::vector<network::p2p::payloads::Neo3Transaction> result;
    size_t total_size = 0;

    for (const auto& tx : transactions)
    {
        auto tx_size = tx.GetSize();
        if (total_size + tx_size > max_size) break;

        // Check dependencies are satisfied
        auto it = metadata_.find(tx.GetHash());
        if (it != metadata_.end())
        {
            bool deps_satisfied = true;
            for (const auto& dep : it->second.dependencies)
            {
                // Check if dependency is already in result
                bool found = false;
                for (const auto& included : result)
                {
                    if (included.GetHash() == dep)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    deps_satisfied = false;
                    break;
                }
            }

            if (!deps_satisfied) continue;
        }

        result.push_back(tx);
        total_size += tx_size;
    }

    return result;
}

bool TransactionPoolManager::ContainsTransaction(const io::UInt256& hash) const
{
    std::shared_lock lock(mutex_);
    // C# pattern: Delegate to MemoryPool's Contains method
    return memory_pool_->Contains(hash);
}

TransactionPoolManager::PoolStats TransactionPoolManager::GetStatistics() const
{
    std::shared_lock lock(mutex_);

    // C# pattern: Get statistics from MemoryPool first
    auto mempool_stats = memory_pool_->GetStatistics();

    PoolStats stats;
    stats.total_transactions = mempool_stats.verified_transaction_count + mempool_stats.unverified_transaction_count;
    stats.verified_count = mempool_stats.verified_transaction_count;
    stats.unverified_count = mempool_stats.unverified_transaction_count;
    stats.pending_count = stats.unverified_count;
    stats.rejected_count = total_rejected_.load();
    stats.total_fees = total_fees_.load();
    stats.average_fee = mempool_stats.average_fee_per_byte;  // C# uses fee per byte

    // Calculate average validation time
    std::chrono::milliseconds total_validation_time{0};
    size_t validated_count = 0;
    for (const auto& [hash, meta] : metadata_)
    {
        if (meta.is_verified)
        {
            auto validation_time =
                std::chrono::duration_cast<std::chrono::milliseconds>(meta.validated_time - meta.received_time);
            total_validation_time += validation_time;
            validated_count++;
        }
    }
    stats.average_validation_time = validated_count > 0
                                        ? std::chrono::milliseconds(total_validation_time.count() / validated_count)
                                        : std::chrono::milliseconds{0};

    // Use MemoryPool's calculated memory usage
    stats.memory_usage_bytes = mempool_stats.total_size_bytes;

    // Calculate throughput
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    stats.throughput_tps = elapsed_seconds > 0 ? static_cast<double>(total_received_.load()) / elapsed_seconds : 0.0;

    return stats;
}

void TransactionPoolManager::Clear(const std::string& reason)
{
    std::unique_lock lock(mutex_);

    neo::logging::Logger::Instance().Info("TxPool", "Clearing pool: " + reason);

    // Clear all data structures
    metadata_.clear();
    while (!priority_queue_.empty())
    {
        priority_queue_.pop();
    }
    conflict_groups_.clear();

    // Clear memory pool
    memory_pool_->Clear();

    // Reset metrics
    total_received_ = 0;
    total_validated_ = 0;
    total_rejected_ = 0;
    total_fees_ = 0;
    start_time_ = std::chrono::steady_clock::now();
}

size_t TransactionPoolManager::ValidateUnverifiedTransactions()
{
    if (!validator_)
    {
        return 0;
    }

    std::unique_lock lock(mutex_);

    // C# pattern: Use MemoryPool's ReverifyTransactions method
    // This follows the C# Neo pattern of reverifying unverified transactions
    size_t max_to_reverify = std::min(static_cast<size_t>(10), memory_pool_->GetUnverifiedSize());
    memory_pool_->ReverifyTransactions(max_to_reverify);

    // Update metadata for verified transactions
    size_t validated = 0;
    auto unverified = memory_pool_->GetUnverifiedTransactions();

    for (const auto& tx : unverified)
    {
        auto hash = tx.GetHash();
        auto it = metadata_.find(hash);
        if (it == metadata_.end()) continue;

        // Check if now verified (moved to verified pool)
        if (memory_pool_->Contains(hash) && !IsInUnverifiedPool(hash))
        {
            it->second.is_verified = true;
            it->second.validated_time = std::chrono::steady_clock::now();
            validated++;
            total_validated_.fetch_add(1);

            neo::logging::Logger::Instance().Debug("TxPool", "Validated transaction: " + hash.ToString());
        }
    }

    return validated;
}

size_t TransactionPoolManager::RemoveExpiredTransactions()
{
    std::unique_lock lock(mutex_);

    size_t removed = 0;
    auto now = std::chrono::steady_clock::now();

    std::vector<io::UInt256> to_remove;
    for (const auto& [hash, meta] : metadata_)
    {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - meta.received_time);
        if (age > config_.transaction_timeout)
        {
            to_remove.push_back(hash);
        }
    }

    for (const auto& hash : to_remove)
    {
        RemoveTransaction(hash, "Expired");
        removed++;
    }

    if (removed > 0)
    {
        neo::logging::Logger::Instance().Info("TxPool", "Removed " + std::to_string(removed) + " expired transactions");
    }

    return removed;
}

size_t TransactionPoolManager::DetectAndResolveConflicts()
{
    if (!config_.enable_conflict_detection)
    {
        return 0;
    }

    std::unique_lock lock(mutex_);

    size_t conflicts_resolved = 0;

    // Group transactions by conflicting inputs
    conflict_groups_.clear();
    for (const auto& [hash, meta] : metadata_)
    {
        // TODO: Implement Get method in MemoryPool
        // auto tx_opt = memory_pool_->Get(hash);
        // if (!tx_opt.has_value())
        //     continue;

        // TODO: Complete conflict detection once Get method is available
        // auto& tx = tx_opt.value().GetTransaction();
        // for (const auto& input : tx.GetInputs())
        // {
        //     std::string key = input.GetPrevHash().ToString() + ":" + std::to_string(input.GetPrevIndex());
        //     conflict_groups_[key].push_back(hash);
        // }
    }

    // Resolve conflicts by keeping highest fee transaction
    for (const auto& [key, group] : conflict_groups_)
    {
        if (group.size() <= 1) continue;

        // Find transaction with highest fee
        io::UInt256 best_hash;
        uint64_t best_fee = 0;

        for (const auto& hash : group)
        {
            auto it = metadata_.find(hash);
            if (it != metadata_.end() && it->second.fee > best_fee)
            {
                best_fee = it->second.fee;
                best_hash = hash;
            }
        }

        // Remove all except the best
        for (const auto& hash : group)
        {
            if (hash != best_hash)
            {
                RemoveTransaction(hash, "Conflict resolved - lower fee");
                conflicts_resolved++;
            }
        }
    }

    if (conflicts_resolved > 0)
    {
        neo::logging::Logger::Instance().Info(
            "TxPool", "Resolved " + std::to_string(conflicts_resolved) + " transaction conflicts");
    }

    return conflicts_resolved;
}

std::optional<TransactionPoolManager::TransactionMetadata> TransactionPoolManager::GetTransactionMetadata(
    const io::UInt256& hash) const
{
    std::shared_lock lock(mutex_);

    auto it = metadata_.find(hash);
    if (it != metadata_.end())
    {
        return it->second;
    }

    return std::nullopt;
}

void TransactionPoolManager::SetValidator(std::function<bool(const network::p2p::payloads::Neo3Transaction&)> validator)
{
    validator_ = validator;
    memory_pool_->SetVerifier(validator);
}

void TransactionPoolManager::SetOnTransactionAdded(std::function<void(const io::UInt256&, const std::string&)> callback)
{
    on_transaction_added_ = callback;
}

void TransactionPoolManager::SetOnTransactionRemoved(
    std::function<void(const io::UInt256&, const std::string&)> callback)
{
    on_transaction_removed_ = callback;
}

void TransactionPoolManager::SetOnStatsUpdated(std::function<void(const PoolStats&)> callback)
{
    on_stats_updated_ = callback;
}

void TransactionPoolManager::UpdateConfiguration(const Configuration& config)
{
    std::unique_lock lock(mutex_);
    config_ = config;

    neo::logging::Logger::Instance().Info("TxPool", "Configuration updated");
}

void TransactionPoolManager::CleanupThread()
{
    neo::logging::Logger::Instance().Info("TxPool", "Cleanup thread started");

    while (running_)
    {
        // Sleep for cleanup interval
        std::this_thread::sleep_for(config_.cleanup_interval);

        if (!running_) break;

        // Perform cleanup tasks
        size_t expired = RemoveExpiredTransactions();
        size_t validated = ValidateUnverifiedTransactions();
        size_t conflicts = DetectAndResolveConflicts();

        // Update metrics
        UpdateMetrics();

        // Trigger stats callback
        if (on_stats_updated_)
        {
            on_stats_updated_(GetStatistics());
        }

        neo::logging::Logger::Instance().Debug(
            "TxPool", "Cleanup cycle completed - Expired: " + std::to_string(expired) +
                          ", Validated: " + std::to_string(validated) + ", Conflicts: " + std::to_string(conflicts));
    }

    neo::logging::Logger::Instance().Info("TxPool", "Cleanup thread stopped");
}

TransactionPoolManager::Priority TransactionPoolManager::CalculatePriority(
    const network::p2p::payloads::Neo3Transaction& transaction) const
{
    auto fee = transaction.GetNetworkFee();

    // Simple fee-based priority calculation
    if (fee >= 1000000000)  // >= 10 GAS
        return Priority::Critical;
    else if (fee >= 100000000)  // >= 1 GAS
        return Priority::High;
    else if (fee >= 10000000)  // >= 0.1 GAS
        return Priority::Normal;
    else
        return Priority::Low;
}

std::vector<io::UInt256> TransactionPoolManager::CheckConflicts(
    const network::p2p::payloads::Neo3Transaction& transaction) const
{
    std::vector<io::UInt256> conflicts;

    // Neo3 uses conflict attributes instead of checking inputs
    // Check for conflicts through the Conflicts attribute
    for (const auto& attr : transaction.GetAttributes())
    {
        if (attr->GetType() == ledger::TransactionAttribute::Usage::Conflicts)
        {
            // The Conflicts attribute contains a hash that this transaction conflicts with
            // This is how Neo3 handles transaction conflicts
            // For now, we'll skip this check as it requires the Conflicts attribute implementation
        }
    }

    return conflicts;
}

void TransactionPoolManager::UpdateMetrics()
{
    // This method can be extended to update more sophisticated metrics
    // For now, basic metrics are updated inline during operations
}

size_t TransactionPoolManager::CalculateMemoryUsage() const
{
    // Estimate memory usage
    size_t usage = sizeof(TransactionPoolManager);
    usage += metadata_.size() * (sizeof(io::UInt256) + sizeof(TransactionMetadata));
    usage += conflict_groups_.size() * 100;   // Rough estimate
    usage += memory_pool_->GetSize() * 1024;  // Assume ~1KB per transaction

    return usage;
}

bool TransactionPoolManager::IsInUnverifiedPool(const io::UInt256& hash) const
{
    // C# pattern: Check if transaction is in unverified pool
    auto unverified = memory_pool_->GetUnverifiedTransactions();
    for (const auto& tx : unverified)
    {
        if (tx.GetHash() == hash)
        {
            return true;
        }
    }
    return false;
}

}  // namespace neo::ledger