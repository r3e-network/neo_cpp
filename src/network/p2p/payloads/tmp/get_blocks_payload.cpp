#include <neo/network/payloads/get_blocks_payload.h>

namespace neo::network::payloads
{
    GetBlocksPayload::GetBlocksPayload()
        : count_(500) // Default count of 500 blocks
    {
    }

    GetBlocksPayload::GetBlocksPayload(const io::UInt256& hashStart)
        : hashStart_(hashStart), count_(500) // Default count of 500 blocks
    {
    }

    const io::UInt256& GetBlocksPayload::GetHashStart() const
    {
        return hashStart_;
    }

    void GetBlocksPayload::SetHashStart(const io::UInt256& hashStart)
    {
        hashStart_ = hashStart;
    }

    uint16_t GetBlocksPayload::GetCount() const
    {
        return count_;
    }

    void GetBlocksPayload::SetCount(uint16_t count)
    {
        count_ = count;
    }

    void GetBlocksPayload::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(hashStart_);
        writer.Write(count_);
    }

    void GetBlocksPayload::Deserialize(io::BinaryReader& reader)
    {
        hashStart_ = reader.ReadUInt256();
        count_ = reader.ReadUInt16();
    }

    void GetBlocksPayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("hash_start", hashStart_);
        writer.Write("count", count_);
    }

    void GetBlocksPayload::DeserializeJson(const io::JsonReader& reader)
    {
        hashStart_ = reader.ReadUInt256("hash_start");
        count_ = reader.ReadUInt16("count");
    }
}
