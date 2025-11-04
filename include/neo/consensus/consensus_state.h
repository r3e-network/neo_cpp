/**
 * @file consensus_state.h
 * @brief Consensus State
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/consensus/consensus_message.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/ledger/transaction_verification_context.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    std::vector<io::UInt256> transaction_hashes_;
    std::chrono::system_clock::time_point timestamp_;
    uint64_t nonce_{0};
    std::shared_ptr<ledger::TransactionVerificationContext> verification_context_;
    size_t block_size_bytes_{0};
    int64_t total_system_fee_{0};
    int64_t total_network_fee_{0};

    // Tracking responses
    struct ViewChangeInfo
    {
        uint32_t original_view{0};
        uint32_t new_view{0};
        ChangeViewReason reason{ChangeViewReason::Timeout};
        uint64_t timestamp{0};
        io::ByteVector invocation_script;
    };

    struct PrepareResponseInfo
    {
        io::UInt256 hash;
        io::ByteVector invocation_script;
    };

    std::unordered_map<uint32_t, PrepareResponseInfo> prepare_responses_;  // validator_index -> info
    std::unordered_map<uint32_t, std::vector<uint8_t>> commits_;           // validator_index -> signature
    std::unordered_map<uint32_t, ViewChangeInfo> view_changes_;            // validator -> info

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
    void SetPrepareRequest(const io::UInt256& hash, const std::vector<network::p2p::payloads::Neo3Transaction>& txs,
                           const std::vector<io::UInt256>& transaction_hashes,
                           std::chrono::system_clock::time_point timestamp, uint64_t nonce,
                           std::shared_ptr<ledger::TransactionVerificationContext> verification_context,
                           size_t block_size_bytes, int64_t total_system_fee, int64_t total_network_fee);

    /**
     * @brief Add prepare response
     * @return true if this was a new response
     */
    bool AddPrepareResponse(uint32_t validator_index, const io::UInt256& hash,
                            const io::ByteVector& invocation_script);

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
    bool AddViewChange(uint32_t validator_index, uint32_t original_view, uint32_t new_view,
                       ChangeViewReason reason, const io::ByteVector& invocation_script, uint64_t timestamp_ms);

    /**
     * @brief Get view change count
     */
    size_t GetViewChangeCount() const;

    /**
     * @brief Get reason associated with a validator's view change request.
     */
    std::optional<ChangeViewReason> GetViewChangeReason(uint32_t validator_index) const;

    /**
     * @brief Count view changes proposing at least the supplied view number.
     */
    size_t CountViewChangesAtOrAbove(uint32_t view_number) const;

    /**
     * @brief Get the new view number requested by a validator, if known.
     */
    std::optional<uint32_t> GetViewChangeView(uint32_t validator_index) const;

    /**
     * @brief Retrieve all recorded view change info.
     */
    std::unordered_map<uint32_t, ViewChangeInfo> GetViewChanges() const;

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
     * @brief Retrieve a cached transaction if it exists in the local pool.
     */
    std::optional<network::p2p::payloads::Neo3Transaction> GetCachedTransaction(const io::UInt256& hash) const;

    /**
     * @brief Return a copy of the collected prepare responses keyed by validator index.
     */
    std::unordered_map<uint32_t, PrepareResponseInfo> GetPrepareResponses() const;

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
     * @brief Get transaction hashes associated with the current proposal.
     */
    std::vector<io::UInt256> GetTransactionHashes() const;

    /**
     * @brief Get verification context used during proposal validation.
     */
    std::shared_ptr<ledger::TransactionVerificationContext> GetVerificationContext() const;

    /**
     * @brief Get total serialized block size produced by the current proposal.
     */
    size_t GetBlockSizeBytes() const;

    /**
     * @brief Get aggregate system fee for the current proposal.
     */
    int64_t GetTotalSystemFee() const;

    /**
     * @brief Get aggregate network fee for the current proposal.
     */
    int64_t GetTotalNetworkFee() const;

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
}  // namespace neo::consensus
