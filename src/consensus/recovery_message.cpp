/**
 * @file recovery_message.cpp
 * @brief Consensus recovery message implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/recovery_message.h>
#include <algorithm>

namespace neo::consensus
{

RecoveryMessage::RecoveryMessage(uint8_t viewNumber) 
    : ConsensusMessage(ConsensusMessageType::RecoveryMessage)
{
    SetViewNumber(viewNumber);
}

const std::vector<std::shared_ptr<ChangeViewMessage>>& RecoveryMessage::GetChangeViewMessages() const
{
    return changeViewMessages_;
}

void RecoveryMessage::AddChangeViewMessage(std::shared_ptr<ChangeViewMessage> message)
{
    if (message) {
        changeViewMessages_.push_back(message);
    }
}

std::shared_ptr<PrepareRequest> RecoveryMessage::GetPrepareRequest() const 
{ 
    return prepareRequest_;
}

void RecoveryMessage::SetPrepareRequest(std::shared_ptr<PrepareRequest> prepareRequest)
{
    prepareRequest_ = prepareRequest;
}

const std::vector<std::shared_ptr<PrepareResponse>>& RecoveryMessage::GetPrepareResponses() const
{
    return prepareResponses_;
}

void RecoveryMessage::AddPrepareResponse(std::shared_ptr<PrepareResponse> message)
{
    if (message) {
        prepareResponses_.push_back(message);
    }
}

const std::vector<std::shared_ptr<CommitMessage>>& RecoveryMessage::GetCommitMessages() const
{
    return commitMessages_;
}

void RecoveryMessage::AddCommitMessage(std::shared_ptr<CommitMessage> message)
{
    if (message) {
        commitMessages_.push_back(message);
    }
}

void RecoveryMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    
    // Serialize change view messages
    writer.Write(static_cast<uint32_t>(changeViewMessages_.size()));
    for (const auto& msg : changeViewMessages_) {
        msg->Serialize(writer);
    }
    
    // Serialize prepare request
    writer.Write(static_cast<uint8_t>(prepareRequest_ ? 1 : 0));
    if (prepareRequest_) {
        prepareRequest_->Serialize(writer);
    }
    
    // Serialize prepare responses
    writer.Write(static_cast<uint32_t>(prepareResponses_.size()));
    for (const auto& msg : prepareResponses_) {
        msg->Serialize(writer);
    }
    
    // Serialize commit messages
    writer.Write(static_cast<uint32_t>(commitMessages_.size()));
    for (const auto& msg : commitMessages_) {
        msg->Serialize(writer);
    }
}

void RecoveryMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    
    // Deserialize change view messages
    uint32_t changeViewCount = reader.ReadUInt32();
    changeViewMessages_.clear();
    changeViewMessages_.reserve(changeViewCount);
    for (uint32_t i = 0; i < changeViewCount; i++) {
        auto msg = std::make_shared<ChangeViewMessage>();
        msg->Deserialize(reader);
        changeViewMessages_.push_back(msg);
    }
    
    // Deserialize prepare request
    uint8_t hasPrepareRequest = reader.ReadUInt8();
    if (hasPrepareRequest) {
        prepareRequest_ = std::make_shared<PrepareRequest>();
        prepareRequest_->Deserialize(reader);
    }
    
    // Deserialize prepare responses
    uint32_t prepareResponseCount = reader.ReadUInt32();
    prepareResponses_.clear();
    prepareResponses_.reserve(prepareResponseCount);
    for (uint32_t i = 0; i < prepareResponseCount; i++) {
        auto msg = std::make_shared<PrepareResponse>();
        msg->Deserialize(reader);
        prepareResponses_.push_back(msg);
    }
    
    // Deserialize commit messages
    uint32_t commitCount = reader.ReadUInt32();
    commitMessages_.clear();
    commitMessages_.reserve(commitCount);
    for (uint32_t i = 0; i < commitCount; i++) {
        auto msg = std::make_shared<CommitMessage>();
        msg->Deserialize(reader);
        commitMessages_.push_back(msg);
    }
}

}  // namespace neo::consensus
