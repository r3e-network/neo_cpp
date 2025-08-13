/**
 * @file get_block_by_index_payload.cpp
 * @brief Block structure and validation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p/payloads/get_block_by_index_payload.h>

namespace neo::network::p2p::payloads
{
GetBlockByIndexPayload::GetBlockByIndexPayload() : indexStart_(0), count_(500) {}

GetBlockByIndexPayload::GetBlockByIndexPayload(uint32_t indexStart, uint16_t count)
    : indexStart_(indexStart), count_(count)
{
}

uint32_t GetBlockByIndexPayload::GetIndexStart() const { return indexStart_; }

void GetBlockByIndexPayload::SetIndexStart(uint32_t indexStart) { indexStart_ = indexStart; }

uint16_t GetBlockByIndexPayload::GetCount() const { return count_; }

void GetBlockByIndexPayload::SetCount(uint16_t count) { count_ = count; }

size_t GetBlockByIndexPayload::GetSize() const { return sizeof(uint32_t) + sizeof(uint16_t); }

GetBlockByIndexPayload GetBlockByIndexPayload::Create(uint32_t indexStart, uint16_t count)
{
    return GetBlockByIndexPayload(indexStart, count);
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
    writer.WriteStartObject();
    writer.Write("indexStart", indexStart_);
    writer.Write("count", count_);
    writer.WriteEndObject();
}

void GetBlockByIndexPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    indexStart_ = reader.ReadUInt32("indexStart");
    count_ = reader.ReadUInt16("count");
    reader.ReadEndObject();
}
}  // namespace neo::network::p2p::payloads
