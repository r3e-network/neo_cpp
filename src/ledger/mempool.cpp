#include <neo/ledger/mempool.h>
#include <algorithm>
#include <stdexcept>

namespace neo::ledger
{
    // PoolItem implementation
    PoolItem::PoolItem(std::shared_ptr<Transaction> tx)
        : tx_(tx), time_(std::chrono::system_clock::now())
    {
    }

    std::shared_ptr<Transaction> PoolItem::GetTransaction() const
    {
        return tx_;
    }

    std::chrono::system_clock::time_point PoolItem::GetTime() const
    {
        return time_;
    }

    io::UInt256 PoolItem::GetHash() const
    {
        return tx_->GetHash();
    }

    // MemoryPool implementation
    MemoryPool::MemoryPool(std::shared_ptr<Blockchain> blockchain, size_t capacity)
        : blockchain_(blockchain), capacity_(capacity)
    {
    }

    size_t MemoryPool::Count() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return transactions_.size();
    }

    size_t MemoryPool::GetCapacity() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return capacity_;
    }

    void MemoryPool::SetCapacity(size_t capacity)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        capacity_ = capacity;
        RemoveOldTransactions();
    }

    std::vector<std::shared_ptr<Transaction>> MemoryPool::GetTransactions() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<Transaction>> result;
        result.reserve(transactions_.size());
        for (const auto& [hash, item] : transactions_)
        {
            result.push_back(item.GetTransaction());
        }
        return result;
    }

    std::shared_ptr<Transaction> MemoryPool::GetTransaction(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = transactions_.find(hash);
        if (it == transactions_.end())
            return nullptr;
        return it->second.GetTransaction();
    }

    bool MemoryPool::AddTransaction(std::shared_ptr<Transaction> tx)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the transaction is already in the pool
        io::UInt256 hash = tx->GetHash();
        if (transactions_.find(hash) != transactions_.end())
            return false;

        // Check if the transaction is already in the blockchain
        if (blockchain_->ContainsTransaction(hash))
            return false;

        // Verify the transaction
        if (!tx->Verify())
            return false;

        // Check if the pool is full
        if (transactions_.size() >= capacity_)
        {
            RemoveOldTransactions();
            if (transactions_.size() >= capacity_)
                return false;
        }

        // Add the transaction to the pool
        transactions_.emplace(hash, PoolItem(tx));
        return true;
    }

    bool MemoryPool::RemoveTransaction(const io::UInt256& hash)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return transactions_.erase(hash) > 0;
    }

    void MemoryPool::RemoveTransactions(const Block& block)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& tx : block.GetTransactions())
        {
            transactions_.erase(tx->GetHash());
        }
    }

    void MemoryPool::RemoveTransactions(const std::vector<Block>& blocks)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& block : blocks)
        {
            for (const auto& tx : block.GetTransactions())
            {
                transactions_.erase(tx->GetHash());
            }
        }
    }

    void MemoryPool::RemoveConflicts(const Block& block)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Create a set of spent inputs for faster lookup
        std::unordered_set<std::pair<io::UInt256, uint32_t>, InputHasher> spentInputs;
        for (const auto& tx : block.GetTransactions())
        {
            for (const auto& input : tx->GetInputs())
            {
                spentInputs.emplace(input.GetPrevHash(), input.GetPrevIndex());
            }
        }

        // Remove transactions that spend the same inputs
        for (auto it = transactions_.begin(); it != transactions_.end();)
        {
            const auto& poolTx = it->second.GetTransaction();
            bool conflict = false;

            for (const auto& poolInput : poolTx->GetInputs())
            {
                if (spentInputs.find(std::make_pair(poolInput.GetPrevHash(), poolInput.GetPrevIndex())) != spentInputs.end())
                {
                    conflict = true;
                    break;
                }
            }

            if (conflict)
                it = transactions_.erase(it);
            else
                ++it;
        }
    }

    void MemoryPool::RemoveConflicts(const std::vector<Block>& blocks)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Create a set of spent inputs for faster lookup
        std::unordered_set<std::pair<io::UInt256, uint32_t>, InputHasher> spentInputs;
        for (const auto& block : blocks)
        {
            for (const auto& tx : block.GetTransactions())
            {
                for (const auto& input : tx->GetInputs())
                {
                    spentInputs.emplace(input.GetPrevHash(), input.GetPrevIndex());
                }
            }
        }

        // Remove transactions that spend the same inputs
        for (auto it = transactions_.begin(); it != transactions_.end();)
        {
            const auto& poolTx = it->second.GetTransaction();
            bool conflict = false;

            for (const auto& poolInput : poolTx->GetInputs())
            {
                if (spentInputs.find(std::make_pair(poolInput.GetPrevHash(), poolInput.GetPrevIndex())) != spentInputs.end())
                {
                    conflict = true;
                    break;
                }
            }

            if (conflict)
                it = transactions_.erase(it);
            else
                ++it;
        }
    }

    bool MemoryPool::ContainsTransaction(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return transactions_.find(hash) != transactions_.end();
    }

    void MemoryPool::Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        transactions_.clear();
    }

    void MemoryPool::RemoveOldTransactions()
    {
        if (transactions_.size() <= capacity_)
            return;

        // Sort transactions by time
        std::vector<std::pair<io::UInt256, PoolItem>> items;
        items.reserve(transactions_.size());
        for (const auto& [hash, item] : transactions_)
        {
            items.emplace_back(hash, item);
        }
        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
            return a.second.GetTime() < b.second.GetTime();
        });

        // Remove the oldest transactions
        size_t count = transactions_.size() - capacity_;
        for (size_t i = 0; i < count; i++)
        {
            transactions_.erase(items[i].first);
        }
    }
}
