#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/ipayload.h>

#include <string>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a reject payload.
 *
 * This payload is used to indicate rejection of an inventory.
 * Note: Currently the Neo N3 protocol defines this message command
 * but doesn't specify a payload structure. This implementation
 * provides a base implementation that can be extended if needed.
 */
class RejectPayload : public IPayload
{
   public:
    /**
     * @brief Constructs an empty RejectPayload.
     */
    RejectPayload() = default;

    // IPayload implementation
    void Serialize(io::BinaryWriter& writer) const override
    {
        // Currently empty - no data to serialize
    }

    void Deserialize(io::BinaryReader& reader) override
    {
        // Currently empty - no data to deserialize
    }

    size_t GetSize() const
    {
        return 0;  // Empty payload
    }
    
    // IJsonSerializable implementation
    void SerializeJson(io::JsonWriter& writer) const override
    {
        // Currently empty - no data to serialize
    }
    
    void DeserializeJson(const io::JsonReader& reader) override
    {
        // Currently empty - no data to deserialize
    }
};
}  // namespace neo::network::p2p::payloads