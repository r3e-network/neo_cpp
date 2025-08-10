#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/ipayload.h>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a mempool payload.
 *
 * This message is used to request the memory pool of transactions.
 */
class MempoolPayload : public IPayload
{
   public:
    /**
     * @brief Constructs a MempoolPayload.
     */
    MempoolPayload() = default;

    /**
     * @brief Serializes the MempoolPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the MempoolPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the MempoolPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const;

    /**
     * @brief Deserializes the MempoolPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader);
};
}  // namespace neo::network::p2p::payloads
