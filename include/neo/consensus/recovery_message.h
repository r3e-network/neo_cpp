/**
 * @file recovery_message.h
 * @brief Network message handling
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/consensus/change_view_message.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/io/byte_vector.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace neo::consensus
{
/**
 * @brief Represents a recovery message.
 */
class RecoveryMessage : public ConsensusMessage
{
   public:
    struct ChangeViewPayloadCompact
    {
        uint32_t validator_index{0};
        uint32_t original_view_number{0};
        uint64_t timestamp{0};
        io::ByteVector invocation_script;
    };

    struct PreparationPayloadCompact
    {
        uint32_t validator_index{0};
        io::ByteVector invocation_script;
    };

    struct CommitPayloadCompact
    {
        uint32_t view_number{0};
        uint32_t validator_index{0};
        io::ByteVector signature;
        io::ByteVector invocation_script;
    };

   public:
    /**
     * @brief Constructs a RecoveryMessage.
     * @param viewNumber The view number.
     */
    explicit RecoveryMessage(uint8_t viewNumber);

    const std::vector<ChangeViewPayloadCompact>& GetChangeViewPayloads() const { return change_view_payloads_; }
    void AddChangeViewPayload(const ChangeViewPayloadCompact& payload);

    /**
     * @brief Gets the prepare request.
     * @return The prepare request.
     */
    std::shared_ptr<PrepareRequest> GetPrepareRequest() const;

    /**
     * @brief Sets the prepare request.
     * @param prepareRequest The prepare request.
     */
    void SetPrepareRequest(std::shared_ptr<PrepareRequest> prepareRequest);

    void SetPreparationHash(const io::UInt256& hash);
    std::optional<io::UInt256> GetPreparationHash() const { return preparation_hash_; }

    /**
     * @brief Convenience setter accepting an array of prepare requests.
     *        First non-null entry will be stored as the active request.
     */
    void SetPrepareRequests(const std::vector<std::shared_ptr<PrepareRequest>>& requests);

    const std::vector<PreparationPayloadCompact>& GetPreparationPayloads() const { return preparation_payloads_; }
    void AddPreparationPayload(const PreparationPayloadCompact& payload);

    void SetPreparationPayloads(const std::vector<PreparationPayloadCompact>& payloads);

    const std::vector<CommitPayloadCompact>& GetCommitPayloads() const { return commit_payloads_; }
    void AddCommitPayload(const CommitPayloadCompact& payload);
    void SetCommitPayloads(const std::vector<CommitPayloadCompact>& payloads);

    /**
     * @brief Access transactions bundled with the recovery payload.
     */
    const std::vector<network::p2p::payloads::Neo3Transaction>& GetTransactions() const { return transactions_; }

    /**
     * @brief Add a single transaction to the recovery payload.
     */
    void AddTransaction(const network::p2p::payloads::Neo3Transaction& transaction);

    /**
     * @brief Replace recovery transactions.
     */
    void SetTransactions(const std::vector<network::p2p::payloads::Neo3Transaction>& transactions);

    /**
     * @brief Serializes the object.
     * @param writer The writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the object.
     * @param reader The reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Gets the message data.
     * @return The message data.
     */
    io::ByteVector GetData() const;

   private:
    std::vector<ChangeViewPayloadCompact> change_view_payloads_;
    std::shared_ptr<PrepareRequest> prepareRequest_;
    std::optional<io::UInt256> preparation_hash_;
    std::vector<PreparationPayloadCompact> preparation_payloads_;
    std::vector<CommitPayloadCompact> commit_payloads_;
    std::vector<network::p2p::payloads::Neo3Transaction> transactions_;
};
}  // namespace neo::consensus
