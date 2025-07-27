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
 * @brief Represents a ping payload.
 */
class PingPayload : public IPayload, public io::IJsonSerializable
{
  public:
    /**
     * @brief Constructs a PingPayload.
     */
    PingPayload();

    /**
     * @brief Constructs a PingPayload.
     * @param lastBlockIndex The latest block index.
     * @param timestamp The current timestamp.
     * @param nonce The random nonce.
     */
    PingPayload(uint32_t lastBlockIndex, uint32_t timestamp, uint32_t nonce);

    /**
     * @brief Gets the latest block index.
     * @return The latest block index.
     */
    uint32_t GetLastBlockIndex() const;

    /**
     * @brief Sets the latest block index.
     * @param lastBlockIndex The latest block index.
     */
    void SetLastBlockIndex(uint32_t lastBlockIndex);

    /**
     * @brief Gets the timestamp.
     * @return The timestamp.
     */
    uint32_t GetTimestamp() const;

    /**
     * @brief Sets the timestamp.
     * @param timestamp The timestamp.
     */
    void SetTimestamp(uint32_t timestamp);

    /**
     * @brief Gets the nonce.
     * @return The nonce.
     */
    uint32_t GetNonce() const;

    /**
     * @brief Sets the nonce.
     * @param nonce The nonce.
     */
    void SetNonce(uint32_t nonce);

    /**
     * @brief Gets the size of the payload in bytes.
     * @return The size of the payload.
     */
    uint32_t GetSize() const;

    /**
     * @brief Creates a new PingPayload with the specified height.
     * @param height The latest block height.
     * @return The created PingPayload.
     */
    static PingPayload Create(uint32_t height);

    /**
     * @brief Creates a new PingPayload with the specified height and nonce.
     * @param height The latest block height.
     * @param nonce The random nonce.
     * @return The created PingPayload.
     */
    static PingPayload Create(uint32_t height, uint32_t nonce);

    /**
     * @brief Serializes the PingPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the PingPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the PingPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the PingPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

  private:
    uint32_t lastBlockIndex_;
    uint32_t timestamp_;
    uint32_t nonce_;
};
}  // namespace neo::network::payloads
