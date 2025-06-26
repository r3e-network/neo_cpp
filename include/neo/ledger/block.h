#pragma once

#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/iserializable.h>
#include <neo/ledger/transaction.h>
#include <vector>
#include <chrono>

namespace neo::ledger
{
    /**
     * @brief Represents a block in the Neo blockchain
     */
    class Block : public io::ISerializable
    {
    private:
        uint32_t version_{0};
        io::UInt256 previous_hash_;
        io::UInt256 merkle_root_;
        std::chrono::system_clock::time_point timestamp_;
        uint32_t index_{0};
        uint32_t primary_index_{0};
        io::UInt160 next_consensus_;
        std::vector<Transaction> transactions_;
        
    public:
        /**
         * @brief Default constructor
         */
        Block() = default;
        
        /**
         * @brief Get block version
         */
        uint32_t GetVersion() const { return version_; }
        
        /**
         * @brief Set block version
         */
        void SetVersion(uint32_t version) { version_ = version; }
        
        /**
         * @brief Get previous block hash
         */
        const io::UInt256& GetPreviousHash() const { return previous_hash_; }
        
        /**
         * @brief Set previous block hash
         */
        void SetPreviousHash(const io::UInt256& hash) { previous_hash_ = hash; }
        
        /**
         * @brief Get merkle root
         */
        const io::UInt256& GetMerkleRoot() const { return merkle_root_; }
        
        /**
         * @brief Set merkle root
         */
        void SetMerkleRoot(const io::UInt256& root) { merkle_root_ = root; }
        
        /**
         * @brief Get block timestamp
         */
        std::chrono::system_clock::time_point GetTimestamp() const { return timestamp_; }
        
        /**
         * @brief Set block timestamp
         */
        void SetTimestamp(std::chrono::system_clock::time_point timestamp) { timestamp_ = timestamp; }
        
        /**
         * @brief Get block index
         */
        uint32_t GetIndex() const { return index_; }
        
        /**
         * @brief Set block index
         */
        void SetIndex(uint32_t index) { index_ = index; }
        
        /**
         * @brief Get primary index
         */
        uint32_t GetPrimaryIndex() const { return primary_index_; }
        
        /**
         * @brief Set primary index
         */
        void SetPrimaryIndex(uint32_t index) { primary_index_ = index; }
        
        /**
         * @brief Get next consensus address
         */
        const io::UInt160& GetNextConsensus() const { return next_consensus_; }
        
        /**
         * @brief Set next consensus address
         */
        void SetNextConsensus(const io::UInt160& address) { next_consensus_ = address; }
        
        /**
         * @brief Get transactions
         */
        const std::vector<Transaction>& GetTransactions() const { return transactions_; }
        
        /**
         * @brief Add transaction
         */
        void AddTransaction(const Transaction& tx) { transactions_.push_back(tx); }
        
        /**
         * @brief Get block hash
         */
        io::UInt256 GetHash() const;
        
        /**
         * @brief Get block size
         */
        uint32_t GetSize() const;
        
        // ISerializable implementation
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
    };
}