#include <neo/network/p2p/payloads/conflicts.h>

namespace neo::network::p2p::payloads
{
    Conflicts::Conflicts()
    {
    }

    Conflicts::Conflicts(const io::UInt256& hash)
        : hash_(hash)
    {
    }

    const io::UInt256& Conflicts::GetHash() const
    {
        return hash_;
    }

    void Conflicts::SetHash(const io::UInt256& hash)
    {
        hash_ = hash;
    }

    ledger::TransactionAttributeType Conflicts::GetType() const
    {
        return ledger::TransactionAttributeType::Conflicts;
    }

    bool Conflicts::AllowMultiple() const
    {
        return true; // Multiple conflicts are allowed
    }

    int Conflicts::GetSize() const
    {
        return sizeof(uint8_t) + 32; // Type byte + UInt256 (32 bytes)
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
        // The verification logic should ensure the conflicting transaction
        // is either in the mempool or on the blockchain
        // For now, return true as this is a placeholder
        // TODO: Implement actual verification with mempool/blockchain check
        return true;
    }

    int64_t Conflicts::CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // Conflicts attribute incurs a network fee
        // TODO: Get this value from protocol settings
        return 5000000; // 0.05 GAS in datoshi (typical fee for conflicts)
    }

    bool Conflicts::operator==(const Conflicts& other) const
    {
        return hash_ == other.hash_;
    }

    bool Conflicts::operator!=(const Conflicts& other) const
    {
        return !(*this == other);
    }
} 