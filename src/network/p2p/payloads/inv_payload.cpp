#include <neo/network/p2p/payloads/inv_payload.h>
#include <stdexcept>

namespace neo::network::p2p::payloads
{
InvPayload::InvPayload() : type_(InventoryType::Transaction) {}

InvPayload::InvPayload(InventoryType type, const std::vector<io::UInt256>& hashes) : type_(type), hashes_(hashes) {}

InvPayload::InvPayload(const std::vector<InventoryVector>& inventories)
{
    // This constructor assumes all InventoryVectors have the same type
    if (!inventories.empty())
    {
        type_ = inventories[0].GetType();
        hashes_.reserve(inventories.size());

        for (const auto& inv : inventories)
        {
            if (inv.GetType() != type_)
            {
                throw std::runtime_error("All inventory vectors must have the same type");
            }

            hashes_.push_back(inv.GetHash());
        }
    }
    else
    {
        type_ = InventoryType::Transaction;
    }
}

InventoryType InvPayload::GetType() const
{
    return type_;
}

void InvPayload::SetType(InventoryType type)
{
    type_ = type;
}

std::vector<io::UInt256> InvPayload::GetHashes() const
{
    return hashes_;
}

void InvPayload::SetHashes(const std::vector<io::UInt256>& hashes)
{
    hashes_ = hashes;
}

const std::vector<InventoryVector>& InvPayload::GetInventories() const
{
    static thread_local std::vector<InventoryVector> inventories;

    inventories.clear();
    inventories.reserve(hashes_.size());

    for (const auto& hash : hashes_)
    {
        inventories.emplace_back(type_, hash);
    }

    return inventories;
}

void InvPayload::SetInventories(const std::vector<InventoryVector>& inventories)
{
    // This method assumes all InventoryVectors have the same type
    if (!inventories.empty())
    {
        type_ = inventories[0].GetType();
        hashes_.clear();
        hashes_.reserve(inventories.size());

        for (const auto& inv : inventories)
        {
            if (inv.GetType() != type_)
            {
                throw std::runtime_error("All inventory vectors must have the same type");
            }

            hashes_.push_back(inv.GetHash());
        }
    }
    else
    {
        type_ = InventoryType::Transaction;
        hashes_.clear();
    }
}

size_t InvPayload::GetSize() const
{
    // Type (1 byte) + Hashes size
    size_t hashesSize = 1;  // VarInt for count
    if (hashes_.size() >= 0xFD)
    {
        hashesSize += 2;  // Add 2 more bytes for length if count is large
    }

    // Each hash is 32 bytes
    hashesSize += hashes_.size() * 32;

    return sizeof(uint8_t) + hashesSize;
}

InvPayload InvPayload::Create(InventoryType type, const std::vector<io::UInt256>& hashes)
{
    return InvPayload(type, hashes);
}

std::vector<InvPayload> InvPayload::CreateGroup(InventoryType type, const std::vector<io::UInt256>& hashes)
{
    std::vector<InvPayload> result;

    // Process in chunks of MaxHashesCount
    for (size_t i = 0; i < hashes.size(); i += MaxHashesCount)
    {
        size_t chunkSize = std::min<size_t>(MaxHashesCount, hashes.size() - i);
        std::vector<io::UInt256> chunk(hashes.begin() + i, hashes.begin() + i + chunkSize);

        result.push_back(InvPayload(type, chunk));
    }

    return result;
}

void InvPayload::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(type_));
    writer.WriteVarInt(hashes_.size());

    for (const auto& hash : hashes_)
    {
        writer.Write(hash);
    }
}

void InvPayload::Deserialize(io::BinaryReader& reader)
{
    type_ = static_cast<InventoryType>(reader.ReadUInt8());

    // Validate InventoryType
    if (type_ != InventoryType::Transaction && type_ != InventoryType::Block && type_ != InventoryType::Consensus)
    {
        throw std::runtime_error("Invalid inventory type");
    }

    uint64_t count = reader.ReadVarInt();
    hashes_.clear();
    hashes_.reserve(count);

    for (uint64_t i = 0; i < count; i++)
    {
        io::UInt256 hash = reader.ReadUInt256();
        hashes_.push_back(hash);
    }
}

void InvPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("type", static_cast<uint8_t>(type_));

    nlohmann::json hashesArray = nlohmann::json::array();
    for (const auto& hash : hashes_)
    {
        hashesArray.push_back(hash.ToHexString());
    }

    writer.Write("hashes", hashesArray);
}

void InvPayload::DeserializeJson(const io::JsonReader& reader)
{
    type_ = static_cast<InventoryType>(reader.ReadUInt8("type"));

    // Validate InventoryType
    if (type_ != InventoryType::Transaction && type_ != InventoryType::Block && type_ != InventoryType::Consensus)
    {
        throw std::runtime_error("Invalid inventory type");
    }

    auto hashesArray = reader.ReadArray("hashes");
    hashes_.clear();
    hashes_.reserve(hashesArray.size());

    for (const auto& hashJson : hashesArray)
    {
        std::string hashStr = hashJson.get<std::string>();
        io::UInt256 hash = io::UInt256::Parse(hashStr);
        hashes_.push_back(hash);
    }
}
}  // namespace neo::network::p2p::payloads
