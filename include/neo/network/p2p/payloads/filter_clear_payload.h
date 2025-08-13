/**
 * @file filter_clear_payload.h
 * @brief Filter Clear Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/ipayload.h>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a filter clear payload for SPV protocol.
 *
 * This message is used to clear the bloom filter.
 */
class FilterClearPayload : public IPayload
{
   public:
    /**
     * @brief Constructs a FilterClearPayload.
     */
    FilterClearPayload() = default;

    /**
     * @brief Serializes the FilterClearPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the FilterClearPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the FilterClearPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const;

    /**
     * @brief Deserializes the FilterClearPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader);
};
}  // namespace neo::network::p2p::payloads
