#pragma once

#include <neo/network/ipayload.h>

namespace neo::network::payloads
{
    /**
     * @brief Represents a verack message payload.
     * 
     * This is an empty payload used to acknowledge a version message.
     */
    class VerAckPayload : public IPayload
    {
    public:
        /**
         * @brief Constructs a VerAckPayload.
         */
        VerAckPayload() = default;

        /**
         * @brief Serializes the VerAckPayload to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the VerAckPayload from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;
        
        /**
         * @brief Serializes the VerAckPayload to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;
        
        /**
         * @brief Deserializes the VerAckPayload from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;
    };
}
