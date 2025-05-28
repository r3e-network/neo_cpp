#include <neo/network/payloads/inventory_payload.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <limits>
#include <stdexcept>

namespace neo::network::payloads
{
    InventoryPayload::InventoryPayload()
        : type_(InventoryType::TX)
    {
    }

    InventoryPayload::InventoryPayload(InventoryType type, const std::vector<io::UInt256>& hashes)
        : type_(type), hashes_(hashes)
    {
    }

    InventoryType InventoryPayload::GetType() const
    {
        return type_;
    }

    const std::vector<io::UInt256>& InventoryPayload::GetHashes() const
    {
        return hashes_;
    }

    void InventoryPayload::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(type_));
        writer.WriteVarInt(hashes_.size());
        for (const auto& hash : hashes_)
        {
            writer.Write(hash);
        }
    }

    void InventoryPayload::Deserialize(io::BinaryReader& reader)
    {
        type_ = static_cast<InventoryType>(reader.ReadUInt8());
        int64_t count = reader.ReadVarInt();
        if (count < 0 || count > std::numeric_limits<size_t>::max())
            throw std::out_of_range("Invalid hash count");
        
        hashes_.clear();
        hashes_.reserve(static_cast<size_t>(count));
        
        for (int64_t i = 0; i < count; i++)
        {
            hashes_.push_back(reader.ReadUInt256());
        }
    }
    
    void InventoryPayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("type", static_cast<uint8_t>(type_));
        
        writer.WriteStartArray("hashes");
        for (const auto& hash : hashes_)
        {
            writer.WriteValue(hash.ToString());
        }
        writer.WriteEndArray();
    }
    
    void InventoryPayload::DeserializeJson(const io::JsonReader& reader)
    {
        type_ = static_cast<InventoryType>(reader.ReadUInt8("type"));
        
        auto hashesArray = reader.ReadArray("hashes");
        hashes_.clear();
        hashes_.reserve(hashesArray.size());
        
        for (const auto& hashJson : hashesArray)
        {
            std::string hashStr = hashJson.get<std::string>();
            hashes_.push_back(io::UInt256::Parse(hashStr));
        }
    }
}
