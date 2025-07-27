#pragma once

#include <cstdint>
#include <memory>
#include <neo/blockchain/header.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/uint256.h>
#include <neo/network/ipayload.h>
#include <vector>

namespace neo::network::payloads
{
/**
 * @brief Represents a merkle block payload for SPV protocol.
 */
class MerkleBlockPayload : public IPayload
{
  public:
    /**
     * @brief Constructs an empty MerkleBlockPayload.
     */
    MerkleBlockPayload();

    /**
     * @brief Constructs a MerkleBlockPayload with the specified parameters.
     * @param header The block header.
     * @param transactionCount The total number of transactions in the block.
     * @param hashes The transaction hashes that match the filter.
     * @param flags The flags bits, which indicate matches in the merkle tree.
     */
    MerkleBlockPayload(std::shared_ptr<blockchain::Header> header, uint32_t transactionCount,
                       const std::vector<io::UInt256>& hashes, const io::ByteVector& flags);

    /**
     * @brief Gets the block header.
     * @return The block header.
     */
    std::shared_ptr<blockchain::Header> GetHeader() const;

    /**
     * @brief Sets the block header.
     * @param header The block header.
     */
    void SetHeader(std::shared_ptr<blockchain::Header> header);

    /**
     * @brief Gets the transaction count.
     * @return The transaction count.
     */
    uint32_t GetTransactionCount() const;

    /**
     * @brief Sets the transaction count.
     * @param count The transaction count.
     */
    void SetTransactionCount(uint32_t count);

    /**
     * @brief Gets the transaction hashes.
     * @return The transaction hashes.
     */
    const std::vector<io::UInt256>& GetHashes() const;

    /**
     * @brief Sets the transaction hashes.
     * @param hashes The transaction hashes.
     */
    void SetHashes(const std::vector<io::UInt256>& hashes);

    /**
     * @brief Gets the flags bits.
     * @return The flags bits.
     */
    const io::ByteVector& GetFlags() const;

    /**
     * @brief Sets the flags bits.
     * @param flags The flags bits.
     */
    void SetFlags(const io::ByteVector& flags);

    /**
     * @brief Serializes the MerkleBlockPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the MerkleBlockPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the MerkleBlockPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the MerkleBlockPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

  private:
    std::shared_ptr<blockchain::Header> header_;
    uint32_t transactionCount_;
    std::vector<io::UInt256> hashes_;
    io::ByteVector flags_;
};
}  // namespace neo::network::payloads
