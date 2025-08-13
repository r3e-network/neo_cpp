/**
 * @file verack_payload.h
 * @brief Verack Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/network/p2p/ipayload.h>

namespace neo::network::p2p::payloads
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
    void SerializeJson(io::JsonWriter& writer) const;

    /**
     * @brief Deserializes the VerAckPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader);
};
}  // namespace neo::network::p2p::payloads
