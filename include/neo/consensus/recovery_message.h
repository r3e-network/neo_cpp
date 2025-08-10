#pragma once

#include <neo/consensus/change_view_message.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace neo::consensus
{
/**
 * @brief Represents a recovery message.
 */
class RecoveryMessage : public ConsensusMessage
{
   public:
    /**
     * @brief Constructs a RecoveryMessage.
     * @param viewNumber The view number.
     */
    explicit RecoveryMessage(uint8_t viewNumber);

    /**
     * @brief Gets the change view messages.
     * @return The change view messages.
     */
    const std::vector<std::shared_ptr<ChangeViewMessage>>& GetChangeViewMessages() const;

    /**
     * @brief Adds a change view message.
     * @param message The change view message.
     */
    void AddChangeViewMessage(std::shared_ptr<ChangeViewMessage> message);

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

    /**
     * @brief Gets the prepare responses.
     * @return The prepare responses.
     */
    const std::vector<std::shared_ptr<PrepareResponse>>& GetPrepareResponses() const;

    /**
     * @brief Adds a prepare response.
     * @param message The prepare response.
     */
    void AddPrepareResponse(std::shared_ptr<PrepareResponse> message);

    /**
     * @brief Gets the commit messages.
     * @return The commit messages.
     */
    const std::vector<std::shared_ptr<CommitMessage>>& GetCommitMessages() const;

    /**
     * @brief Adds a commit message.
     * @param message The commit message.
     */
    void AddCommitMessage(std::shared_ptr<CommitMessage> message);

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
    std::vector<std::shared_ptr<ChangeViewMessage>> changeViewMessages_;
    std::shared_ptr<PrepareRequest> prepareRequest_;
    std::vector<std::shared_ptr<PrepareResponse>> prepareResponses_;
    std::vector<std::shared_ptr<CommitMessage>> commitMessages_;
};
}  // namespace neo::consensus
