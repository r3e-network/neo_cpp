#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/witness.h>
#include <cstdint>

namespace neo::ledger
{
    class Block;

    /**
     * @brief Represents a block header in the Neo blockchain.
     */
    class BlockHeader : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty BlockHeader.
         */
        BlockHeader();

        /**
         * @brief Constructs a BlockHeader from a Block.
         * @param block The block.
         */
        explicit BlockHeader(const Block& block);

        /**
         * @brief Gets the version of the block header.
         * @return The version.
         */
        uint32_t GetVersion() const;

        /**
         * @brief Sets the version of the block header.
         * @param version The version.
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
         * @return The merkle root.
         */
        const io::UInt256& GetMerkleRoot() const;

        /**
         * @brief Sets the merkle root of the transactions.
         * @param merkleRoot The merkle root.
         */
        void SetMerkleRoot(const io::UInt256& merkleRoot);

        /**
         * @brief Gets the timestamp of the block header.
         * @return The timestamp.
         */
        uint64_t GetTimestamp() const;

        /**
         * @brief Sets the timestamp of the block header.
         * @param timestamp The timestamp.
         */
        void SetTimestamp(uint64_t timestamp);

        /**
         * @brief Gets the nonce of the block header.
         * @return The nonce.
         */
        uint64_t GetNonce() const;

        /**
         * @brief Sets the nonce of the block header.
         * @param nonce The nonce.
         */
        void SetNonce(uint64_t nonce);

        /**
         * @brief Gets the index of the block header.
         * @return The index.
         */
        uint32_t GetIndex() const;

        /**
         * @brief Sets the index of the block header.
         * @param index The index.
         */
        void SetIndex(uint32_t index);

        /**
         * @brief Gets the primary index of the consensus node.
         * @return The primary index.
         */
        uint8_t GetPrimaryIndex() const;

        /**
         * @brief Sets the primary index of the consensus node.
         * @param primaryIndex The primary index.
         */
        void SetPrimaryIndex(uint8_t primaryIndex);

        /**
         * @brief Gets the next consensus address.
         * @return The next consensus address.
         */
        const io::UInt160& GetNextConsensus() const;

        /**
         * @brief Sets the next consensus address.
         * @param nextConsensus The next consensus address.
         */
        void SetNextConsensus(const io::UInt160& nextConsensus);

        /**
         * @brief Gets the witness of the block header.
         * @return The witness.
         */
        const Witness& GetWitness() const;

        /**
         * @brief Sets the witness of the block header.
         * @param witness The witness.
         */
        void SetWitness(const Witness& witness);

        /**
         * @brief Gets the hash of the block header.
         * @return The hash.
         */
        io::UInt256 GetHash() const;

        /**
         * @brief Gets the size of the block header.
         * @return The size in bytes.
         */
        int GetSize() const;

        /**
         * @brief Verifies the block header.
         * @return True if the block header is valid, false otherwise.
         */
        bool Verify() const;

        /**
         * @brief Verifies the witness of the block header.
         * @return True if the witness is valid, false otherwise.
         */
        bool VerifyWitness() const;

        /**
         * @brief Serializes the block header to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the block header from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the block header to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the block header from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Equality operator.
         * @param other The other block header.
         * @return True if the block headers are equal, false otherwise.
         */
        bool operator==(const BlockHeader& other) const;

        /**
         * @brief Inequality operator.
         * @param other The other block header.
         * @return True if the block headers are not equal, false otherwise.
         */
        bool operator!=(const BlockHeader& other) const;

    private:
        uint32_t version_;
        io::UInt256 prevHash_;
        io::UInt256 merkleRoot_;
        uint64_t timestamp_;
        uint64_t nonce_;
        uint32_t index_;
        uint8_t primaryIndex_;
        io::UInt160 nextConsensus_;
        Witness witness_;

        // Helper methods for witness verification
        bool IsMultiSignatureContract(const io::ByteVector& script) const;
        bool VerifyMultiSignatureWitness(const Witness& witness) const;
        io::ByteVector GetSignData() const;
    };
}
