/**
 * @file recovery_message.cpp
 * @brief Network message handling
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/consensus_message.h>
#include <neo/consensus/recovery_message.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

#include <sstream>

namespace neo::consensus
{
RecoveryMessage::RecoveryMessage(uint8_t viewNumber) : ConsensusMessage(ConsensusMessageType::RecoveryMessage)
{
    SetViewNumber(viewNumber);
}

const std::vector<std::shared_ptr<ChangeViewMessage>>& RecoveryMessage::GetChangeViewMessages() const
{
    return changeViewMessages_;
}

void RecoveryMessage::AddChangeViewMessage(std::shared_ptr<ChangeViewMessage> message)
{
    changeViewMessages_.push_back(message);
}

std::shared_ptr<PrepareRequest> RecoveryMessage::GetPrepareRequest() const { return prepareRequest_; }

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
    prepareResponses_.push_back(message);
}

const std::vector<std::shared_ptr<CommitMessage>>& RecoveryMessage::GetCommitMessages() const
{
    return commitMessages_;
}

void RecoveryMessage::AddCommitMessage(std::shared_ptr<CommitMessage> message) { commitMessages_.push_back(message); }

void RecoveryMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);

    // Serialize change view messages
    writer.WriteVarInt(changeViewMessages_.size());
    for (const auto& message : changeViewMessages_)
    {
        writer.WriteUInt16(message->GetValidatorIndex());
        writer.WriteByte(message->GetNewViewNumber());
        writer.WriteUInt64(message->GetTimestamp());
        writer.WriteVarBytes(message->GetSignature().AsSpan());
    }

    // Serialize prepare request
    if (prepareRequest_)
    {
        writer.WriteBool(true);
        writer.WriteUInt16(prepareRequest_->GetValidatorIndex());
        writer.WriteUInt64(prepareRequest_->GetTimestamp());
        writer.WriteUInt64(prepareRequest_->GetNonce());
        writer.Write(prepareRequest_->GetNextConsensus());

        writer.WriteVarInt(prepareRequest_->GetTransactionHashes().size());
        for (const auto& hash : prepareRequest_->GetTransactionHashes())
        {
            writer.Write(hash);
        }

        writer.WriteVarBytes(prepareRequest_->GetSignature().AsSpan());
        writer.WriteVarBytes(prepareRequest_->GetInvocationScript().AsSpan());
        writer.WriteVarBytes(prepareRequest_->GetVerificationScript().AsSpan());
    }
    else
    {
        writer.WriteBool(false);
    }

    // Serialize prepare responses
    writer.WriteVarInt(prepareResponses_.size());
    for (const auto& message : prepareResponses_)
    {
        writer.WriteUInt16(message->GetValidatorIndex());
        writer.Write(message->GetPreparationHash());
        writer.WriteVarBytes(message->GetSignature().AsSpan());
    }

    // Serialize commit messages
    writer.WriteVarInt(commitMessages_.size());
    for (const auto& message : commitMessages_)
    {
        writer.WriteUInt16(message->GetValidatorIndex());
        writer.Write(message->GetCommitHash());
        writer.WriteVarBytes(message->GetCommitSignature().AsSpan());
        writer.WriteVarBytes(message->GetSignature().AsSpan());
    }
}

void RecoveryMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);

    // Deserialize change view messages
    uint64_t count = reader.ReadVarInt();
    changeViewMessages_.clear();
    changeViewMessages_.reserve(count);
    for (uint64_t i = 0; i < count; i++)
    {
        uint16_t validatorIndex = reader.ReadUInt16();
        uint8_t newViewNumber = reader.ReadByte();
        uint64_t timestamp = reader.ReadUInt64();
        io::ByteVector signature = reader.ReadVarBytes();

        auto message = std::make_shared<ChangeViewMessage>(GetViewNumber(), newViewNumber, timestamp);
        message->SetValidatorIndex(validatorIndex);
        message->SetSignature(signature);

        changeViewMessages_.push_back(message);
    }

    // Deserialize prepare request
    bool hasPrepareRequest = reader.ReadBool();
    if (hasPrepareRequest)
    {
        uint16_t validatorIndex = reader.ReadUInt16();
        uint64_t timestamp = reader.ReadUInt64();
        uint64_t nonce = reader.ReadUInt64();
        io::UInt160 nextConsensus = reader.ReadSerializable<io::UInt160>();

        uint64_t hashCount = reader.ReadVarInt();
        std::vector<io::UInt256> transactionHashes;
        transactionHashes.reserve(hashCount);
        for (uint64_t i = 0; i < hashCount; i++)
        {
            transactionHashes.push_back(reader.ReadSerializable<io::UInt256>());
        }

        io::ByteVector signature = reader.ReadVarBytes();
        io::ByteVector invocationScript = reader.ReadVarBytes();
        io::ByteVector verificationScript = reader.ReadVarBytes();

        prepareRequest_ = std::make_shared<PrepareRequest>(GetViewNumber(), timestamp, nonce, nextConsensus);
        prepareRequest_->SetValidatorIndex(validatorIndex);
        prepareRequest_->SetTransactionHashes(transactionHashes);
        prepareRequest_->SetSignature(signature);
        prepareRequest_->SetInvocationScript(invocationScript);
        prepareRequest_->SetVerificationScript(verificationScript);
    }
    else
    {
        prepareRequest_ = nullptr;
    }

    // Deserialize prepare responses
    count = reader.ReadVarInt();
    prepareResponses_.clear();
    prepareResponses_.reserve(count);
    for (uint64_t i = 0; i < count; i++)
    {
        uint16_t validatorIndex = reader.ReadUInt16();
        io::UInt256 preparationHash = reader.ReadSerializable<io::UInt256>();
        io::ByteVector signature = reader.ReadVarBytes();

        auto message = std::make_shared<PrepareResponse>(GetViewNumber(), preparationHash);
        message->SetValidatorIndex(validatorIndex);
        message->SetSignature(signature);

        prepareResponses_.push_back(message);
    }

    // Deserialize commit messages
    count = reader.ReadVarInt();
    commitMessages_.clear();
    commitMessages_.reserve(count);
    for (uint64_t i = 0; i < count; i++)
    {
        uint16_t validatorIndex = reader.ReadUInt16();
        io::UInt256 commitHash = reader.ReadSerializable<io::UInt256>();
        io::ByteVector commitSignature = reader.ReadVarBytes();
        io::ByteVector signature = reader.ReadVarBytes();

        auto message = std::make_shared<CommitMessage>(GetViewNumber(), commitHash, commitSignature);
        message->SetValidatorIndex(validatorIndex);
        message->SetSignature(signature);

        commitMessages_.push_back(message);
    }
}

io::ByteVector RecoveryMessage::GetData() const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.WriteByte(static_cast<uint8_t>(GetType()));
    writer.WriteByte(GetViewNumber());
    writer.WriteUInt16(GetValidatorIndex());
    std::string data = stream.str();

    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}
}  // namespace neo::consensus
