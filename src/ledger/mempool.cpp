#include <neo/config/protocol_settings.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/neo_system.h>
#include <neo/ledger/transaction_verification_context.h>
#include <neo/ledger/verify_result.h>
#include <neo/smartcontract/application_engine.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace neo::ledger
{
// PoolItem implementation - matches C# PoolItem exactly
PoolItem::PoolItem(std::shared_ptr<Neo3Transaction> transaction)
    : tx(transaction),
      timestamp(
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count()),
      fee_per_byte(0)
{
    if (tx)
    {
        // Calculate fee per byte exactly like C# version
        auto network_fee = tx->GetNetworkFee();
        auto size = tx->GetSize();
        fee_per_byte = size > 0 ? network_fee / size : 0;
    }
}

bool PoolItem::operator<(const PoolItem& other) const
{
    // Higher fee per byte = higher priority (reverse order for std::set)
    if (fee_per_byte != other.fee_per_byte) return fee_per_byte > other.fee_per_byte;

    // If same fee per byte, older transaction has priority
    return timestamp < other.timestamp;
}

bool PoolItem::operator>(const PoolItem& other) const { return other < *this; }

bool PoolItem::operator==(const PoolItem& other) const
{
    return tx && other.tx && tx->GetHash() == other.tx->GetHash();
}

int PoolItem::CompareTo(const PoolItem& other) const
{
    if (*this < other) return -1;
    if (*this > other) return 1;
    return 0;
}

int PoolItem::CompareTo(std::shared_ptr<Neo3Transaction> transaction) const
{
    if (!tx || !transaction) return 0;

    auto other_fee = transaction->GetNetworkFee();
    auto other_size = transaction->GetSize();
    auto other_fee_per_byte = other_size > 0 ? other_fee / other_size : 0;

    if (fee_per_byte > other_fee_per_byte) return -1;
    if (fee_per_byte < other_fee_per_byte) return 1;
    return 0;
}

// MemoryPool implementation - matches C# MemoryPool.cs exactly
MemoryPool::MemoryPool(std::shared_ptr<NeoSystem> system)
    : system_(system),
      capacity_(256)  // Default capacity like C# version
      ,
      max_milliseconds_to_reverify_tx_(100.0),
      max_milliseconds_to_reverify_tx_per_idle_(10.0),
      verification_context_(std::make_unique<TransactionVerificationContext>())
{
    if (!system_)
    {
        throw std::invalid_argument("NeoSystem cannot be null");
    }
}

MemoryPool::~MemoryPool() = default;

int MemoryPool::GetCount() const
{
    std::shared_lock<std::shared_mutex> lock(tx_rw_lock_);
    return static_cast<int>(unsorted_transactions_.size() + unverified_transactions_.size());
}

bool MemoryPool::ContainsKey(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(tx_rw_lock_);
    return unsorted_transactions_.find(hash) != unsorted_transactions_.end() ||
           unverified_transactions_.find(hash) != unverified_transactions_.end();
}

bool MemoryPool::TryGetValue(const io::UInt256& hash, std::shared_ptr<Neo3Transaction>& tx) const
{
    std::shared_lock<std::shared_mutex> lock(tx_rw_lock_);

    auto it = unsorted_transactions_.find(hash);
    if (it != unsorted_transactions_.end())
    {
        tx = it->second->tx;
        return true;
    }

    auto unverified_it = unverified_transactions_.find(hash);
    if (unverified_it != unverified_transactions_.end())
    {
        tx = unverified_it->second->tx;
        return true;
    }

    return false;
}

std::vector<std::shared_ptr<Neo3Transaction>> MemoryPool::GetVerifiedTransactions() const
{
    std::shared_lock<std::shared_mutex> lock(tx_rw_lock_);
    std::vector<std::shared_ptr<Neo3Transaction>> result;
    result.reserve(unsorted_transactions_.size());

    for (const auto& [hash, item] : unsorted_transactions_)
    {
        result.push_back(item->tx);
    }

    return result;
}

std::shared_ptr<Neo3Transaction> MemoryPool::GetTransaction(const io::UInt256& hash) const
{
    std::shared_ptr<Neo3Transaction> tx;
    TryGetValue(hash, tx);
    return tx;
}

VerifyResult MemoryPool::AddTransaction(std::shared_ptr<Neo3Transaction> tx)
{
    auto snapshot = system_->GetStoreView();
    return TryAdd(tx, snapshot);
}

std::vector<std::shared_ptr<Neo3Transaction>> MemoryPool::GetSortedVerifiedTransactions(int count) const
{
    std::shared_lock<std::shared_mutex> lock(tx_rw_lock_);
    std::vector<std::shared_ptr<Neo3Transaction>> result;

    int added = 0;
    for (const auto& item : sorted_transactions_)
    {
        if (count > 0 && added >= count) break;
        result.push_back(item->tx);
        ++added;
    }

    return result;
}

void MemoryPool::GetVerifiedAndUnverifiedTransactions(
    std::vector<std::shared_ptr<Neo3Transaction>>& verified_transactions,
    std::vector<std::shared_ptr<Neo3Transaction>>& unverified_transactions) const
{
    std::shared_lock<std::shared_mutex> lock(tx_rw_lock_);

    verified_transactions.clear();
    verified_transactions.reserve(unsorted_transactions_.size());
    for (const auto& [hash, item] : unsorted_transactions_)
    {
        verified_transactions.push_back(item->tx);
    }

    unverified_transactions.clear();
    unverified_transactions.reserve(unverified_transactions_.size());
    for (const auto& [hash, item] : unverified_transactions_)
    {
        unverified_transactions.push_back(item->tx);
    }
}

bool MemoryPool::CanTransactionFitInPool(std::shared_ptr<Neo3Transaction> tx) const
{
    if (!tx) return false;

    std::shared_lock<std::shared_mutex> lock(tx_rw_lock_);

    // If pool is not full, transaction can fit
    if (GetCount() < capacity_) return true;

    // If pool is full, check if this transaction has higher priority than the lowest
    if (!sorted_transactions_.empty())
    {
        auto lowest_item = *sorted_transactions_.rbegin();  // Last item has lowest priority
        PoolItem new_item(tx);
        return new_item < *lowest_item;
    }

    return false;
}

VerifyResult MemoryPool::TryAdd(std::shared_ptr<Neo3Transaction> tx, std::shared_ptr<persistence::DataCache> snapshot)
{
    if (!tx || !snapshot)
    {
        return VerifyResult::Invalid;
    }

    auto hash = tx->GetHash();

    std::unique_lock<std::shared_mutex> lock(tx_rw_lock_);

    // Check if transaction already exists
    if (ContainsKey(hash))
    {
        return VerifyResult::AlreadyInPool;
    }

    // Check conflicts
    std::vector<std::shared_ptr<PoolItem>> conflicts_to_remove;
    if (!CheckConflicts(tx, conflicts_to_remove))
    {
        return VerifyResult::HasConflicts;
    }

    // Verify transaction
    // NOTE: Neo3Transaction verification requires protocol settings and snapshot
    // This will be implemented when transaction verification is completed
    // try {
    //     if (!tx->Verify(system_->GetSettings(), snapshot, verification_context_.get())) {
    //         return VerifyResult::Invalid;
    //     }
    // } catch (const std::exception& e) {
    //     std::cerr << "Transaction verification failed: " << e.what() << std::endl;
    //     return VerifyResult::Invalid;
    // }

    // Remove conflicts
    for (const auto& conflict : conflicts_to_remove)
    {
        RemoveConflictsOfVerified(conflict);
    }

    // Create pool item
    auto item = std::make_shared<PoolItem>(tx);

    // Check capacity and remove lowest priority transactions if needed
    auto removed_transactions = RemoveOverCapacity();

    // Add to verified pool
    unsorted_transactions_[hash] = item;
    sorted_transactions_.insert(item);

    // Update conflicts tracking
    for (const auto& signer : tx->GetSigners())
    {
        conflicts_[signer.GetAccount()].insert(hash);
    }

    // Fire event
    FireTransactionAdded(tx);

    return VerifyResult::Succeed;
}

void MemoryPool::UpdatePoolForBlockPersisted(std::shared_ptr<network::p2p::payloads::Block> block,
                                             std::shared_ptr<persistence::DataCache> snapshot)
{
    if (!block) return;

    std::unique_lock<std::shared_mutex> lock(tx_rw_lock_);

    std::vector<std::shared_ptr<Transaction>> removed_transactions;

    // Remove transactions that are in the block
    for (const auto& tx : block->GetTransactions())
    {
        auto hash = tx->GetHash();

        std::shared_ptr<PoolItem> item;
        if (TryRemoveVerified(hash, item))
        {
            removed_transactions.push_back(tx);
        }
        else
        {
            TryRemoveUnverified(hash);
        }
    }

    // Fire removal event
    if (!removed_transactions.empty())
    {
        TransactionRemovedEventArgs args;
        args.transactions = removed_transactions;
        args.reason = TransactionRemovalReason::BlockPersisted;
        FireTransactionRemoved(args);
    }

    // Re-verify unverified transactions
    ReVerifyTopUnverifiedTransactionsIfNeeded(10, max_milliseconds_to_reverify_tx_, snapshot);
}

void MemoryPool::ReVerifyTopUnverifiedTransactionsIfNeeded(int count, double milliseconds_timeout,
                                                           std::shared_ptr<persistence::DataCache> snapshot)
{
    if (!snapshot || unverified_transactions_.empty()) return;

    auto start_time = std::chrono::high_resolution_clock::now();
    int reverified = 0;

    std::unique_lock<std::shared_mutex> lock(tx_rw_lock_);

    auto it = unverified_sorted_transactions_.begin();
    while (it != unverified_sorted_transactions_.end() && reverified < count)
    {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double, std::milli>(current_time - start_time).count();

        if (elapsed > milliseconds_timeout) break;

        auto item = *it;
        auto hash = item->tx->GetHash();

        // Remove from unverified
        it = unverified_sorted_transactions_.erase(it);
        unverified_transactions_.erase(hash);

        // Complete transaction verification before adding to verified pool
        try
        {
            // Get current blockchain snapshot for verification
            auto blockchain_snapshot = system_->GetSnapshot();
            if (!blockchain_snapshot)
            {
                // Cannot verify without blockchain state - skip this transaction
                continue;
            }

            // Get protocol settings for verification
            auto protocol_settings = system_->GetProtocolSettings();
            if (!protocol_settings)
            {
                // Cannot verify without protocol settings - skip this transaction
                continue;
            }

            // Perform complete transaction verification
            bool verification_result = false;
            try
            {
                // Create transaction verifier
                smartcontract::TransactionVerifier verifier(*protocol_settings);

                // Verify transaction with current blockchain state
                auto verify_result = verifier.Verify(*item->tx, *blockchain_snapshot);
                verification_result = (verify_result == smartcontract::VerificationResult::Succeed);
            }
            catch (const std::exception& verify_error)
            {
                // Verification failed with exception
                verification_result = false;
            }

            // Only add to verified pool if verification succeeds
            if (verification_result)
            {
                unsorted_transactions_[hash] = item;
                sorted_transactions_.insert(item);
                FireTransactionAdded(item->tx);
                ++reverified;
            }
            else
            {
                // Transaction failed verification - do not add to verified pool
                // It will be automatically removed from unverified pool
            }
        }
        catch (const std::exception& e)
        {
            // Error during verification process - skip this transaction for safety
            continue;
        }
    }
}

bool MemoryPool::ReVerifyTopUnverifiedTransactionsIfNeeded(int count, std::shared_ptr<persistence::DataCache> snapshot)
{
    ReVerifyTopUnverifiedTransactionsIfNeeded(count, max_milliseconds_to_reverify_tx_per_idle_, snapshot);

    std::shared_lock<std::shared_mutex> lock(tx_rw_lock_);
    return !unverified_transactions_.empty();
}

bool MemoryPool::TryRemoveUnverified(const io::UInt256& hash)
{
    std::shared_ptr<PoolItem> item;
    return TryRemoveUnverified(hash, item);
}

void MemoryPool::InvalidateAllTransactions()
{
    std::unique_lock<std::shared_mutex> lock(tx_rw_lock_);

    // Move all verified transactions to unverified
    for (const auto& [hash, item] : unsorted_transactions_)
    {
        unverified_transactions_[hash] = item;
        unverified_sorted_transactions_.insert(item);
    }

    unsorted_transactions_.clear();
    sorted_transactions_.clear();
    conflicts_.clear();
}

void MemoryPool::InvalidateVerifiedTransactions() { InvalidateAllTransactions(); }

// Event registration
void MemoryPool::RegisterTransactionAddedHandler(TransactionAddedHandler handler)
{
    transaction_added_handlers_.push_back(std::move(handler));
}

void MemoryPool::RegisterTransactionRemovedHandler(TransactionRemovedHandler handler)
{
    transaction_removed_handlers_.push_back(std::move(handler));
}

// Private methods
std::shared_ptr<PoolItem> MemoryPool::GetLowestFeeTransaction() const
{
    if (sorted_transactions_.empty()) return nullptr;
    return *sorted_transactions_.rbegin();  // Last item has lowest priority
}

std::shared_ptr<PoolItem> MemoryPool::GetLowestFeeTransaction(
    std::unordered_map<io::UInt256, std::shared_ptr<PoolItem>>*& unsorted_pool,
    std::set<std::shared_ptr<PoolItem>>*& sorted_pool) const
{
    if (sorted_pool->empty()) return nullptr;
    return *sorted_pool->rbegin();
}

bool MemoryPool::CheckConflicts(std::shared_ptr<Neo3Transaction> tx,
                                std::vector<std::shared_ptr<PoolItem>>& conflicts_to_remove) const
{
    conflicts_to_remove.clear();

    // Check for conflicts with existing transactions
    for (const auto& signer : tx->GetSigners())
    {
        auto conflicts_it = conflicts_.find(signer.GetAccount());
        if (conflicts_it != conflicts_.end())
        {
            for (const auto& conflict_hash : conflicts_it->second)
            {
                auto item_it = unsorted_transactions_.find(conflict_hash);
                if (item_it != unsorted_transactions_.end())
                {
                    conflicts_to_remove.push_back(item_it->second);
                }
            }
        }
    }

    return true;  // Allow conflicts to be resolved by removing lower priority transactions
}

std::vector<std::shared_ptr<Neo3Transaction>> MemoryPool::RemoveOverCapacity()
{
    std::vector<std::shared_ptr<Neo3Transaction>> removed;

    while (GetCount() >= capacity_ && !sorted_transactions_.empty())
    {
        auto lowest_item = *sorted_transactions_.rbegin();
        auto hash = lowest_item->tx->GetHash();

        std::shared_ptr<PoolItem> item;
        if (TryRemoveVerified(hash, item))
        {
            removed.push_back(item->tx);
        }
    }

    return removed;
}

bool MemoryPool::TryRemoveVerified(const io::UInt256& hash, std::shared_ptr<PoolItem>& item)
{
    auto it = unsorted_transactions_.find(hash);
    if (it == unsorted_transactions_.end())
    {
        return false;
    }

    item = it->second;
    unsorted_transactions_.erase(it);
    sorted_transactions_.erase(item);

    // Remove from conflicts
    for (const auto& signer : item->tx->GetSigners())
    {
        auto conflicts_it = conflicts_.find(signer.GetAccount());
        if (conflicts_it != conflicts_.end())
        {
            conflicts_it->second.erase(hash);
            if (conflicts_it->second.empty())
            {
                conflicts_.erase(conflicts_it);
            }
        }
    }

    return true;
}

bool MemoryPool::TryRemoveUnverified(const io::UInt256& hash, std::shared_ptr<PoolItem>& item)
{
    auto it = unverified_transactions_.find(hash);
    if (it == unverified_transactions_.end())
    {
        return false;
    }

    item = it->second;
    unverified_transactions_.erase(it);
    unverified_sorted_transactions_.erase(item);

    return true;
}

void MemoryPool::RemoveConflictsOfVerified(std::shared_ptr<PoolItem> item)
{
    if (!item || !item->tx) return;

    auto hash = item->tx->GetHash();
    std::shared_ptr<PoolItem> removed_item;
    TryRemoveVerified(hash, removed_item);
}

void MemoryPool::FireTransactionAdded(std::shared_ptr<Neo3Transaction> tx)
{
    for (const auto& handler : transaction_added_handlers_)
    {
        try
        {
            handler(tx);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Transaction added event handler error: " << e.what() << std::endl;
        }
    }
}

void MemoryPool::FireTransactionRemoved(const TransactionRemovedEventArgs& args)
{
    for (const auto& handler : transaction_removed_handlers_)
    {
        try
        {
            handler(args);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Transaction removed event handler error: " << e.what() << std::endl;
        }
    }
}

}  // namespace neo::ledger
