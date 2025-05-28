#include <neo/network/p2p/inventory_vector.h>

namespace neo::network::p2p
{
    InventoryVector::InventoryVector()
        : type_(InventoryType::Transaction)
    {
    }
    
    InventoryVector::InventoryVector(InventoryType type, const io::UInt256& hash)
        : type_(type), hash_(hash)
    {
    }
    
    InventoryType InventoryVector::GetType() const
    {
        return type_;
    }
    
    void InventoryVector::SetType(InventoryType type)
    {
        type_ = type;
    }
    
    const io::UInt256& InventoryVector::GetHash() const
    {
        return hash_;
    }
    
    void InventoryVector::SetHash(const io::UInt256& hash)
    {
        hash_ = hash;
    }
    
    void InventoryVector::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(type_));
        writer.Write(hash_);
    }
    
    void InventoryVector::Deserialize(io::BinaryReader& reader)
    {
        type_ = static_cast<InventoryType>(reader.ReadUInt8());
        hash_ = reader.ReadSerializable<io::UInt256>();
    }
    
    void InventoryVector::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("type", static_cast<uint8_t>(type_));
        writer.Write("hash", hash_);
    }
    
    void InventoryVector::DeserializeJson(const io::JsonReader& reader)
    {
        type_ = static_cast<InventoryType>(reader.ReadUInt8("type"));
        hash_ = reader.ReadUInt256("hash");
    }
    
    bool InventoryVector::operator==(const InventoryVector& other) const
    {
        return type_ == other.type_ && hash_ == other.hash_;
    }
    
    bool InventoryVector::operator!=(const InventoryVector& other) const
    {
        return !(*this == other);
    }
}
