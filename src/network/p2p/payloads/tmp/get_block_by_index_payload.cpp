#include <neo/network/payloads/get_block_by_index_payload.h>

namespace neo::network::payloads
{
    GetBlockByIndexPayload::GetBlockByIndexPayload()
        : indexStart_(0), count_(500) // Default count of 500 blocks
    {
    }

    GetBlockByIndexPayload::GetBlockByIndexPayload(uint32_t indexStart, uint16_t count)
        : indexStart_(indexStart), count_(count)
    {
    }

    uint32_t GetBlockByIndexPayload::GetIndexStart() const
    {
        return indexStart_;
    }

    void GetBlockByIndexPayload::SetIndexStart(uint32_t indexStart)
    {
        indexStart_ = indexStart;
    }

    uint16_t GetBlockByIndexPayload::GetCount() const
    {
        return count_;
    }

    void GetBlockByIndexPayload::SetCount(uint16_t count)
    {
        count_ = count;
    }

    void GetBlockByIndexPayload::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(indexStart_);
        writer.Write(count_);
    }

    void GetBlockByIndexPayload::Deserialize(io::BinaryReader& reader)
    {
        indexStart_ = reader.ReadUInt32();
        count_ = reader.ReadUInt16();
    }

    void GetBlockByIndexPayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("index_start", indexStart_);
        writer.Write("count", count_);
    }

    void GetBlockByIndexPayload::DeserializeJson(const io::JsonReader& reader)
    {
        indexStart_ = reader.ReadUInt32("index_start");
        count_ = reader.ReadUInt16("count");
    }
}
