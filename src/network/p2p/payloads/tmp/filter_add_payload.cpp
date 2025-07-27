#include <neo/network/payloads/filter_add_payload.h>

namespace neo::network::payloads
{
FilterAddPayload::FilterAddPayload() = default;

FilterAddPayload::FilterAddPayload(const io::ByteVector& data) : data_(data) {}

const io::ByteVector& FilterAddPayload::GetData() const
{
    return data_;
}

void FilterAddPayload::SetData(const io::ByteVector& data)
{
    data_ = data;
}

void FilterAddPayload::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarBytes(data_.AsSpan());
}

void FilterAddPayload::Deserialize(io::BinaryReader& reader)
{
    data_ = reader.ReadVarBytes(520);  // Maximum size of a script
}

void FilterAddPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("data", data_);
}

void FilterAddPayload::DeserializeJson(const io::JsonReader& reader)
{
    data_ = reader.ReadByteVector("data");
}
}  // namespace neo::network::payloads
