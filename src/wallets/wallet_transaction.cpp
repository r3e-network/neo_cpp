#include <neo/wallets/wallet_transaction.h>

namespace neo::wallets
{
    WalletTransaction::WalletTransaction(std::shared_ptr<Neo3Transaction> transaction)
        : transaction_(transaction)
        , confirmed_(false)
        , block_height_(0)
    {
    }

    io::UInt256 WalletTransaction::GetHash() const
    {
        if (transaction_)
        {
            return transaction_->GetHash();
        }
        return io::UInt256::Zero();
    }

    void WalletTransaction::SerializeJson(io::JsonWriter& /* writer */) const
    {
        // Simplified JSON serialization - basic implementation
        // In full implementation, this would write proper JSON
    }

    void WalletTransaction::DeserializeJson(const io::JsonReader& /* reader */)
    {
        // Basic deserialization - in full implementation this would parse the JSON
        // For now, just set default values
        confirmed_ = false;
        block_height_ = 0;
    }

} // namespace neo::wallets 