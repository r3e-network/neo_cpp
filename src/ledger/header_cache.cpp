#include <neo/ledger/header_cache.h>
#include <algorithm>

namespace neo::ledger
{
    HeaderCache::HeaderCache(size_t max_size)
        : max_size_(max_size)
    {
        if (max_size_ == 0)
        {
            max_size_ = 1;
        }
    }

    void HeaderCache::Add(std::shared_ptr<BlockHeader> header)
    {
        if (!header)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        auto hash = header->GetHash();
        headers_[hash] = header;
        
        EvictIfNeeded();
    }

    std::shared_ptr<BlockHeader> HeaderCache::Get(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = headers_.find(hash);
        if (it != headers_.end())
        {
            return it->second;
        }
        
        return nullptr;
    }

    bool HeaderCache::Contains(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return headers_.count(hash) > 0;
    }

    bool HeaderCache::Remove(const io::UInt256& hash)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = headers_.find(hash);
        if (it != headers_.end())
        {
            headers_.erase(it);
            return true;
        }
        
        return false;
    }

    void HeaderCache::Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        headers_.clear();
    }

    size_t HeaderCache::Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return headers_.size();
    }

    size_t HeaderCache::MaxSize() const
    {
        return max_size_;
    }

    bool HeaderCache::IsFull() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return headers_.size() >= max_size_;
    }

    void HeaderCache::EvictIfNeeded()
    {
        // This method assumes mutex is already locked
        if (headers_.size() <= max_size_)
        {
            return;
        }

        // Simple eviction strategy: remove the header with the lowest index
        // In a more sophisticated implementation, we might use LRU or other strategies
        auto min_it = std::min_element(headers_.begin(), headers_.end(),
            [](const auto& a, const auto& b) {
                return a.second->GetIndex() < b.second->GetIndex();
            });

        if (min_it != headers_.end())
        {
            headers_.erase(min_it);
        }
    }
}
