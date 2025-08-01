#pragma once

#include <cstdint>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/ipayload.h>

namespace neo::network::payloads
{
/**
 * @brief Represents a filter add payload for SPV protocol.
 */
class FilterAddPayload : public IPayload, public io::IJsonSerializable
{
  public:
    /**
     * @brief Constructs an empty FilterAddPayload.
     */
    FilterAddPayload();

    /**
     * @brief Constructs a FilterAddPayload with the specified data.
     * @param data The data to add to the bloom filter.
     */
    explicit FilterAddPayload(const io::ByteVector& data);

    /**
     * @brief Gets the data.
     * @return The data.
     */
    const io::ByteVector& GetData() const;

    /**
     * @brief Sets the data.
     * @param data The data.
     */
    void SetData(const io::ByteVector& data);

    /**
     * @brief Serializes the FilterAddPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the FilterAddPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the FilterAddPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the FilterAddPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

  private:
    io::ByteVector data_;
};
}  // namespace neo::network::payloads
