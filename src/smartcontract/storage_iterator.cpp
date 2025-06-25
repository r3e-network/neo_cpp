#include <neo/smartcontract/storage_iterator.h>
#include <stdexcept>

namespace neo::smartcontract
{
    StorageIterator::StorageIterator(std::shared_ptr<persistence::DataCache> snapshot, const persistence::StorageKey& prefix)
        : snapshot_(snapshot), prefix_(prefix), currentIndex_(0), currentPair_({io::ByteVector(), io::ByteVector()})
    {
        // Get all matching keys
        auto results = snapshot->Find(&prefix);
        for (const auto& entry : results)
        {
            entries_.push_back(entry);
        }
    }

    bool StorageIterator::HasNext() const
    {
        return currentIndex_ < entries_.size();
    }

    std::pair<io::ByteVector, io::ByteVector> StorageIterator::Next()
    {
        if (!HasNext())
            throw std::runtime_error("No more items in iterator");

        auto entry = entries_[currentIndex_++];
        currentPair_ = std::make_pair(entry.first.GetKey(), entry.second.GetValue());
        return currentPair_;
    }

    std::pair<io::ByteVector, io::ByteVector> StorageIterator::GetCurrent() const
    {
        if (currentIndex_ == 0)
            throw std::runtime_error("Iterator not advanced");

        return currentPair_;
    }
}
