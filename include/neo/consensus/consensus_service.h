#pragma once

#include <neo/consensus/consensus_message.h>
#include <neo/consensus/change_view_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <neo/consensus/consensus_context.h>
#include <neo/node/neo_system.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>

namespace neo::consensus
{
    /**
     * @brief Represents the consensus service.
     */
    class ConsensusService
    {
    public:
        /**
         * @brief Constructs a ConsensusService.
         * @param neoSystem The Neo system.
         * @param keyPair The key pair.
         */
        ConsensusService(std::shared_ptr<node::NeoSystem> neoSystem, const cryptography::ecc::KeyPair& keyPair);

        /**
         * @brief Destructor.
         */
        ~ConsensusService();

        /**
         * @brief Starts the service.
         */
        void Start();

        /**
         * @brief Stops the service.
         */
        void Stop();

        /**
         * @brief Checks if the service is running.
         * @return True if the service is running, false otherwise.
         */
        bool IsRunning() const;

        /**
         * @brief Gets the validators.
         * @return The validators.
         */
        const std::vector<cryptography::ecc::ECPoint>& GetValidators() const;

        /**
         * @brief Gets the validator index.
         * @return The validator index.
         */
        uint16_t GetValidatorIndex() const;

        /**
         * @brief Gets the primary index.
         * @return The primary index.
         */
        uint16_t GetPrimaryIndex() const;

        /**
         * @brief Gets the view number.
         * @return The view number.
         */
        uint8_t GetViewNumber() const;

        /**
         * @brief Gets the block index.
         * @return The block index.
         */
        uint32_t GetBlockIndex() const;

        /**
         * @brief Gets the key pair.
         * @return The key pair.
         */
        const cryptography::ecc::KeyPair& GetKeyPair() const;

        /**
         * @brief Gets the Neo system.
         * @return The Neo system.
         */
        std::shared_ptr<node::NeoSystem> GetNeoSystem() const;

    private:
        std::shared_ptr<node::NeoSystem> neoSystem_;
        cryptography::ecc::KeyPair keyPair_;
        std::shared_ptr<ConsensusContext> context_;
        std::unordered_map<uint8_t, std::unordered_map<uint16_t, std::shared_ptr<ChangeViewMessage>>> pendingChangeViewMessages_;
        std::unordered_map<uint8_t, std::shared_ptr<PrepareRequest>> pendingPrepareRequests_;
        std::unordered_map<uint8_t, std::unordered_map<uint16_t, std::shared_ptr<PrepareResponse>>> pendingPrepareResponses_;
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
}
