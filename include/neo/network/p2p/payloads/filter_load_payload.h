#pragma once

#include <cstdint>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/ipayload.h>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a filter load payload for SPV protocol.
 *
 * This message is used to set a new bloom filter on the connection.
 */
class FilterLoadPayload : public IPayload
{
  public:
    /**
     * @brief Maximum filter size in bytes
     */
    static constexpr uint32_t MaxFilterSize = 36000;

    /**
     * @brief Constructs an empty FilterLoadPayload.
     */
    FilterLoadPayload();

    /**
     * @brief Constructs a FilterLoadPayload with the specified parameters.
     * @param filter The bloom filter data.
     * @param k The number of hash functions.
     * @param tweak A random value to add to the seed value.
     * @param flags The filter flags.
     */
    FilterLoadPayload(const io::ByteVector& filter, uint8_t k, uint32_t tweak, uint8_t flags);

    /**
     * @brief Gets the bloom filter data.
     * @return The bloom filter data.
     */
    const io::ByteVector& GetFilter() const;

    /**
     * @brief Sets the bloom filter data.
     * @param filter The bloom filter data.
     */
    void SetFilter(const io::ByteVector& filter);

    /**
     * @brief Gets the number of hash functions.
     * @return The number of hash functions.
     */
    uint8_t GetK() const;

    /**
     * @brief Sets the number of hash functions.
     * @param k The number of hash functions.
     */
    void SetK(uint8_t k);

    /**
     * @brief Gets the random tweak value.
     * @return The random tweak value.
     */
    uint32_t GetTweak() const;

    /**
     * @brief Sets the random tweak value.
     * @param tweak The random tweak value.
     */
    void SetTweak(uint32_t tweak);

    /**
     * @brief Gets the filter flags.
     * @return The filter flags.
     */
    uint8_t GetFlags() const;

    /**
     * @brief Sets the filter flags.
     * @param flags The filter flags.
     */
    void SetFlags(uint8_t flags);

    /**
     * @brief Serializes the FilterLoadPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the FilterLoadPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the FilterLoadPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const;

    /**
     * @brief Deserializes the FilterLoadPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader);

  private:
    io::ByteVector filter_;
    uint8_t k_;
    uint32_t tweak_;
    uint8_t flags_;
};
}  // namespace neo::network::p2p::payloads
