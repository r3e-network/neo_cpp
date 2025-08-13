/**
 * @file block.h
 * @brief Block structure and validation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block_header.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/witness.h>

#include <chrono>
#include <vector>

namespace neo::ledger
{
/**
 * @brief Represents a block in the Neo blockchain
 */
class Block : public io::ISerializable
{
   private:
    BlockHeader header_;
    std::vector<Transaction> transactions_;
    mutable io::UInt256 hash_;  // Cached hash
    mutable bool hash_calculated_{false};

   public:
    /**
     * @brief Default constructor
     */
    Block() = default;

    /**
     * @brief Virtual destructor
     */
    virtual ~Block() = default;

    /**
     * @brief Get the header of the block
     */
    const BlockHeader& GetHeader() const { return header_; }

    /**
     * @brief Get the header of the block (non-const)
     */
    BlockHeader& GetHeader()
    {
        hash_calculated_ = false;
        return header_;
    }

    /**
     * @brief Set the header of the block
     */
    void SetHeader(const BlockHeader& header)
    {
        header_ = header;
        hash_calculated_ = false;
    }

    /**
     * @brief Get block version (delegates to header)
     */
    uint32_t GetVersion() const { return header_.GetVersion(); }

    /**
     * @brief Set block version (delegates to header)
     */
    void SetVersion(uint32_t version)
    {
        header_.SetVersion(version);
        hash_calculated_ = false;
    }

    /**
     * @brief Get previous block hash (delegates to header)
     */
    const io::UInt256& GetPreviousHash() const { return header_.GetPrevHash(); }
    const io::UInt256& GetPrevHash() const { return GetPreviousHash(); }  // Alias for compatibility

    /**
     * @brief Set previous block hash (delegates to header)
     */
    void SetPreviousHash(const io::UInt256& hash)
    {
        header_.SetPrevHash(hash);
        hash_calculated_ = false;
    }

    /**
     * @brief Get merkle root (delegates to header)
     */
    const io::UInt256& GetMerkleRoot() const { return header_.GetMerkleRoot(); }

    /**
     * @brief Set merkle root (delegates to header)
     */
    void SetMerkleRoot(const io::UInt256& root)
    {
        header_.SetMerkleRoot(root);
        hash_calculated_ = false;
    }

    /**
     * @brief Get block timestamp (delegates to header)
     */
    uint64_t GetTimestamp() const { return header_.GetTimestamp(); }

    /**
     * @brief Set block timestamp (delegates to header)
     */
    void SetTimestamp(uint64_t timestamp)
    {
        header_.SetTimestamp(timestamp);
        hash_calculated_ = false;
    }

    /**
     * @brief Get block index (delegates to header)
     */
    uint32_t GetIndex() const { return header_.GetIndex(); }

    /**
     * @brief Set block index (delegates to header)
     */
    void SetIndex(uint32_t index)
    {
        header_.SetIndex(index);
        hash_calculated_ = false;
    }

    /**
     * @brief Get primary index (delegates to header)
     */
    uint32_t GetPrimaryIndex() const { return static_cast<uint32_t>(header_.GetPrimaryIndex()); }

    /**
     * @brief Set primary index (delegates to header)
     */
    void SetPrimaryIndex(uint32_t index)
    {
        header_.SetPrimaryIndex(static_cast<uint8_t>(index));
        hash_calculated_ = false;
    }

    /**
     * @brief Get next consensus address (delegates to header)
     */
    const io::UInt160& GetNextConsensus() const { return header_.GetNextConsensus(); }

    /**
     * @brief Set next consensus address (delegates to header)
     */
    void SetNextConsensus(const io::UInt160& address)
    {
        header_.SetNextConsensus(address);
        hash_calculated_ = false;
    }

    /**
     * @brief Get transactions
     */
    const std::vector<Transaction>& GetTransactions() const { return transactions_; }

    /**
     * @brief Add transaction
     */
    void AddTransaction(const Transaction& tx) { transactions_.push_back(tx); }

    /**
     * @brief Get nonce (delegates to header)
     */
    uint64_t GetNonce() const { return header_.GetNonce(); }

    /**
     * @brief Set nonce (delegates to header)
     */
    void SetNonce(uint64_t nonce)
    {
        header_.SetNonce(nonce);
        hash_calculated_ = false;
    }

    /**
     * @brief Get witness (delegates to header)
     */
    const Witness& GetWitness() const { return header_.GetWitness(); }

    /**
     * @brief Set witness (delegates to header)
     */
    void SetWitness(const Witness& witness)
    {
        header_.SetWitness(witness);
        hash_calculated_ = false;
    }

    /**
     * @brief Get block hash
     */
    io::UInt256 GetHash() const;

    /**
     * @brief Calculate hash (force recalculation)
     */
    io::UInt256 CalculateHash() const;

    /**
     * @brief Update hash (force recalculation and cache)
     */
    void UpdateHash()
    {
        hash_ = CalculateHash();
        hash_calculated_ = true;
    }

    /**
     * @brief Get block size
     */
    uint32_t GetSize() const;

    /**
     * @brief Compute and return the Merkle root of transactions
     */
    io::UInt256 ComputeMerkleRoot() const;

    /**
     * @brief Verify witness signatures
     */
    bool VerifyWitnesses() const;

    // ISerializable implementation
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;
};
}  // namespace neo::ledger