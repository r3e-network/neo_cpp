/**
 * @file dbft_consensus.h
 * @brief Dbft Consensus
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/ledger/witness.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
// Using Neo N3 transactions for consensus
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/consensus_state.h>
#include <neo/consensus/recovery_message.h>
#include <neo/core/logging.h>
#include <neo/cryptography/ecc/ecpoint.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <thread>
#include <vector>

namespace neo::consensus
{
/**
 * @brief Configuration for dBFT consensus
 */
struct ConsensusConfig
{
    std::chrono::milliseconds block_time{15000};      // Target block time (15 seconds)
    std::chrono::milliseconds view_timeout{60000};    // View change timeout (60 seconds)
    uint32_t max_transactions_per_block{512};         // Maximum transactions per block
    uint32_t max_block_size{262144};                  // Maximum block size (256KB)
    uint64_t max_block_system_fee{9000000000000ULL};  // Maximum system fee per block
    bool is_active{true};                             // Whether this node participates in consensus
};

/**
 * @brief dBFT (Delegated Byzantine Fault Tolerance) consensus implementation
 *
 * This implements the Neo consensus algorithm which provides:
 * - Byzantine fault tolerance up to f = (n-1)/3 faulty nodes
 * - Single block finality
 * - Deterministic block production
 */
class DbftConsensus
{
    friend class ConsensusService;
   public:
    using SignatureProvider = std::function<io::ByteVector(const io::ByteSpan&)>;
    using TransactionVerifier = std::function<bool(const network::p2p::payloads::Neo3Transaction&)>;
    using BlockPersister = std::function<bool(const std::shared_ptr<ledger::Block>&)>;
    using MessageBroadcaster = std::function<void(const ConsensusMessage&)>;

   private:
    ConsensusConfig config_;
    std::shared_ptr<ConsensusState> state_;
    std::shared_ptr<core::Logger> logger_;

    // Callbacks
    TransactionVerifier tx_verifier_;
    BlockPersister block_persister_;
    MessageBroadcaster message_broadcaster_;

    // Threading
    std::atomic<bool> running_{false};
    std::thread consensus_thread_;
    std::thread timer_thread_;

    // Node identity
    io::UInt160 node_id_;
    uint32_t validator_index_;
    std::vector<io::UInt160> validators_;
    SignatureProvider signature_provider_;
    std::unordered_map<io::UInt160, cryptography::ecc::ECPoint> validator_public_keys_;
    std::atomic<bool> missing_signature_warning_emitted_{false};

    // Timing
    std::chrono::steady_clock::time_point view_started_;
    std::chrono::steady_clock::time_point last_block_time_;
    std::chrono::steady_clock::time_point last_commit_relay_;

    // Consensus message tracking
    std::map<uint32_t, std::shared_ptr<CommitMessage>> commit_messages_;
    std::unordered_map<uint32_t, io::ByteVector> commit_invocation_scripts_;
    uint32_t last_broadcast_change_view_{0};

    // External dependencies
    std::shared_ptr<ledger::MemoryPool> mempool_;
    std::shared_ptr<ledger::Blockchain> blockchain_;

   public:
    /**
     * @brief Construct a new dBFT consensus instance
     * @param config Consensus configuration
     * @param node_id This node's validator public key hash
     * @param validators List of all validator public key hashes
     * @param mempool Memory pool for transaction management
     * @param blockchain Blockchain reference for block data
     */
    DbftConsensus(const ConsensusConfig& config, const io::UInt160& node_id, const std::vector<io::UInt160>& validators,
                  std::shared_ptr<ledger::MemoryPool> mempool, std::shared_ptr<ledger::Blockchain> blockchain);

    ~DbftConsensus();

    /**
     * @brief Start consensus operations
     */
    void Start();

    /**
     * @brief Stop consensus operations
     */
    void Stop();

    /**
     * @brief Returns true if the consensus engine threads are running.
     */
    bool IsRunning() const { return running_.load(); }

    /**
     * @brief Primary index for the current view.
     */
    uint32_t GetPrimaryIndex() const;

    /**
     * @brief Validator index of this node.
     */
    uint32_t GetValidatorIndex() const { return validator_index_; }

    /**
     * @brief Process incoming consensus message
     * @param message The consensus message to process
     * @return true if message was processed successfully
     */
    bool ProcessMessage(const ConsensusMessage& message);

    /**
     * @brief Add transaction to memory pool for inclusion in next block
     * @param tx Transaction to add
     * @return true if transaction was added successfully
     */
    bool AddTransaction(const network::p2p::payloads::Neo3Transaction& tx);
    void RemoveCachedTransaction(const io::UInt256& hash);

    /**
     * @brief Get current consensus state
     * @return Current state
     */
    const ConsensusState& GetState() const;

    /**
     * @brief Set transaction verifier callback
     * @param verifier Callback to verify transactions
     */
    void SetTransactionVerifier(TransactionVerifier verifier) { tx_verifier_ = verifier; }

    /**
     * @brief Inject validator public keys corresponding to configured script hashes.
     * @param public_keys Validator ECPoints ordered by validator index.
     */
    void SetValidatorPublicKeys(const std::vector<cryptography::ecc::ECPoint>& public_keys);

    /**
     * @brief Provide a signing delegate used for commit payload signatures.
     * @param provider Delegate that signs the supplied message hash.
     */
    void SetSignatureProvider(SignatureProvider provider);

    /**
     * @brief Set block persister callback
     * @param persister Callback to persist blocks
     */
    void SetBlockPersister(BlockPersister persister) { block_persister_ = persister; }

    /**
     * @brief Set message broadcaster callback
     * @param broadcaster Callback to broadcast messages
     */
    void SetMessageBroadcaster(MessageBroadcaster broadcaster) { message_broadcaster_ = broadcaster; }

   private:
    /**
     * @brief Main consensus loop
     */
    void ConsensusLoop();

    /**
     * @brief Timer loop for timeouts
     */
    void TimerLoop();

    /**
     * @brief Start a new consensus round
     */
    void StartNewRound();

    /**
     * @brief Check if this node is the primary for current view
     * @return true if this node is primary
     */
    bool IsPrimary() const;

    /**
     * @brief Check if this node is watch-only (not part of validator set)
     * @return true if watch-only
     */
    bool IsWatchOnly() const;

    /**
     * @brief Get the primary node index for given view
     * @param view_number View number
     * @return Primary node index
     */
    uint32_t ComputePrimaryIndex(uint32_t view_number) const;

    /**
     * @brief Create and broadcast prepare request (primary only)
     */
    void SendPrepareRequest();

    /**
     * @brief Process prepare request message
     * @param message Prepare request message
     */
    void ProcessPrepareRequest(const PrepareRequestMessage& message);

    /**
     * @brief Send prepare response for current proposal
     */
    void SendPrepareResponse();

    /**
     * @brief Process prepare response message
     * @param message Prepare response message
     */
    void ProcessPrepareResponse(const PrepareResponseMessage& message);

    /**
     * @brief Send commit message when prepared
     */
    void SendCommit();

    /**
     * @brief Process commit message
     * @param message Commit message
     */
    void ProcessCommit(const CommitMessage& message);

    /**
     * @brief Request view change
     */
    void RequestViewChange(ChangeViewReason reason = ChangeViewReason::Timeout);

    /**
     * @brief Process view change message
     * @param message View change message
     */
    void ProcessViewChange(const ViewChangeMessage& message);
    void BroadcastChangeView(uint32_t new_view, ChangeViewReason reason);
    void EvaluateExpectedView(uint32_t target_view);
    std::shared_ptr<RecoveryMessage> BuildRecoveryMessage() const;
    void RecordSentPayload(const ConsensusMessage& message, const ledger::Witness& witness);

    /**
     * @brief Check if we have enough prepare responses
     * @return true if we have 2f+1 prepare responses
     */
    bool HasEnoughPrepareResponses() const;

    /**
     * @brief Check if we have enough commits
     * @return true if we have 2f+1 commits
     */
    bool HasEnoughCommits() const;

    /**
     * @brief Calculate f value (maximum Byzantine nodes)
     * @return f = (n-1)/3
     */
    uint32_t GetF() const;

    /**
     * @brief Calculate M value (minimum required signatures)
     * @return M = 2f+1
     */
    uint32_t GetM() const;

    /**
     * @brief Create block from current state
     * @return The created block
     */
    std::shared_ptr<ledger::Block> CreateBlock();

    /**
     * @brief Verify block proposal
     * @param block Block to verify
     * @return true if block is valid
     */
    bool VerifyBlock(const std::shared_ptr<ledger::Block>& block);

    /**
     * @brief Reset consensus state for new round
     */
    void Reset();

    /**
     * @brief Handle timeout
     */
    void OnTimeout();

    /**
     * @brief Generate random nonce
     * @return Random nonce value
     */
    uint64_t GenerateNonce();

    /**
     * @brief Create consensus invocation script with validator signatures
     * @return Invocation script for multi-signature verification
     */
    io::ByteVector CreateConsensusInvocationScript();

    /**
     * @brief Create consensus verification script for M-of-N signatures
     * @return Verification script for multi-signature consensus
     */
    io::ByteVector CreateConsensusVerificationScript();

    /**
     * @brief Get validator public key from validator ID
     * @param validator_id The validator's script hash
     * @return The validator's public key, or nullopt if not found
     */
    std::optional<cryptography::ecc::ECPoint> GetValidatorPublicKey(const io::UInt160& validator_id);

    /**
     * @brief Get memory pool reference
     * @return Memory pool for transaction management
     */
    std::shared_ptr<ledger::MemoryPool> GetMemoryPool();

    /**
     * @brief Get previous block from blockchain
     * @return Previous block, or nullptr if genesis
     */
    std::shared_ptr<ledger::Block> GetPreviousBlock();

    /**
     * @brief Calculate merkle root from transactions
     * @param transactions Transaction list
     * @return Merkle root hash
     */
    io::UInt256 CalculateMerkleRoot(const std::vector<network::p2p::payloads::Neo3Transaction>& transactions);

    /**
     * @brief Calculate next consensus address
     * @return Next consensus script hash
     */
    io::UInt160 CalculateNextConsensus();

    /**
     * @brief Get current blockchain height
     * @return Current block height
     */
    uint32_t GetCurrentBlockHeight();

    /**
     * @brief Verify consensus witness signatures
     * @param witness The witness to verify
     * @param block_hash The block hash being signed
     * @return true if witness is valid
     */
    bool VerifyConsensusWitness(const ledger::Witness& witness, const io::UInt256& block_hash);
};
}  // namespace neo::consensus
