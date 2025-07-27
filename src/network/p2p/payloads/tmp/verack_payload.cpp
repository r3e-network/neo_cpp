#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/payloads/verack_payload.h>

namespace neo::network::payloads
{
void VerAckPayload::Serialize(io::BinaryWriter& writer) const
{
    // VerAck has no payload data
}

void VerAckPayload::Deserialize(io::BinaryReader& reader)
{
    // VerAck has no payload data
}

void VerAckPayload::SerializeJson(io::JsonWriter& writer) const
{
    // VerAck has no fields to serialize
    writer.WriteStartObject();
    writer.WriteEndObject();
}

void VerAckPayload::DeserializeJson(const io::JsonReader& reader)
{
    // VerAck has no fields to deserialize
    reader.ReadStartObject();
    reader.ReadEndObject();
}
}  // namespace neo::network::payloads
