/**
 * @file filter_add_payload.cpp
 * @brief Filter Add Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p/payloads/filter_add_payload.h>

namespace neo::network::p2p::payloads
{
FilterAddPayload::FilterAddPayload() = default;

FilterAddPayload::FilterAddPayload(const io::ByteVector& data) : data_(data) {}

const io::ByteVector& FilterAddPayload::GetData() const { return data_; }

void FilterAddPayload::SetData(const io::ByteVector& data) { data_ = data; }

void FilterAddPayload::Serialize(io::BinaryWriter& writer) const { writer.WriteVarBytes(data_.AsSpan()); }

void FilterAddPayload::Deserialize(io::BinaryReader& reader) { data_ = reader.ReadVarBytes(520); }

void FilterAddPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.Write("data", data_.ToHexString());
    writer.WriteEndObject();
}

void FilterAddPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    data_ = io::ByteVector::FromHexString(reader.ReadString("data"));
    reader.ReadEndObject();
}
}  // namespace neo::network::p2p::payloads
