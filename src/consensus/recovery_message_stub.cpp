#include <neo/consensus/recovery_message.h>

namespace neo::consensus
{
// Minimal stub implementation to satisfy linker
RecoveryMessage::RecoveryMessage(uint8_t viewNumber) : ConsensusMessage(ConsensusMessageType::RecoveryMessage)
{
    SetViewNumber(viewNumber);
}

const std::vector<std::shared_ptr<ChangeViewMessage>>& RecoveryMessage::GetChangeViewMessages() const
{
    static std::vector<std::shared_ptr<ChangeViewMessage>> empty;
    return empty;
}

void RecoveryMessage::AddChangeViewMessage(std::shared_ptr<ChangeViewMessage> message)
{
    // Stub
}

std::shared_ptr<PrepareRequest> RecoveryMessage::GetPrepareRequest() const { return nullptr; }

void RecoveryMessage::SetPrepareRequest(std::shared_ptr<PrepareRequest> prepareRequest)
{
    // Stub
}

const std::vector<std::shared_ptr<PrepareResponse>>& RecoveryMessage::GetPrepareResponses() const
{
    static std::vector<std::shared_ptr<PrepareResponse>> empty;
    return empty;
}

void RecoveryMessage::AddPrepareResponse(std::shared_ptr<PrepareResponse> message)
{
    // Stub
}

const std::vector<std::shared_ptr<CommitMessage>>& RecoveryMessage::GetCommitMessages() const
{
    static std::vector<std::shared_ptr<CommitMessage>> empty;
    return empty;
}

void RecoveryMessage::AddCommitMessage(std::shared_ptr<CommitMessage> message)
{
    // Stub
}

void RecoveryMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    // Stub - minimal serialization
    writer.Write(static_cast<uint32_t>(0));  // No change view messages
    writer.Write(static_cast<uint8_t>(0));   // No prepare request
    writer.Write(static_cast<uint32_t>(0));  // No prepare responses
    writer.Write(static_cast<uint32_t>(0));  // No commit messages
}

void RecoveryMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    // Stub - minimal deserialization
    reader.ReadUInt32();  // Skip change view count
    reader.ReadUInt8();   // Skip prepare request flag
    reader.ReadUInt32();  // Skip prepare response count
    reader.ReadUInt32();  // Skip commit message count
}

// JSON serialization methods are not required for stub
}  // namespace neo::consensus