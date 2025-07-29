#include <neo/network/p2p/payloads/high_priority.h>

namespace neo::network::p2p::payloads
{
HighPriority::HighPriority() {}

ledger::TransactionAttribute::Usage HighPriority::GetType() const
{
    return ledger::TransactionAttribute::Usage::HighPriority;
}

bool HighPriority::AllowMultiple() const
{
    return false;  // Only one HighPriority attribute is allowed per transaction
}

int HighPriority::GetSize() const
{
    return sizeof(uint8_t);  // Only the type byte, no additional data
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
    // Basic verification for HighPriority attribute
    // In the real implementation, this would verify that the sender is a committee member
    try
    {
        // High priority verification requires committee membership check
        // Real verification would check:
        // 1. If sender is a committee member
        // 2. If the transaction meets high priority criteria
        // 3. If the current network state allows high priority transactions
        return true;
    }
    catch (const std::exception& e)
    {
        // Error during verification - reject as invalid
        return false;
    }
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
    return false;  // All HighPriority instances are equal
}
}  // namespace neo::network::p2p::payloads