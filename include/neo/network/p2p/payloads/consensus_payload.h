/**
 * @file consensus_payload.h
 * @brief Consensus Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/uint160.h>
#include <neo/network/ipayload.h>

#include <cstdint>

namespace neo::network::payloads
{
/**
 * @brief Represents a consensus payload.
 */
class ConsensusPayload : public IPayload, public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs an empty ConsensusPayload.
     */
    ConsensusPayload();

    /**
     * @brief Constructs a ConsensusPayload with the specified parameters.
     * @param version The version of the payload.
     * @param prevHash The previous block hash.
     * @param blockIndex The block index.
     * @param validatorIndex The validator index.
     * @param data The consensus data.
     */
    ConsensusPayload(uint32_t version, const io::UInt256& prevHash, uint32_t blockIndex, uint16_t validatorIndex,
                     const io::ByteVector& data);

    /**
     * @brief Gets the version.
     * @return The version.
     */
    uint32_t GetVersion() const;

    /**
     * @brief Sets the version.
     * @param version The version.
     */
    void SetVersion(uint32_t version);

    /**
     * @brief Gets the previous block hash.
     * @return The previous block hash.
     */
    const io::UInt256& GetPrevHash() const;

    /**
     * @brief Sets the previous block hash.
     * @param prevHash The previous block hash.
     */
    void SetPrevHash(const io::UInt256& prevHash);

    /**
     * @brief Gets the block index.
     * @return The block index.
     */
    uint32_t GetBlockIndex() const;

    /**
     * @brief Sets the block index.
     * @param blockIndex The block index.
     */
    void SetBlockIndex(uint32_t blockIndex);

    /**
     * @brief Gets the validator index.
     * @return The validator index.
     */
    uint16_t GetValidatorIndex() const;

    /**
     * @brief Sets the validator index.
     * @param validatorIndex The validator index.
     */
    void SetValidatorIndex(uint16_t validatorIndex);

    /**
     * @brief Gets the consensus data.
     * @return The consensus data.
     */
    const io::ByteVector& GetData() const;

    /**
     * @brief Sets the consensus data.
     * @param data The consensus data.
     */
    void SetData(const io::ByteVector& data);

    /**
     * @brief Serializes the ConsensusPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the ConsensusPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the ConsensusPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the ConsensusPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   private:
    uint32_t version_;
    io::UInt256 prevHash_;
    uint32_t blockIndex_;
    uint16_t validatorIndex_;
    io::ByteVector data_;
};
}  // namespace neo::network::payloads
