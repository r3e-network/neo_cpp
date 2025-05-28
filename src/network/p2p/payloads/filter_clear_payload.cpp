#include <neo/network/p2p/payloads/filter_clear_payload.h>

namespace neo::network::p2p::payloads
{
    void FilterClearPayload::Serialize(io::BinaryWriter& writer) const
    {
        // FilterClear has no payload data
    }

    void FilterClearPayload::Deserialize(io::BinaryReader& reader)
    {
        // FilterClear has no payload data
    }
    
    void FilterClearPayload::SerializeJson(io::JsonWriter& writer) const
    {
        // Empty object since there's no data
        writer.WriteStartObject();
        writer.WriteEndObject();
    }
    
    void FilterClearPayload::DeserializeJson(const io::JsonReader& reader)
    {
        // No data to read
        reader.ReadStartObject();
        reader.ReadEndObject();
    }
}
