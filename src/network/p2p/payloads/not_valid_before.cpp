/**
 * @file not_valid_before.cpp
 * @brief Not Valid Before
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p/payloads/not_valid_before.h>

namespace neo::network::p2p::payloads
{
NotValidBefore::NotValidBefore() : height_(0) {}

NotValidBefore::NotValidBefore(uint32_t height) : height_(height) {}

uint32_t NotValidBefore::GetHeight() const { return height_; }

void NotValidBefore::SetHeight(uint32_t height) { height_ = height; }

ledger::TransactionAttribute::Usage NotValidBefore::GetType() const
{
    return ledger::TransactionAttribute::Usage::NotValidBefore;
}

bool NotValidBefore::AllowMultiple() const
{
    return false;  // Only one NotValidBefore attribute is allowed per transaction
}

int NotValidBefore::GetSize() const
{
    return sizeof(uint8_t) + sizeof(uint32_t);  // Type byte + height
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
    // Basic verification for NotValidBefore attribute
    // In the real implementation, this would check current blockchain height
    try
    {
        // Basic validation - height should be non-zero
        if (height_ == 0)
        {
            return false;
        }

        // Height validation requires blockchain state access
        // Real verification would check:
        // 1. Current blockchain height vs required height
        // 2. If the transaction is being processed at the correct time
        return true;
    }
    catch (const std::exception& e)
    {
        // Error during verification - reject as invalid
        return false;
    }
}

int64_t NotValidBefore::CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const
{
    // NotValidBefore attribute does not incur additional network fees
    return 0;
}

bool NotValidBefore::operator==(const NotValidBefore& other) const { return height_ == other.height_; }

bool NotValidBefore::operator!=(const NotValidBefore& other) const { return !(*this == other); }
}  // namespace neo::network::p2p::payloads