#pragma once

#include <neo/network/ipayload.h>

namespace neo::network::payloads
{
    /**
     * @brief Represents a getaddr message payload.
     */
    class GetAddrPayload : public IPayload
    {
    public:
        /**
         * @brief Constructs a GetAddrPayload.
         */
        GetAddrPayload() = default;

        /**
         * @brief Serializes the GetAddrPayload to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the GetAddrPayload from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;
        
        /**
         * @brief Serializes the GetAddrPayload to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;
        
        /**
         * @brief Deserializes the GetAddrPayload from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;
    };
}
