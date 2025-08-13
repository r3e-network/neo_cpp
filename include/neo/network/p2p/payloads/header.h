/**
 * @file header.h
 * @brief Header
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/config/protocol_settings.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/witness.h>
#include <neo/persistence/data_cache.h>

#include <cstdint>
#include <memory>

namespace neo::network::p2p::payloads
{
// Forward declarations
class HeaderCache;

/**
 * @brief Represents the header of a block.
 * This matches the C# Neo.Network.P2P.Payloads.Header class exactly.
 */
class Header : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs an empty Header.
     */
    Header();

    /**
     * @brief Destructor.
     */
    ~Header() = default;

    /**
     * @brief Copy constructor.
     */
    Header(const Header& other);

    /**
     * @brief Assignment operator.
     */
    Header& operator=(const Header& other);

    /**
     * @brief Gets the version of the block.
     */
    uint32_t GetVersion() const { return version_; }

    /**
     * @brief Sets the version of the block.
     */
    void SetVersion(uint32_t version);

    /**
     * @brief Gets the hash of the previous block.
     */
    const io::UInt256& GetPrevHash() const { return prev_hash_; }

    /**
     * @brief Sets the hash of the previous block.
     */
    void SetPrevHash(const io::UInt256& prev_hash);

    /**
     * @brief Gets the merkle root of the transactions.
     */
    const io::UInt256& GetMerkleRoot() const { return merkle_root_; }

    /**
     * @brief Sets the merkle root of the transactions.
     */
    void SetMerkleRoot(const io::UInt256& merkle_root);

    /**
     * @brief Gets the timestamp of the block.
     */
    uint64_t GetTimestamp() const { return timestamp_; }

    /**
     * @brief Sets the timestamp of the block.
     */
    void SetTimestamp(uint64_t timestamp);

    /**
     * @brief Gets the nonce of the block.
     */
    uint64_t GetNonce() const { return nonce_; }

    /**
     * @brief Sets the nonce of the block.
     */
    void SetNonce(uint64_t nonce);

    /**
     * @brief Gets the index of the block.
     */
    uint32_t GetIndex() const { return index_; }

    /**
     * @brief Sets the index of the block.
     */
    void SetIndex(uint32_t index);

    /**
     * @brief Gets the primary index of the consensus node.
     */
    uint8_t GetPrimaryIndex() const { return primary_index_; }

    /**
     * @brief Sets the primary index of the consensus node.
     */
    void SetPrimaryIndex(uint8_t primary_index);

    /**
     * @brief Gets the next consensus address.
     */
    const io::UInt160& GetNextConsensus() const { return next_consensus_; }

    /**
     * @brief Sets the next consensus address.
     */
    void SetNextConsensus(const io::UInt160& next_consensus);

    /**
     * @brief Gets the witness of the block.
     */
    const ledger::Witness& GetWitness() const { return witness_; }

    /**
     * @brief Sets the witness of the block.
     */
    void SetWitness(const ledger::Witness& witness);

    /**
     * @brief Gets the hash of the header.
     */
    io::UInt256 GetHash() const;

    /**
     * @brief Tries to get the hash (with error checking).
     */
    bool TryGetHash(io::UInt256& hash) const;

    /**
     * @brief Tries to get the hash (with error checking).
     */
    bool TryGetHash() const;

    /**
     * @brief Gets the size of the header in bytes.
     */
    size_t GetSize() const;

    /**
     * @brief Verifies the header.
     */
    bool Verify(std::shared_ptr<config::ProtocolSettings> settings,
                std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Verifies the header with header cache.
     */
    bool Verify(std::shared_ptr<config::ProtocolSettings> settings, std::shared_ptr<persistence::DataCache> snapshot,
                std::shared_ptr<HeaderCache> header_cache) const;

    /**
     * @brief Creates a clone of this header.
     */
    std::shared_ptr<Header> Clone() const;

    // ISerializable implementation
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

    // IJsonSerializable implementation
    void SerializeJson(io::JsonWriter& writer) const override;
    void DeserializeJson(const io::JsonReader& reader) override;

    // Equality operators
    bool operator==(const Header& other) const;
    bool operator!=(const Header& other) const;

   private:
    /**
     * @brief Calculates and caches the hash.
     */
    void CalculateHash() const;

    /**
     * @brief Invalidates the cached hash.
     */
    void InvalidateHash();

    // Header fields (matching C# exactly)
    uint32_t version_;
    io::UInt256 prev_hash_;
    io::UInt256 merkle_root_;
    uint64_t timestamp_;
    uint64_t nonce_;
    uint32_t index_;
    uint8_t primary_index_;
    io::UInt160 next_consensus_;
    ledger::Witness witness_;

    // Cached hash (mutable for lazy calculation)
    mutable io::UInt256 hash_;
    mutable bool hash_calculated_;
};

}  // namespace neo::network::p2p::payloads