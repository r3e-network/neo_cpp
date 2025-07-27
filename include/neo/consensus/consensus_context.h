#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/consensus_payload_helper.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/neo_system.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_verification_context.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/persistence/data_cache.h>
#include <neo/protocol_settings.h>
#include <neo/sign/isigner.h>
#include <unordered_map>
#include <vector>

namespace neo::consensus
{
/**
 * @brief Represents the consensus context for Neo N3 dBFT.
 *
 * This class manages the state of the consensus process, including
 * tracking messages from validators and building blocks.
 */
class ConsensusContext : public io::ISerializable
{
  public:
    /**
     * @brief Constructs a ConsensusContext.
     * @param neoSystem The Neo system instance.
     * @param settings The protocol settings.
     * @param signer The signer for creating signatures.
     */
    ConsensusContext(std::shared_ptr<ledger::NeoSystem> neoSystem, std::shared_ptr<ProtocolSettings> settings,
                     std::shared_ptr<sign::ISigner> signer);

    // Block and consensus state
    std::shared_ptr<ledger::Block> Block;
    uint8_t ViewNumber = 0;
    std::chrono::milliseconds TimePerBlock;
    std::vector<cryptography::ecc::ECPoint> Validators;
    int MyIndex = -1;
    std::vector<io::UInt256> TransactionHashes;
    std::unordered_map<io::UInt256, std::shared_ptr<ledger::Transaction>> Transactions;

    // Consensus message storage (using ExtensiblePayload)
    std::vector<std::shared_ptr<network::p2p::payloads::ExtensiblePayload>> PreparationPayloads;
    std::vector<std::shared_ptr<network::p2p::payloads::ExtensiblePayload>> CommitPayloads;
    std::vector<std::shared_ptr<network::p2p::payloads::ExtensiblePayload>> ChangeViewPayloads;
    std::vector<std::shared_ptr<network::p2p::payloads::ExtensiblePayload>> LastChangeViewPayloads;

    // Last seen message tracking
    std::unordered_map<cryptography::ecc::ECPoint, uint32_t> LastSeenMessage;

    // Transaction verification context
    ledger::TransactionVerificationContext VerificationContext;

    // Snapshot of the blockchain state
    std::shared_ptr<persistence::DataCache> Snapshot;

    // Consensus parameters
    int F() const
    {
        return (Validators.size() - 1) / 3;
    }
    int M() const
    {
        return Validators.size() - F();
    }
    bool IsPrimary() const
    {
        return MyIndex == Block->GetPrimaryIndex();
    }
    bool IsBackup() const
    {
        return MyIndex >= 0 && MyIndex != Block->GetPrimaryIndex();
    }
    bool WatchOnly() const
    {
        return MyIndex < 0;
    }

    // State queries
    bool RequestSentOrReceived() const;
    bool ResponseSent() const;
    bool CommitSent() const;
    bool BlockSent() const;
    bool ViewChanging() const;

    /**
     * @brief Resets the context for a new consensus round.
     * @param viewNumber The view number.
     */
    void Reset(uint8_t viewNumber);

    /**
     * @brief Gets the primary index for a given view number.
     * @param viewNumber The view number.
     * @return The primary index.
     */
    uint8_t GetPrimaryIndex(uint8_t viewNumber) const;

    /**
     * @brief Creates a signed ExtensiblePayload for a consensus message.
     * @param message The consensus message.
     * @return The signed payload.
     */
    std::shared_ptr<network::p2p::payloads::ExtensiblePayload>
    MakeSignedPayload(std::shared_ptr<ConsensusMessage> message);

    /**
     * @brief Makes a change view message.
     * @param reason The reason for changing view.
     * @return The change view payload.
     */
    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> MakeChangeView(uint8_t reason);

    /**
     * @brief Makes a prepare request message.
     * @return The prepare request payload.
     */
    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> MakePrepareRequest();

    /**
     * @brief Makes a prepare response message.
     * @return The prepare response payload.
     */
    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> MakePrepareResponse();

    /**
     * @brief Makes a commit message.
     * @return The commit payload.
     */
    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> MakeCommit();

    /**
     * @brief Makes a recovery request message.
     * @return The recovery request payload.
     */
    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> MakeRecoveryRequest();

    /**
     * @brief Makes a recovery message.
     * @return The recovery message payload.
     */
    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> MakeRecoveryMessage();

    /**
     * @brief Ensures transactions don't exceed block limits.
     * @param txs The transactions to check.
     */
    void EnsureMaxBlockLimitation(const std::vector<std::shared_ptr<ledger::Transaction>>& txs);

    /**
     * @brief Saves the consensus state.
     */
    void Save();

    // ISerializable implementation
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

  private:
    std::shared_ptr<ledger::NeoSystem> neoSystem_;
    std::shared_ptr<ProtocolSettings> settings_;
    std::shared_ptr<sign::ISigner> signer_;
    cryptography::ecc::ECPoint myPublicKey_;
    size_t witnessSize_ = 0;
    std::unordered_map<io::UInt256, std::shared_ptr<ConsensusMessage>> cachedMessages_;

    /**
     * @brief Signs an ExtensiblePayload.
     * @param payload The payload to sign.
     */
    void SignPayload(std::shared_ptr<network::p2p::payloads::ExtensiblePayload> payload);

    /**
     * @brief Gets the expected block size without transactions.
     * @param txCount The transaction count.
     * @return The expected size.
     */
    size_t GetExpectedBlockSizeWithoutTransactions(size_t txCount) const;

    /**
     * @brief Ensures the block header is properly set.
     * @return The block.
     */
    std::shared_ptr<ledger::Block> EnsureHeader();
};
}  // namespace neo::consensus