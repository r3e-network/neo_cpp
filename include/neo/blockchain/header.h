#pragma once

#include <cstdint>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/witness.h>

namespace neo::blockchain
{
/**
 * @brief Represents the header of a block.
 */
class Header : public io::ISerializable, public io::IJsonSerializable
{
  public:
    /**
     * @brief Constructs an empty Header.
     */
    Header();

    /**
     * @brief Gets the version of the block.
     * @return The version of the block.
     */
    uint32_t GetVersion() const;

    /**
     * @brief Sets the version of the block.
     * @param version The version of the block.
     */
    void SetVersion(uint32_t version);

    /**
     * @brief Gets the hash of the previous block.
     * @return The hash of the previous block.
     */
    const io::UInt256& GetPrevHash() const;

    /**
     * @brief Sets the hash of the previous block.
     * @param prevHash The hash of the previous block.
     */
    void SetPrevHash(const io::UInt256& prevHash);

    /**
     * @brief Gets the merkle root of the transactions.
     * @return The merkle root of the transactions.
     */
    const io::UInt256& GetMerkleRoot() const;

    /**
     * @brief Sets the merkle root of the transactions.
     * @param merkleRoot The merkle root of the transactions.
     */
    void SetMerkleRoot(const io::UInt256& merkleRoot);

    /**
     * @brief Gets the timestamp of the block.
     * @return The timestamp of the block.
     */
    uint64_t GetTimestamp() const;

    /**
     * @brief Sets the timestamp of the block.
     * @param timestamp The timestamp of the block.
     */
    void SetTimestamp(uint64_t timestamp);

    /**
     * @brief Gets the nonce of the block.
     * @return The nonce of the block.
     */
    uint64_t GetNonce() const;

    /**
     * @brief Sets the nonce of the block.
     * @param nonce The nonce of the block.
     */
    void SetNonce(uint64_t nonce);

    /**
     * @brief Gets the index of the block.
     * @return The index of the block.
     */
    uint32_t GetIndex() const;

    /**
     * @brief Sets the index of the block.
     * @param index The index of the block.
     */
    void SetIndex(uint32_t index);

    /**
     * @brief Gets the primary index of the consensus node that generated this block.
     * @return The primary index of the consensus node.
     */
    uint8_t GetPrimaryIndex() const;

    /**
     * @brief Sets the primary index of the consensus node that generated this block.
     * @param primaryIndex The primary index of the consensus node.
     */
    void SetPrimaryIndex(uint8_t primaryIndex);

    /**
     * @brief Gets the multi-signature address of the consensus nodes that generates the next block.
     * @return The multi-signature address of the consensus nodes.
     */
    const io::UInt160& GetNextConsensus() const;

    /**
     * @brief Sets the multi-signature address of the consensus nodes that generates the next block.
     * @param nextConsensus The multi-signature address of the consensus nodes.
     */
    void SetNextConsensus(const io::UInt160& nextConsensus);

    /**
     * @brief Gets the witness of the block.
     * @return The witness of the block.
     */
    const ledger::Witness& GetWitness() const;

    /**
     * @brief Sets the witness of the block.
     * @param witness The witness of the block.
     */
    void SetWitness(const ledger::Witness& witness);

    /**
     * @brief Gets the hash of the block.
     * @return The hash of the block.
     */
    io::UInt256 GetHash() const;

    /**
     * @brief Serializes the header to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the header from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the header to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the header from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

  private:
    uint32_t version_;
    io::UInt256 prevHash_;
    io::UInt256 merkleRoot_;
    uint64_t timestamp_;
    uint64_t nonce_;
    uint32_t index_;
    uint8_t primaryIndex_;
    io::UInt160 nextConsensus_;
    ledger::Witness witness_;
    mutable io::UInt256 hash_;
};
}  // namespace neo::blockchain
