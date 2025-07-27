#pragma once

#include <cstdint>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/ipayload.h>

namespace neo::network::payloads
{
/**
 * @brief Represents a get block by index payload.
 */
class GetBlockByIndexPayload : public IPayload, public io::IJsonSerializable
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
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the GetBlockByIndexPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

  private:
    uint32_t indexStart_;
    uint16_t count_;
};
}  // namespace neo::network::payloads
