/**
 * @file consensus_service.h
 * @brief Neo dBFT consensus service implementation
 * @details This file contains the ConsensusService class which implements
 *          the delegated Byzantine Fault Tolerance (dBFT) consensus mechanism
 *          for the Neo blockchain.
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/consensus/change_view_message.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/consensus_context.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/node/neo_system.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * @namespace neo::consensus
 * @brief Contains dBFT consensus implementation components
 * @details This namespace includes all classes related to the delegated
 *          Byzantine Fault Tolerance consensus mechanism, including
 *          message types, consensus context, and service implementation.
 */
namespace neo::consensus
{
/**
 * @class ConsensusService
 * @brief Implements the dBFT consensus mechanism for block production
 * @details The ConsensusService manages the consensus process including:
 *          - Validator coordination and communication
 *          - Block proposal creation and validation
 *          - Consensus message processing (PrepareRequest, PrepareResponse, Commit)
 *          - View change management for fault tolerance
 *          - Recovery from network partitions
 * 
 * @thread_safety Thread-safe for all public methods
 * @performance Optimized for low-latency block production (15 second blocks)
 * @security Byzantine fault tolerant up to f = (n-1)/3 faulty nodes
 */
class ConsensusService
{
   public:
    /**
     * @brief Constructs a ConsensusService instance
     * @param neoSystem The Neo system instance containing blockchain and network
     * @param keyPair The validator's key pair for signing consensus messages
     * @pre neoSystem must be initialized
     * @pre keyPair must be a valid validator key
     * @throws std::invalid_argument if neoSystem is null or keyPair is invalid
     */
    ConsensusService(std::shared_ptr<node::NeoSystem> neoSystem, const cryptography::ecc::KeyPair& keyPair);

    /**
     * @brief Destructor
     * @details Ensures proper cleanup of consensus threads and resources
     * @post All consensus threads are stopped and resources released
     */
    ~ConsensusService();

    /**
     * @brief Starts the consensus service and begins block production
     * @details Initializes consensus state and starts the consensus thread
     * @pre Service must not be already running
     * @post Service is running and participating in consensus
     * @throws std::runtime_error if service is already running
     */
    void Start();

    /**
     * @brief Stops the consensus service gracefully
     * @details Completes current consensus round and stops all threads
     * @post Service is stopped and can be restarted
     * @note This method blocks until service is fully stopped
     */
    void Stop();

    /**
     * @brief Checks if the consensus service is currently running
     * @return true if service is active and participating in consensus
     * @thread_safety Thread-safe, can be called from any thread
     */
    bool IsRunning() const;

    /**
     * @brief Gets the current set of consensus validators
     * @return Vector of validator public keys (EC points)
     * @details Validators are ordered by their voting weight
     * @thread_safety Thread-safe, returns const reference
     */
    const std::vector<cryptography::ecc::ECPoint>& GetValidators() const;

    /**
     * @brief Gets this node's index in the validator set
     * @return Index in the validator array, or UINT16_MAX if not a validator
     * @details Index is determined by the node's public key position
     */
    uint16_t GetValidatorIndex() const;

    /**
     * @brief Gets the current primary (speaker) validator index
     * @return Index of the primary validator for current view
     * @details Primary rotates based on formula: (blockIndex - viewNumber) % validatorCount
     */
    uint16_t GetPrimaryIndex() const;

    /**
     * @brief Gets the current consensus view number
     * @return Current view (increments on view change/timeout)
     * @details View changes occur when consensus cannot be reached
     */
    uint8_t GetViewNumber() const;

    /**
     * @brief Gets the block index being consensused
     * @return Height of the block currently being produced
     * @details This is always current blockchain height + 1
     */
    uint32_t GetBlockIndex() const;

    /**
     * @brief Gets the validator's key pair
     * @return Reference to the cryptographic key pair
     * @warning Handle with care - contains private key
     * @thread_safety Thread-safe, returns const reference
     */
    const cryptography::ecc::KeyPair& GetKeyPair() const;

    /**
     * @brief Gets the Neo system.
     * @return The Neo system.
     */
    std::shared_ptr<node::NeoSystem> GetNeoSystem() const;

    /**
     * @brief Handles received prepare request message.
     * @param request The prepare request message.
     */
    void OnPrepareRequestReceived(const PrepareRequest& request);

    /**
     * @brief Handles received prepare response message.
     * @param response The prepare response message.
     */
    void OnPrepareResponseReceived(const PrepareResponse& response);

    /**
     * @brief Handles received change view message.
     * @param message The change view message.
     */
    void OnChangeViewReceived(const ChangeViewMessage& message);

    /**
     * @brief Handles received commit message.
     * @param commit The commit message.
     */
    void OnCommitReceived(const CommitMessage& commit);

    /**
     * @brief Validates a block.
     * @param block The block to validate.
     * @return True if the block is valid, false otherwise.
     */
    bool ValidateBlock(std::shared_ptr<ledger::Block> block);

   private:
    std::shared_ptr<node::NeoSystem> neoSystem_;
    cryptography::ecc::KeyPair keyPair_;
    std::shared_ptr<ConsensusContext> context_;
    std::unordered_map<uint8_t, std::unordered_map<uint16_t, std::shared_ptr<ChangeViewMessage>>>
        pendingChangeViewMessages_;
    std::unordered_map<uint8_t, std::shared_ptr<PrepareRequest>> pendingPrepareRequests_;
    std::unordered_map<uint8_t, std::unordered_map<uint16_t, std::shared_ptr<PrepareResponse>>>
        pendingPrepareResponses_;
    std::unordered_set<io::UInt256> knownHashes_;
    std::atomic<bool> running_;
    std::thread consensusThread_;
    std::mutex mutex_;
    std::condition_variable condition_;
    uint64_t lastChangeViewTime_;
    uint64_t lastPrepareRequestTime_;
    uint64_t lastBlockTime_;

    /**
     * @brief Initializes the consensus.
     */
    void Initialize();

    /**
     * @brief Runs the consensus.
     */
    void RunConsensus();

    /**
     * @brief Handles a message.
     * @param message The message.
     * @param sender The sender.
     */
    void OnMessage(std::shared_ptr<ConsensusMessage> message, const cryptography::ecc::ECPoint& sender);

    /**
     * @brief Handles a change view message.
     * @param message The message.
     * @param sender The sender.
     */
    void OnChangeViewMessage(std::shared_ptr<ChangeViewMessage> message, const cryptography::ecc::ECPoint& sender);

    /**
     * @brief Handles a prepare request message.
     * @param message The message.
     * @param sender The sender.
     */
    void OnPrepareRequestMessage(std::shared_ptr<PrepareRequest> message, const cryptography::ecc::ECPoint& sender);

    /**
     * @brief Handles a prepare response message.
     * @param message The message.
     * @param sender The sender.
     */
    void OnPrepareResponseMessage(std::shared_ptr<PrepareResponse> message, const cryptography::ecc::ECPoint& sender);

    /**
     * @brief Handles a commit message.
     * @param message The message.
     * @param sender The sender.
     */
    void OnCommitMessage(std::shared_ptr<CommitMessage> message, const cryptography::ecc::ECPoint& sender);

    /**
     * @brief Handles a recovery message.
     * @param message The message.
     * @param sender The sender.
     */
    void OnRecoveryMessage(std::shared_ptr<RecoveryMessage> message, const cryptography::ecc::ECPoint& sender);

    /**
     * @brief Handles a recovery request message.
     * @param message The message.
     * @param sender The sender.
     */
    void OnRecoveryRequestMessage(std::shared_ptr<RecoveryRequest> message, const cryptography::ecc::ECPoint& sender);

    /**
     * @brief Checks if the node is primary.
     * @return True if the node is primary, false otherwise.
     */
    bool IsPrimary() const;

    /**
     * @brief Checks if the node is backup.
     * @return True if the node is backup, false otherwise.
     */
    bool IsBackup() const;

    /**
     * @brief Gets the primary index for the specified view.
     * @param viewNumber The view number.
     * @return The primary index.
     */
    uint16_t GetPrimaryIndex(uint8_t viewNumber) const;

    /**
     * @brief Checks if the node should change view.
     * @return True if the node should change view, false otherwise.
     */
    bool ShouldChangeView() const;

    /**
     * @brief Checks if the node has received enough change view messages.
     * @param viewNumber The view number.
     * @return True if the node has received enough change view messages, false otherwise.
     */
    bool HasReceivedEnoughChangeViewMessages(uint8_t viewNumber) const;

    /**
     * @brief Checks if the node has received enough prepare responses.
     * @return True if the node has received enough prepare responses, false otherwise.
     */
    bool HasReceivedEnoughPrepareResponses() const;

    /**
     * @brief Checks if the node has received enough commits.
     * @return True if the node has received enough commits, false otherwise.
     */
    bool HasReceivedEnoughCommits() const;

    /**
     * @brief Changes the view.
     * @param viewNumber The view number.
     */
    void ChangeView(uint8_t viewNumber);

    /**
     * @brief Sends a change view message.
     */
    void SendChangeView();

    /**
     * @brief Sends a prepare request.
     */
    void SendPrepareRequest();

    /**
     * @brief Sends a prepare response.
     */
    void SendPrepareResponse();

    /**
     * @brief Sends a commit.
     */
    void SendCommit();

    /**
     * @brief Sends a recovery request.
     */
    void SendRecoveryRequest();

    /**
     * @brief Sends a recovery message.
     * @param validatorIndex The validator index.
     */
    void SendRecoveryMessage(uint16_t validatorIndex);

    /**
     * @brief Broadcasts a message.
     * @param message The message.
     */
    void BroadcastMessage(std::shared_ptr<ConsensusMessage> message);

    /**
     * @brief Sends a message to a validator.
     * @param message The message.
     * @param validatorIndex The validator index.
     */
    void SendMessage(std::shared_ptr<ConsensusMessage> message, uint16_t validatorIndex);

    /**
     * @brief Creates a block.
     * @return The block.
     */
    std::shared_ptr<ledger::Block> CreateBlock();

    /**
     * @brief Processes the block.
     * @param block The block.
     */
    void ProcessBlock(std::shared_ptr<ledger::Block> block);

    /**
     * @brief Resets the consensus.
     */
    void Reset();

    /**
     * @brief Initializes the validators.
     */
    void InitializeValidators();

    /**
     * @brief Gets the current timestamp.
     * @return The current timestamp.
     */
    uint64_t GetCurrentTimestamp() const;

    /**
     * @brief Gets the timeout for the specified view.
     * @param viewNumber The view number.
     * @return The timeout.
     */
    uint64_t GetTimeout(uint8_t viewNumber) const;
};
}  // namespace neo::consensus
