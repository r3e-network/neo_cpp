#include <neo/network/p2p/payloads/conflicts.h>

namespace neo::network::p2p::payloads
{
Conflicts::Conflicts() {}

Conflicts::Conflicts(const io::UInt256& hash) : hash_(hash) {}

const io::UInt256& Conflicts::GetHash() const
{
    return hash_;
}

void Conflicts::SetHash(const io::UInt256& hash)
{
    hash_ = hash;
}

ledger::TransactionAttribute::Usage Conflicts::GetType() const
{
    return ledger::TransactionAttribute::Usage::Conflicts;
}

bool Conflicts::AllowMultiple() const
{
    return true;  // Multiple conflicts are allowed
}

int Conflicts::GetSize() const
{
    return sizeof(uint8_t) + 32;  // Type byte + UInt256 (32 bytes)
}

void Conflicts::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(GetType()));
    writer.Write(hash_);
}

void Conflicts::Deserialize(io::BinaryReader& reader)
{
    // Type is already read by the caller
    hash_ = reader.ReadUInt256();
}

void Conflicts::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("type", "Conflicts");
    writer.WriteProperty("hash", hash_.ToString());
    writer.WriteEndObject();
}

void Conflicts::DeserializeJson(const io::JsonReader& reader)
{
    if (reader.GetJson().contains("hash") && reader.GetJson()["hash"].is_string())
    {
        hash_ = io::UInt256::Parse(reader.GetJson()["hash"].get<std::string>());
    }
}

bool Conflicts::Verify(/* DataCache& snapshot, const Transaction& transaction */) const
{
    // Basic verification - just check if hash is non-zero
    // For a complete implementation, this would need access to the system context
    try
    {
        // Basic validation - hash should not be zero
        if (hash_.IsZero())
        {
            return false;
        }

        // Non-zero hashes are considered valid conflicts
        // In a complete implementation, this would check:
        // 1. If the conflicting transaction exists in mempool
        // 2. If the conflicting transaction exists on blockchain
        // 3. If there's actually a conflict
        return true;  // Non-zero hashes are considered valid
    }
    catch (const std::exception& e)
    {
        // Error during verification - reject as invalid
        return false;
    }
}

int64_t Conflicts::CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const
{
    // Basic network fee calculation for Conflicts attribute
    // In the real implementation, this would be based on protocol settings
    return 5000000;  // 0.05 GAS in datoshi (Neo N3 default fee for conflicts)
}

bool Conflicts::operator==(const Conflicts& other) const
{
    return hash_ == other.hash_;
}

bool Conflicts::operator!=(const Conflicts& other) const
{
    return !(*this == other);
}
}  // namespace neo::network::p2p::payloads