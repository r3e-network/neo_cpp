#include <neo/network/p2p/payloads/mempool_payload.h>

namespace neo::network::p2p::payloads
{
    void MempoolPayload::Serialize([[maybe_unused]] io::BinaryWriter& writer) const
    {
        // Mempool message has no payload data
    }

    void MempoolPayload::Deserialize([[maybe_unused]] io::BinaryReader& reader)
    {
        // Mempool has no payload data
    }
    
    void MempoolPayload::SerializeJson(io::JsonWriter& writer) const
    {
        // Empty object since there's no data
        writer.WriteStartObject();
        writer.WriteEndObject();
    }
    
    void MempoolPayload::DeserializeJson(const io::JsonReader& reader)
    {
        // No data to read
        reader.ReadStartObject();
        reader.ReadEndObject();
    }
}
