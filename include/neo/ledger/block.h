#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/fixed8.h>
#include <neo/ledger/transaction.h>
#include <vector>
#include <memory>
#include <cstdint>

namespace neo::ledger
{
    /**
     * @brief Represents a block in the blockchain.
     */
    class Block : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty Block.
         */
        Block();

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
         * @brief Gets the merkle root.
         * @return The merkle root.
         */
        const io::UInt256& GetMerkleRoot() const;

        /**
         * @brief Sets the merkle root.
         * @param merkleRoot The merkle root.
         */
        void SetMerkleRoot(const io::UInt256& merkleRoot);

        /**
         * @brief Gets the timestamp.
         * @return The timestamp.
         */
        uint64_t GetTimestamp() const;

        /**
         * @brief Sets the timestamp.
         * @param timestamp The timestamp.
         */
        void SetTimestamp(uint64_t timestamp);

        /**
         * @brief Gets the nonce.
         * @return The nonce.
         */
        uint64_t GetNonce() const;

        /**
         * @brief Sets the nonce.
         * @param nonce The nonce.
         */
        void SetNonce(uint64_t nonce);

        /**
         * @brief Gets the index.
         * @return The index.
         */
        uint32_t GetIndex() const;

        /**
         * @brief Sets the index.
         * @param index The index.
         */
        void SetIndex(uint32_t index);

        /**
         * @brief Gets the primary index.
         * @return The primary index.
         */
        uint8_t GetPrimaryIndex() const;

        /**
         * @brief Sets the primary index.
         * @param primaryIndex The primary index.
         */
        void SetPrimaryIndex(uint8_t primaryIndex);

        /**
         * @brief Gets the next consensus.
         * @return The next consensus.
         */
        const io::UInt160& GetNextConsensus() const;

        /**
         * @brief Sets the next consensus.
         * @param nextConsensus The next consensus.
         */
        void SetNextConsensus(const io::UInt160& nextConsensus);

        /**
         * @brief Gets the witness.
         * @return The witness.
         */
        const Witness& GetWitness() const;

        /**
         * @brief Sets the witness.
         * @param witness The witness.
         */
        void SetWitness(const Witness& witness);

        /**
         * @brief Gets the transactions.
         * @return The transactions.
         */
        const std::vector<std::shared_ptr<Transaction>>& GetTransactions() const;

        /**
         * @brief Sets the transactions.
         * @param transactions The transactions.
         */
        void SetTransactions(const std::vector<std::shared_ptr<Transaction>>& transactions);

        /**
         * @brief Gets the hash of the block.
         * @return The hash of the block.
         */
        io::UInt256 GetHash() const;

        /**
         * @brief Gets the size of the block in bytes.
         * @return The size of the block in bytes.
         */
        size_t GetSize() const;

        /**
         * @brief Serializes the Block to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the Block from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the Block to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the Block from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Rebuilds the merkle root.
         */
        void RebuildMerkleRoot();

        /**
         * @brief Verifies the block.
         * @return True if the block is valid, false otherwise.
         */
        bool Verify() const;

        /**
         * @brief Verifies the witness.
         * @return True if the witness is valid, false otherwise.
         */
        bool VerifyWitness() const;

        /**
         * @brief Checks if this Block is equal to another Block.
         * @param other The other Block.
         * @return True if the Blocks are equal, false otherwise.
         */
        bool operator==(const Block& other) const;

        /**
         * @brief Checks if this Block is not equal to another Block.
         * @param other The other Block.
         * @return True if the Blocks are not equal, false otherwise.
         */
        bool operator!=(const Block& other) const;

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
        std::vector<std::shared_ptr<Transaction>> transactions_;
    };

    /**
     * @brief Represents a block header.
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
         * @param block The Block.
         */
        explicit BlockHeader(const Block& block);

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
         * @brief Gets the merkle root.
         * @return The merkle root.
         */
        const io::UInt256& GetMerkleRoot() const;

        /**
         * @brief Sets the merkle root.
         * @param merkleRoot The merkle root.
         */
        void SetMerkleRoot(const io::UInt256& merkleRoot);

        /**
         * @brief Gets the timestamp.
         * @return The timestamp.
         */
        uint64_t GetTimestamp() const;

        /**
         * @brief Sets the timestamp.
         * @param timestamp The timestamp.
         */
        void SetTimestamp(uint64_t timestamp);

        /**
         * @brief Gets the nonce.
         * @return The nonce.
         */
        uint64_t GetNonce() const;

        /**
         * @brief Sets the nonce.
         * @param nonce The nonce.
         */
        void SetNonce(uint64_t nonce);

        /**
         * @brief Gets the index.
         * @return The index.
         */
        uint32_t GetIndex() const;

        /**
         * @brief Sets the index.
         * @param index The index.
         */
        void SetIndex(uint32_t index);

        /**
         * @brief Gets the primary index.
         * @return The primary index.
         */
        uint8_t GetPrimaryIndex() const;

        /**
         * @brief Sets the primary index.
         * @param primaryIndex The primary index.
         */
        void SetPrimaryIndex(uint8_t primaryIndex);

        /**
         * @brief Gets the next consensus.
         * @return The next consensus.
         */
        const io::UInt160& GetNextConsensus() const;

        /**
         * @brief Sets the next consensus.
         * @param nextConsensus The next consensus.
         */
        void SetNextConsensus(const io::UInt160& nextConsensus);

        /**
         * @brief Gets the witness.
         * @return The witness.
         */
        const Witness& GetWitness() const;

        /**
         * @brief Sets the witness.
         * @param witness The witness.
         */
        void SetWitness(const Witness& witness);

        /**
         * @brief Gets the hash of the block header.
         * @return The hash of the block header.
         */
        io::UInt256 GetHash() const;

        /**
         * @brief Serializes the BlockHeader to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the BlockHeader from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the BlockHeader to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the BlockHeader from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Verifies the block header.
         * @return True if the block header is valid, false otherwise.
         */
        bool Verify() const;

        /**
         * @brief Verifies the witness.
         * @return True if the witness is valid, false otherwise.
         */
        bool VerifyWitness() const;

        /**
         * @brief Checks if this BlockHeader is equal to another BlockHeader.
         * @param other The other BlockHeader.
         * @return True if the BlockHeaders are equal, false otherwise.
         */
        bool operator==(const BlockHeader& other) const;

        /**
         * @brief Checks if this BlockHeader is not equal to another BlockHeader.
         * @param other The other BlockHeader.
         * @return True if the BlockHeaders are not equal, false otherwise.
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
    };
}
