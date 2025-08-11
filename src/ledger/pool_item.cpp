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

    hash_ = transaction_->GetHash();
    fee_per_byte_ = CalculateFeePerByte();
}

std::shared_ptr<Transaction> PoolItem::GetTransaction() const { return transaction_; }

std::chrono::system_clock::time_point PoolItem::GetTimestamp() const { return timestamp_; }

uint64_t PoolItem::GetFeePerByte() const { return fee_per_byte_; }

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

bool PoolItem::operator!=(const PoolItem& other) const { return !(*this == other); }

io::UInt256 PoolItem::GetHash() const { return hash_; }

int64_t PoolItem::GetNetworkFee() const { return transaction_->GetNetworkFee(); }

int64_t PoolItem::GetSystemFee() const { return transaction_->GetSystemFee(); }

int PoolItem::GetSize() const { return transaction_->GetSize(); }

bool PoolItem::ConflictsWith(const PoolItem& other) const
{
    // Check if transactions have same signers with conflicting scopes
    const auto& our_signers = transaction_->GetSigners();
    const auto& other_signers = other.transaction_->GetSigners();

    // Check for account conflicts (same sender with overlapping scopes)
    for (const auto& our_signer : our_signers)
    {
        for (const auto& other_signer : other_signers)
        {
            if (our_signer.GetAccount() == other_signer.GetAccount())
            {
                // Same account - check if witness scopes conflict
                auto our_scope = our_signer.GetScopes();
                auto other_scope = other_signer.GetScopes();

                // If both have Global scope, they conflict
                if ((our_scope & ledger::WitnessScope::Global) != ledger::WitnessScope::None &&
                    (other_scope & ledger::WitnessScope::Global) != ledger::WitnessScope::None)
                {
                    return true;
                }

                // Check for overlapping contract-specific scopes
                if ((our_scope & ledger::WitnessScope::CustomContracts) != ledger::WitnessScope::None &&
                    (other_scope & ledger::WitnessScope::CustomContracts) != ledger::WitnessScope::None)
                {
                    // Would need to check AllowedContracts lists for overlap
                    // Assume conflict if both have CustomContracts scope
                    return true;
                }
            }
        }
    }

    // No conflicts detected
    return false;
}

uint64_t PoolItem::CalculateFeePerByte() const
{
    auto size = transaction_->GetSize();
    if (size == 0)
    {
        return 0;
    }

    // Use integer division to avoid Fixed8 dependency issues
    return static_cast<uint64_t>(transaction_->GetNetworkFee() / size);
}
}  // namespace neo::ledger
