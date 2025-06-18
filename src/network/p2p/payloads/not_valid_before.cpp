#include <neo/network/p2p/payloads/not_valid_before.h>

namespace neo::network::p2p::payloads
{
    NotValidBefore::NotValidBefore()
        : height_(0)
    {
    }

    NotValidBefore::NotValidBefore(uint32_t height)
        : height_(height)
    {
    }

    uint32_t NotValidBefore::GetHeight() const
    {
        return height_;
    }

    void NotValidBefore::SetHeight(uint32_t height)
    {
        height_ = height;
    }

    ledger::TransactionAttributeType NotValidBefore::GetType() const
    {
        return ledger::TransactionAttributeType::NotValidBefore;
    }

    bool NotValidBefore::AllowMultiple() const
    {
        return false; // Only one NotValidBefore attribute is allowed per transaction
    }

    int NotValidBefore::GetSize() const
    {
        return sizeof(uint8_t) + sizeof(uint32_t); // Type byte + height
    }

    void NotValidBefore::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(GetType()));
        writer.Write(height_);
    }

    void NotValidBefore::Deserialize(io::BinaryReader& reader)
    {
        // Type is already read by the caller
        height_ = reader.ReadUInt32();
    }

    void NotValidBefore::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("type", "NotValidBefore");
        writer.WriteProperty("height", height_);
        writer.WriteEndObject();
    }

    void NotValidBefore::DeserializeJson(const io::JsonReader& reader)
    {
        if (reader.GetJson().contains("height") && reader.GetJson()["height"].is_number())
        {
            height_ = reader.GetJson()["height"].get<uint32_t>();
        }
    }

    bool NotValidBefore::Verify(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // The verification logic should check if the current block height is >= height_
        // For now, return true as this is a placeholder
        // TODO: Implement actual verification with current block height
        return true;
    }

    int64_t NotValidBefore::CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // NotValidBefore attribute does not incur additional network fees
        return 0;
    }

    bool NotValidBefore::operator==(const NotValidBefore& other) const
    {
        return height_ == other.height_;
    }

    bool NotValidBefore::operator!=(const NotValidBefore& other) const
    {
        return !(*this == other);
    }
} 