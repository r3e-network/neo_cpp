/**
 * @file get_block_by_index_payload.h
 * @brief Block structure and validation
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

#include <cstdint>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a get block by index payload.
 *
 * This message is used to request blocks by index.
 */
class GetBlockByIndexPayload : public IPayload
{
   public:
    /**
     * @brief Constructs a GetBlockByIndexPayload.
     */
    GetBlockByIndexPayload();

    /**
     * @brief Constructs a GetBlockByIndexPayload.
     * @param indexStart The start index.
     * @param count The number of blocks to retrieve.
     */
    GetBlockByIndexPayload(uint32_t indexStart, uint16_t count);

    /**
     * @brief Gets the start index.
     * @return The start index.
     */
    uint32_t GetIndexStart() const;

    /**
     * @brief Sets the start index.
     * @param indexStart The start index.
     */
    void SetIndexStart(uint32_t indexStart);

    /**
     * @brief Gets the count.
     * @return The count.
     */
    uint16_t GetCount() const;

    /**
     * @brief Sets the count.
     * @param count The count.
     */
    void SetCount(uint16_t count);

    /**
     * @brief Gets the size of the payload.
     * @return The size of the payload.
     */
    size_t GetSize() const;

    /**
     * @brief Creates a new GetBlockByIndexPayload with the specified parameters.
     * @param indexStart The start index.
     * @param count The number of blocks to retrieve.
     * @return The created payload.
     */
    static GetBlockByIndexPayload Create(uint32_t indexStart, uint16_t count = 500);

    /**
     * @brief Serializes the GetBlockByIndexPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the GetBlockByIndexPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the GetBlockByIndexPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const;

    /**
     * @brief Deserializes the GetBlockByIndexPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader);

   private:
    uint32_t indexStart_;
    uint16_t count_;
};
}  // namespace neo::network::p2p::payloads
