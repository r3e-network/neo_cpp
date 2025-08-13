/**
 * @file header_cache.cpp
 * @brief Caching mechanisms
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/ledger/header_cache.h>

#include <algorithm>

namespace neo::ledger
{
HeaderCache::HeaderCache(size_t max_size) : max_size_(max_size)
{
    if (max_size_ == 0)
    {
        max_size_ = 1;
    }
}

bool HeaderCache::Add(std::shared_ptr<Header> header)
{
    if (!header)
    {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto hash = header->GetHash();
    auto index = header->GetIndex();

    // Check if already exists by hash
    if (hash_index_.find(hash) != hash_index_.end())
    {
        return false;
    }

    // Check if a header with the same index already exists
    auto height_it = height_index_.find(index);
    if (height_it != height_index_.end())
    {
        // Replace the existing header with the same index
        auto old_header = height_it->second;
        auto old_hash = old_header->GetHash();

        // Remove old header from hash index
        hash_index_.erase(old_hash);

        // Remove old header from deque
        auto deque_it = std::find(headers_.begin(), headers_.end(), old_header);
        if (deque_it != headers_.end())
        {
            headers_.erase(deque_it);
        }
    }

    // Add new header to all indices
    headers_.push_back(header);
    hash_index_[hash] = header;
    height_index_[index] = header;

    // Maintain size limit
    while (headers_.size() > max_size_)
    {
        TryRemoveFirst();
    }

    return true;
}

std::shared_ptr<Header> HeaderCache::Get(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = hash_index_.find(hash);
    if (it != hash_index_.end())
    {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<Header> HeaderCache::Get(uint32_t index) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = height_index_.find(index);
    if (it != height_index_.end())
    {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<Header> HeaderCache::GetLast() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (headers_.empty())
    {
        return nullptr;
    }

    return headers_.back();
}

bool HeaderCache::IsFull() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return headers_.size() >= max_size_;
}

size_t HeaderCache::Size() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return headers_.size();
}

size_t HeaderCache::MaxSize() const { return max_size_; }

bool HeaderCache::Contains(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return hash_index_.find(hash) != hash_index_.end();
}

bool HeaderCache::Remove(const io::UInt256& hash)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = hash_index_.find(hash);
    if (it == hash_index_.end())
    {
        return false;
    }

    auto header = it->second;
    auto index = header->GetIndex();

    // Remove from all indices
    hash_index_.erase(it);
    height_index_.erase(index);

    // Remove from deque
    auto deque_it = std::find(headers_.begin(), headers_.end(), header);
    if (deque_it != headers_.end())
    {
        headers_.erase(deque_it);
    }

    return true;
}

bool HeaderCache::TryRemoveFirst()
{
    // This method assumes mutex is already locked
    if (headers_.empty())
    {
        return false;
    }

    auto first_header = headers_.front();
    auto hash = first_header->GetHash();
    auto index = first_header->GetIndex();

    headers_.pop_front();
    hash_index_.erase(hash);
    height_index_.erase(index);

    return true;
}

void HeaderCache::Clear()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    headers_.clear();
    hash_index_.clear();
    height_index_.clear();
}
}  // namespace neo::ledger
