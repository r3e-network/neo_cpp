#include <neo/network/p2p/payloads/high_priority.h>

namespace neo::network::p2p::payloads
{
    HighPriority::HighPriority()
    {
    }

    ledger::TransactionAttributeType HighPriority::GetType() const
    {
        return ledger::TransactionAttributeType::HighPriority;
    }

    bool HighPriority::AllowMultiple() const
    {
        return false; // Only one HighPriority attribute is allowed per transaction
    }

    int HighPriority::GetSize() const
    {
        return sizeof(uint8_t); // Only the type byte, no additional data
    }

    void HighPriority::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(GetType()));
        // HighPriority has no additional data to serialize
    }

    void HighPriority::Deserialize(io::BinaryReader& reader)
    {
        // Type is already read by the caller
        // HighPriority has no additional data to deserialize
    }

    void HighPriority::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("type", "HighPriority");
        writer.WriteEndObject();
    }

    void HighPriority::DeserializeJson(const io::JsonReader& reader)
    {
        // HighPriority has no additional data to deserialize from JSON
    }

    bool HighPriority::Verify(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // The verification logic should check if the sender is a committee member
        // For now, return true as this is a placeholder
        // TODO: Implement actual verification with committee member check
        return true;
    }

    int64_t HighPriority::CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // HighPriority attribute does not incur additional network fees
        return 0;
    }

    bool HighPriority::operator==(const HighPriority& other) const
    {
        // HighPriority has no data, so all instances are equal
        return true;
    }

    bool HighPriority::operator!=(const HighPriority& other) const
    {
        return false; // All HighPriority instances are equal
    }
} 