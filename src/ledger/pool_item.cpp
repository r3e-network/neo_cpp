#include <neo/ledger/pool_item.h>

namespace neo::ledger
{
PoolItem::PoolItem(std::shared_ptr<Transaction> transaction)
    : transaction_(transaction), timestamp_(std::chrono::system_clock::now())
{
    if (!transaction_)
    {
        throw std::invalid_argument("Transaction cannot be null");
    }

    fee_per_byte_ = CalculateFeePerByte();
}

std::shared_ptr<Transaction> PoolItem::GetTransaction() const
{
    return transaction_;
}

std::chrono::system_clock::time_point PoolItem::GetTimestamp() const
{
    return timestamp_;
}

uint64_t PoolItem::GetFeePerByte() const
{
    return fee_per_byte_;
}

bool PoolItem::HasHigherPriorityThan(const PoolItem& other) const
{
    // Higher fee per byte = higher priority
    if (fee_per_byte_ != other.fee_per_byte_)
    {
        return fee_per_byte_ > other.fee_per_byte_;
    }

    // If fees are equal, earlier timestamp = higher priority
    return timestamp_ < other.timestamp_;
}

bool PoolItem::operator<(const PoolItem& other) const
{
    // For use in priority queue (max heap), return true if this has lower priority
    return !HasHigherPriorityThan(other) && *this != other;
}

bool PoolItem::operator==(const PoolItem& other) const
{
    return transaction_->GetHash() == other.transaction_->GetHash();
}

bool PoolItem::operator!=(const PoolItem& other) const
{
    return !(*this == other);
}

uint64_t PoolItem::CalculateFeePerByte() const
{
    auto size = transaction_->GetSize();
    if (size == 0)
    {
        return 0;
    }

    auto fee_per_byte = io::Fixed8(transaction_->GetNetworkFee()) / io::Fixed8(static_cast<int64_t>(size));
    return static_cast<uint64_t>(fee_per_byte.Value());
}
}  // namespace neo::ledger
