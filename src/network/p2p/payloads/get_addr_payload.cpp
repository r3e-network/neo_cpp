#include <neo/network/p2p/payloads/get_addr_payload.h>

namespace neo::network::p2p::payloads
{
    void GetAddrPayload::Serialize(io::BinaryWriter& writer) const
    {
        // GetAddr message has no payload data
    }

    void GetAddrPayload::Deserialize(io::BinaryReader& reader)
    {
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
        // No data to read
    }
}
