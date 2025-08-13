/**
 * @file get_addr_payload.cpp
 * @brief Get Addr Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p/payloads/get_addr_payload.h>

namespace neo::network::p2p::payloads
{
void GetAddrPayload::Serialize(io::BinaryWriter& writer) const
{
    (void)writer;  // Suppress unused parameter warning
    // GetAddr message has no payload data
}

void GetAddrPayload::Deserialize(io::BinaryReader& reader)
{
    (void)reader;  // Suppress unused parameter warning
    // GetAddr message has no payload data
}

void GetAddrPayload::SerializeJson(io::JsonWriter& writer) const
{
    // Empty object since there's no data
    writer.WriteStartObject();
    writer.WriteEndObject();
}

void GetAddrPayload::DeserializeJson(const io::JsonReader& reader)
{
    (void)reader;  // Suppress unused parameter warning
    // No data to read
}
}  // namespace neo::network::p2p::payloads
