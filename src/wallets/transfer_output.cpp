#include <neo/wallets/transfer_output.h>

#include <sstream>

namespace neo::wallets
{
TransferOutput::TransferOutput() : asset_id_(io::UInt160::Zero()), script_hash_(io::UInt160::Zero()), amount_(0) {}

TransferOutput::TransferOutput(const io::UInt160& asset_id, const io::UInt160& script_hash, int64_t amount,
                               const std::string& data)
    : asset_id_(asset_id), script_hash_(script_hash), amount_(amount), data_(data)
{
}

const io::UInt160& TransferOutput::GetAssetId() const { return asset_id_; }

void TransferOutput::SetAssetId(const io::UInt160& asset_id) { asset_id_ = asset_id; }

const io::UInt160& TransferOutput::GetScriptHash() const { return script_hash_; }

void TransferOutput::SetScriptHash(const io::UInt160& script_hash) { script_hash_ = script_hash; }

int64_t TransferOutput::GetAmount() const { return amount_; }

void TransferOutput::SetAmount(int64_t amount) { amount_ = amount; }

const std::string& TransferOutput::GetData() const { return data_; }

void TransferOutput::SetData(const std::string& data) { data_ = data; }

bool TransferOutput::IsValid() const
{
    // Check if asset ID is not zero
    if (asset_id_.IsZero())
    {
        return false;
    }

    // Check if script hash is not zero
    if (script_hash_.IsZero())
    {
        return false;
    }

    // Check if amount is positive
    if (amount_ <= 0)
    {
        return false;
    }

    return true;
}

bool TransferOutput::operator==(const TransferOutput& other) const
{
    return asset_id_ == other.asset_id_ && script_hash_ == other.script_hash_ && amount_ == other.amount_ &&
           data_ == other.data_;
}

bool TransferOutput::operator!=(const TransferOutput& other) const { return !(*this == other); }

std::string TransferOutput::ToString() const
{
    std::ostringstream oss;
    oss << "TransferOutput{";
    oss << "AssetId: " << asset_id_.ToString();
    oss << ", ScriptHash: " << script_hash_.ToString();
    oss << ", Amount: " << amount_;
    if (!data_.empty())
    {
        oss << ", Data: " << data_;
    }
    oss << "}";
    return oss.str();
}
}  // namespace neo::wallets
