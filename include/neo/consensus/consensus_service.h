/**
 * @file consensus_service.h
 * @brief High-level wrapper around the dBFT consensus engine.
 * @author Neo
 */

#pragma once

#include <neo/consensus/consensus_state.h>
#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/pool_item.h>

#include <atomic>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace neo
{
namespace core
{
class ProtocolSettings;
}

namespace ledger
{
class Blockchain;
class MemoryPool;
class Block;
}  // namespace ledger

namespace network::p2p
{
class LocalNode;
namespace payloads
{
class ExtensiblePayload;
}  // namespace payloads
}  // namespace network::p2p

namespace cryptography::ecc
{
class KeyPair;
class ECPoint;
}  // namespace cryptography::ecc

namespace consensus
{
class ConsensusMessage;
class DbftConsensus;

/**
 * @brief Service façade that wires application components into the dBFT engine.
 *
 * The implementation mirrors the architecture of the C# ConsensusService, but relies on
 * the C++ DbftConsensus core for the actual state machine logic.  This wrapper is
 * responsible for:
 *  - Initialising validator metadata from protocol settings
 *  - Bridging consensus callbacks (transaction verification, block persistence, message relay)
 *  - Exposing a lightweight status snapshot for RPC consumers
 *  - Receiving ExtensiblePayload messages from the network layer
 */
class ConsensusService : public std::enable_shared_from_this<ConsensusService>
{
   public:
    /**
     * @brief Lightweight snapshot of the consensus state used by RPC consumers.
     */
    struct Status
    {
        struct ValidatorStatus
        {
            bool hasProposal{false};
            bool hasPrepareResponse{false};
            bool hasCommit{false};
            std::optional<ChangeViewReason> viewChangeReason;
            std::optional<uint32_t> requestedView;
        };

        bool running{false};
        uint32_t blockIndex{0};
        uint32_t viewNumber{0};
        ConsensusPhase phase{ConsensusPhase::Initial};
        size_t prepareResponseCount{0};
        size_t commitCount{0};
        size_t viewChangeCount{0};
        std::vector<cryptography::ecc::ECPoint> validators;
        uint32_t primaryIndex{0};
        uint16_t validatorIndex{std::numeric_limits<uint16_t>::max()};
        std::vector<ValidatorStatus> validatorStates;
        std::optional<io::UInt256> prepareRequestHash;
        size_t expectedTransactionCount{0};
        size_t transactionCount{0};
        std::optional<uint64_t> timestampMilliseconds;
        std::optional<uint64_t> nonce;
    };

    /**
     * @brief Construct a new consensus service wrapper.
     * @param protocolSettings Active protocol settings.
     * @param blockchain Blockchain instance used for persistence and validation.
     * @param memoryPool Transaction memory pool.
     * @param p2pServer P2P server used for broadcasting consensus payloads.
     */
    ConsensusService(std::shared_ptr<core::ProtocolSettings> protocolSettings,
                     std::shared_ptr<ledger::Blockchain> blockchain, std::shared_ptr<ledger::MemoryPool> memoryPool);

    /**
     * @brief Inject the validator key pair used for signing consensus payloads.
     */
    void SetKeyPair(std::unique_ptr<cryptography::ecc::KeyPair> keyPair);

    /**
     * @brief Start the consensus service.
     */
    void Start();

    /**
     * @brief Stop the consensus service.
     */
    void Stop();

    /**
     * @brief Manually start consensus when auto-start is disabled.
     * @return true if consensus transitions to running state.
     */
    bool StartManually();

    bool IsAutoStartEnabled() const;
    void SetAutoStartEnabled(bool value);

    /**
     * @brief Returns true when consensus threads are running.
     */
    bool IsRunning() const;

    /**
     * @brief Retrieves a status snapshot suitable for RPC responses.
     */
    Status GetStatus() const;

    /**
     * @brief Current primary index according to the dBFT context.
     */
    uint32_t GetPrimaryIndex() const;

    /**
     * @brief Validator index of this node (or UINT16_MAX if not a validator).
     */
    uint16_t GetValidatorIndex() const;

    /**
     * @brief Current consensus block index.
     */
    uint32_t GetBlockIndex() const;

    /**
     * @brief Current consensus view number.
     */
    uint32_t GetViewNumber() const;

    /**
     * @brief Validator public keys in consensus order.
     */
    const std::vector<cryptography::ecc::ECPoint>& GetValidators() const;
    const std::vector<io::UInt160>& GetValidatorHashes() const;

    void SetStatusForTesting(const Status& status, const std::vector<io::UInt160>& validatorHashes,
                             std::optional<uint32_t> primaryIndex = std::nullopt,
                             std::optional<uint16_t> validatorIndex = std::nullopt);
    void ClearStatusOverrideForTesting();

    /**
     * @brief Handle an incoming consensus payload from the network layer.
     */
    void HandlePayload(const network::p2p::payloads::ExtensiblePayload& payload);

   private:
    std::shared_ptr<core::ProtocolSettings> protocolSettings_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<ledger::MemoryPool> memoryPool_;
    std::shared_ptr<DbftConsensus> consensus_;
    std::vector<cryptography::ecc::ECPoint> validators_;
    std::vector<io::UInt160> validatorHashes_;
    std::unique_ptr<cryptography::ecc::KeyPair> keyPair_;
    io::UInt160 nodeScriptHash_;
    std::atomic<bool> missing_key_warning_emitted_{false};
    std::atomic<bool> autoStart_{false};

    mutable std::mutex mutex_;
    std::atomic<bool> running_{false};

    std::optional<Status> test_status_override_;
    std::optional<uint32_t> test_primary_index_override_;
    std::optional<uint16_t> test_validator_index_override_;
    std::function<void(std::shared_ptr<ledger::Transaction>)> transaction_added_handler_;
    bool transaction_subscription_registered_{false};
    std::function<void(const ledger::TransactionRemovedEventArgs&)> transaction_removed_handler_;
    bool transaction_removed_subscription_registered_{false};

    void EnsureConsensusInitialised();
    void EnsureTransactionSubscription();
    std::vector<io::UInt160> BuildValidatorHashes(const std::vector<cryptography::ecc::ECPoint>& validators) const;
    std::vector<cryptography::ecc::ECPoint> LoadActiveValidators() const;
    bool VerifyTransaction(const network::p2p::payloads::Neo3Transaction& tx);
    bool PersistBlock(const std::shared_ptr<ledger::Block>& block);
    void BroadcastMessage(const ConsensusMessage& message);
    uint16_t ResolveValidatorIndex() const;
    io::ByteVector SignConsensusData(const io::ByteSpan& data) const;
    void OnTransactionAdded(std::shared_ptr<ledger::Transaction> transaction);
    void OnTransactionRemoved(const ledger::TransactionRemovedEventArgs& args);
};
}  // namespace consensus
}  // namespace neo
