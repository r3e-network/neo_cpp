#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <stdexcept>

namespace neo::network::p2p::payloads
{
    GetBlocksPayload::GetBlocksPayload()
        : count_(-1) // Default count of -1 (as many blocks as possible)
    {
    }

    GetBlocksPayload::GetBlocksPayload(const io::UInt256& hashStart)
        : hashStart_(hashStart), count_(-1) // Default count of -1 (as many blocks as possible)
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

    int16_t GetBlocksPayload::GetCount() const
    {
        return count_;
    }

    void GetBlocksPayload::SetCount(int16_t count)
    {
        count_ = count;
    }

    int GetBlocksPayload::GetSize() const
    {
        return sizeof(int16_t) + io::UInt256::Size;
    }

    GetBlocksPayload GetBlocksPayload::Create(const io::UInt256& hashStart, int16_t count)
    {
        GetBlocksPayload payload;
        payload.SetHashStart(hashStart);
        payload.SetCount(count);
        return payload;
    }

    void GetBlocksPayload::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(hashStart_);
        writer.Write(count_);
    }

    void GetBlocksPayload::Deserialize(io::BinaryReader& reader)
    {
        hashStart_ = reader.ReadUInt256();
        count_ = reader.ReadInt16();

        // Validate count
        if (count_ < -1 || count_ == 0)
            throw std::runtime_error("Invalid count");
    }

    void GetBlocksPayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("hash_start", hashStart_.ToHexString());
        writer.WriteProperty("count", count_);
        writer.WriteEndObject();
    }

    void GetBlocksPayload::DeserializeJson(const io::JsonReader& reader)
    {
        // JSON deserialization implementation
        // This would parse the JSON object and extract hash_start and count
        // For now, this is a placeholder
        throw std::runtime_error("JSON deserialization not implemented");
    }
}
