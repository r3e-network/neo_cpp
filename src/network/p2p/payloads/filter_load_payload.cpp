/**
 * @file filter_load_payload.cpp
 * @brief Filter Load Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p/payloads/filter_load_payload.h>

namespace neo::network::p2p::payloads
{
FilterLoadPayload::FilterLoadPayload() : k_(0), tweak_(0), flags_(0) {}

FilterLoadPayload::FilterLoadPayload(const io::ByteVector& filter, uint8_t k, uint32_t tweak, uint8_t flags)
    : filter_(filter), k_(k), tweak_(tweak), flags_(flags)
{
}

const io::ByteVector& FilterLoadPayload::GetFilter() const { return filter_; }

void FilterLoadPayload::SetFilter(const io::ByteVector& filter) { filter_ = filter; }

uint8_t FilterLoadPayload::GetK() const { return k_; }

void FilterLoadPayload::SetK(uint8_t k) { k_ = k; }

uint32_t FilterLoadPayload::GetTweak() const { return tweak_; }

void FilterLoadPayload::SetTweak(uint32_t tweak) { tweak_ = tweak; }

uint8_t FilterLoadPayload::GetFlags() const { return flags_; }

void FilterLoadPayload::SetFlags(uint8_t flags) { flags_ = flags; }

void FilterLoadPayload::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarBytes(filter_.AsSpan());
    writer.Write(k_);
    writer.Write(tweak_);
    writer.Write(flags_);
}

void FilterLoadPayload::Deserialize(io::BinaryReader& reader)
{
    filter_ = reader.ReadVarBytes(MaxFilterSize);
    k_ = reader.ReadByte();
    tweak_ = reader.ReadUInt32();
    flags_ = reader.ReadByte();
}

void FilterLoadPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.Write("filter", filter_.ToHexString());
    writer.Write("k", k_);
    writer.Write("tweak", tweak_);
    writer.Write("flags", flags_);
    writer.WriteEndObject();
}

void FilterLoadPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    filter_ = io::ByteVector::FromHexString(reader.ReadString("filter"));
    k_ = reader.ReadUInt8("k");
    tweak_ = reader.ReadUInt32("tweak");
    flags_ = reader.ReadUInt8("flags");
    reader.ReadEndObject();
}
}  // namespace neo::network::p2p::payloads
