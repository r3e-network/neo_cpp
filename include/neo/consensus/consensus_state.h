#pragma once

#include <neo/io/uint256.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>

namespace neo::consensus
{
    /**
     * @brief Current phase of consensus
     */
    enum class ConsensusPhase
    {
        Initial,
        Primary,
        Backup,
        RequestSent,
        RequestReceived,
        SignatureSent,
        BlockSent,
        ViewChanging
    };

    /**
     * @brief State tracking for dBFT consensus
     */
    class ConsensusState
    {
    private:
        mutable std::mutex mutex_;
        
        // Basic state
        uint32_t view_number_{0};
        uint32_t block_index_{0};
        ConsensusPhase phase_{ConsensusPhase::Initial};
        
        // Current proposal
        io::UInt256 prepare_request_hash_;
        std::vector<network::p2p::payloads::Neo3Transaction> proposed_transactions_;
        std::chrono::system_clock::time_point timestamp_;
        uint64_t nonce_{0};
        
        // Tracking responses
        std::unordered_map<uint32_t, io::UInt256> prepare_responses_;  // validator_index -> hash
        std::unordered_map<uint32_t, std::vector<uint8_t>> commits_;   // validator_index -> signature
        std::unordered_set<uint32_t> view_changes_;                    // validator indices requesting view change
        
        // Transaction pool
        std::unordered_map<io::UInt256, network::p2p::payloads::Neo3Transaction> transaction_pool_;
        
    public:
        /**
         * @brief Get current view number
         */
        uint32_t GetViewNumber() const;
        
        /**
         * @brief Set view number
         */
        void SetViewNumber(uint32_t view);
        
        /**
         * @brief Get current block index
         */
        uint32_t GetBlockIndex() const;
        
        /**
         * @brief Set block index
         */
        void SetBlockIndex(uint32_t index);
        
        /**
         * @brief Get current consensus phase
         */
        ConsensusPhase GetPhase() const;
        
        /**
         * @brief Set consensus phase
         */
        void SetPhase(ConsensusPhase phase);
        
        /**
         * @brief Get prepare request hash
         */
        io::UInt256 GetPrepareRequestHash() const;
        
        /**
         * @brief Set prepare request details
         */
        void SetPrepareRequest(const io::UInt256& hash, 
                               const std::vector<network::p2p::payloads::Neo3Transaction>& txs,
                               std::chrono::system_clock::time_point timestamp,
                               uint64_t nonce);
        
        /**
         * @brief Add prepare response
         * @return true if this was a new response
         */
        bool AddPrepareResponse(uint32_t validator_index, const io::UInt256& hash);
        
        /**
         * @brief Get prepare response count
         */
        size_t GetPrepareResponseCount() const;
        
        /**
         * @brief Check if we have prepare response from validator
         */
        bool HasPrepareResponse(uint32_t validator_index) const;
        
        /**
         * @brief Add commit
         * @return true if this was a new commit
         */
        bool AddCommit(uint32_t validator_index, const std::vector<uint8_t>& signature);
        
        /**
         * @brief Get commit count
         */
        size_t GetCommitCount() const;
        
        /**
         * @brief Check if we have commit from validator
         */
        bool HasCommit(uint32_t validator_index) const;
        
        /**
         * @brief Add view change request
         * @return true if this was a new request
         */
        bool AddViewChange(uint32_t validator_index);
        
        /**
         * @brief Get view change count
         */
        size_t GetViewChangeCount() const;
        
        /**
         * @brief Add transaction to pool
         * @return true if transaction was added
         */
        bool AddTransaction(const network::p2p::payloads::Neo3Transaction& tx);
        
        /**
         * @brief Remove transaction from pool
         */
        void RemoveTransaction(const io::UInt256& hash);
        
        /**
         * @brief Get transactions for next block
         * @param max_count Maximum number of transactions
         * @return Selected transactions
         */
        std::vector<network::p2p::payloads::Neo3Transaction> GetTransactionsForBlock(size_t max_count) const;
        
        /**
         * @brief Get timestamp
         */
        std::chrono::system_clock::time_point GetTimestamp() const;
        
        /**
         * @brief Get nonce
         */
        uint64_t GetNonce() const;
        
        /**
         * @brief Get all transactions
         */
        std::vector<network::p2p::payloads::Neo3Transaction> GetTransactions() const;
        
        /**
         * @brief Get all commits for block creation
         */
        std::unordered_map<uint32_t, std::vector<uint8_t>> GetCommits() const;
        
        /**
         * @brief Reset state for new consensus round
         */
        void Reset();
        
        /**
         * @brief Reset for view change
         */
        void ResetForViewChange();
    };
}