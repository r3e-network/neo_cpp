/**
 * @file block.h
 * @brief Block structure and validation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/ledger/transaction.h>
#include <neo/network/p2p/payloads/header.h>

#include <memory>
#include <vector>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a block.
 * This matches the C# Neo.Network.P2P.Payloads.Block class exactly.
 */
class Block : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs an empty Block.
     */
    Block();

    /**
     * @brief Destructor.
     */
    ~Block() = default;

    /**
     * @brief Copy constructor.
     */
    Block(const Block& other);

    /**
     * @brief Assignment operator.
     */
    Block& operator=(const Block& other);

    /**
     * @brief Gets the header of the block.
     */
    std::shared_ptr<Header> GetHeader() const { return header_; }

    /**
     * @brief Sets the header of the block.
     */
    void SetHeader(std::shared_ptr<Header> header) { header_ = header; }

    /**
     * @brief Gets the transactions of the block.
     */
    const std::vector<std::shared_ptr<ledger::Transaction>>& GetTransactions() const { return transactions_; }

    /**
     * @brief Sets the transactions of the block.
     */
    void SetTransactions(const std::vector<std::shared_ptr<ledger::Transaction>>& transactions)
    {
        transactions_ = transactions;
    }

    // Convenience properties that delegate to header (matching C# structure)

    /**
     * @brief Gets the hash of the block.
     */
    io::UInt256 GetHash() const { return header_ ? header_->GetHash() : io::UInt256(); }

    /**
     * @brief Gets the version of the block.
     */
    uint32_t GetVersion() const { return header_ ? header_->GetVersion() : 0; }

    /**
     * @brief Gets the hash of the previous block.
     */
    io::UInt256 GetPrevHash() const { return header_ ? header_->GetPrevHash() : io::UInt256(); }

    /**
     * @brief Gets the merkle root of the transactions.
     */
    io::UInt256 GetMerkleRoot() const { return header_ ? header_->GetMerkleRoot() : io::UInt256(); }

    /**
     * @brief Gets the timestamp of the block.
     */
    uint64_t GetTimestamp() const { return header_ ? header_->GetTimestamp() : 0; }

    /**
     * @brief Gets the nonce of the block.
     */
    uint64_t GetNonce() const { return header_ ? header_->GetNonce() : 0; }

    /**
     * @brief Gets the index of the block.
     */
    uint32_t GetIndex() const { return header_ ? header_->GetIndex() : 0; }

    /**
     * @brief Gets the primary index of the consensus node.
     */
    uint8_t GetPrimaryIndex() const { return header_ ? header_->GetPrimaryIndex() : 0; }

    /**
     * @brief Gets the next consensus address.
     */
    io::UInt160 GetNextConsensus() const { return header_ ? header_->GetNextConsensus() : io::UInt160(); }

    /**
     * @brief Gets the witness of the block.
     */
    ledger::Witness GetWitness() const { return header_ ? header_->GetWitness() : ledger::Witness(); }

    /**
     * @brief Gets the witnesses of the block (as a vector for RPC compatibility).
     */
    std::vector<ledger::Witness> GetWitnesses() const
    {
        return header_ ? std::vector<ledger::Witness>{header_->GetWitness()} : std::vector<ledger::Witness>{};
    }

    /**
     * @brief Tries to get the hash (with error checking).
     */
    bool TryGetHash(io::UInt256& hash) const;

    /**
     * @brief Tries to get the hash (with error checking).
     */
    bool TryGetHash() const;

    /**
     * @brief Gets the size of the block in bytes.
     */
    size_t GetSize() const;

    /**
     * @brief Verifies the block.
     */
    bool Verify(std::shared_ptr<config::ProtocolSettings> settings,
                std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Verifies the block with header cache.
     */
    bool Verify(std::shared_ptr<config::ProtocolSettings> settings, std::shared_ptr<persistence::DataCache> snapshot,
                std::shared_ptr<HeaderCache> header_cache) const;

    /**
     * @brief Rebuilds the merkle root from transactions.
     */
    void RebuildMerkleRoot();

    // ISerializable implementation
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

    // IJsonSerializable implementation
    void SerializeJson(io::JsonWriter& writer) const override;
    void DeserializeJson(const io::JsonReader& reader) override;

    // Equality operators
    bool operator==(const Block& other) const;
    bool operator!=(const Block& other) const;

   private:
    // Block data (matching C# structure exactly)
    std::shared_ptr<Header> header_;
    std::vector<std::shared_ptr<ledger::Transaction>> transactions_;
};

}  // namespace neo::network::p2p::payloads