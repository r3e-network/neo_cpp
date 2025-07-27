#include <neo/consensus/consensus_message.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

namespace neo::consensus
{
// ConsensusMessage base class implementation
ConsensusMessage::ConsensusMessage(ConsensusMessageType type)
    : type_(type), view_number_(0), validator_index_(0), block_index_(0)
{
}

void ConsensusMessage::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(type_));
    writer.Write(view_number_);
    writer.Write(validator_index_);
    writer.Write(block_index_);
}

void ConsensusMessage::Deserialize(io::BinaryReader& reader)
{
    type_ = static_cast<ConsensusMessageType>(reader.ReadByte());
    view_number_ = reader.ReadUInt32();
    validator_index_ = reader.ReadUInt32();
    block_index_ = reader.ReadUInt32();
}

std::unique_ptr<ConsensusMessage> ConsensusMessage::CreateFromType(ConsensusMessageType type)
{
    switch (type)
    {
        case ConsensusMessageType::ChangeView:
            return std::make_unique<ViewChangeMessage>();
        case ConsensusMessageType::PrepareRequest:
            return std::make_unique<PrepareRequestMessage>();
        case ConsensusMessageType::PrepareResponse:
            return std::make_unique<PrepareResponseMessage>();
        case ConsensusMessageType::Commit:
            return std::make_unique<CommitMessage>();
        case ConsensusMessageType::RecoveryRequest:
            return std::make_unique<RecoveryRequestMessage>();
        case ConsensusMessageType::RecoveryMessage:
            return std::make_unique<RecoveryMessage>();
        default:
            return nullptr;
    }
}

// ViewChangeMessage implementation
ViewChangeMessage::ViewChangeMessage() : ConsensusMessage(ConsensusMessageType::ChangeView), new_view_number_(0) {}

void ViewChangeMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    writer.Write(new_view_number_);
    writer.Write(static_cast<int64_t>(timestamp_.time_since_epoch().count()));
}

void ViewChangeMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    new_view_number_ = reader.ReadUInt32();
    auto ticks = reader.ReadInt64();
    timestamp_ = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(ticks));
}

// PrepareRequestMessage implementation
PrepareRequestMessage::PrepareRequestMessage() : ConsensusMessage(ConsensusMessageType::PrepareRequest), nonce_(0) {}

io::UInt256 PrepareRequestMessage::GetHash() const
{
    io::ByteVector buffer;
    io::BinaryWriter writer(buffer);
    Serialize(writer);
    return cryptography::Hash::Hash256(io::ByteSpan(buffer.Data(), buffer.Size()));
}

void PrepareRequestMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    writer.Write(nonce_);
    writer.Write(static_cast<int64_t>(timestamp_.time_since_epoch().count()));
    writer.Write(static_cast<uint32_t>(transaction_hashes_.size()));
    for (const auto& hash : transaction_hashes_)
    {
        writer.Write(hash);
    }
}

void PrepareRequestMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    nonce_ = reader.ReadUInt64();
    auto ticks = reader.ReadInt64();
    timestamp_ = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(ticks));

    auto count = reader.ReadUInt32();
    transaction_hashes_.clear();
    transaction_hashes_.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        transaction_hashes_.push_back(reader.Read<io::UInt256>());
    }
}

// PrepareResponseMessage implementation
PrepareResponseMessage::PrepareResponseMessage() : ConsensusMessage(ConsensusMessageType::PrepareResponse) {}

void PrepareResponseMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    writer.Write(prepare_request_hash_);
}

void PrepareResponseMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    prepare_request_hash_ = reader.Read<io::UInt256>();
}

// CommitMessage implementation
CommitMessage::CommitMessage() : ConsensusMessage(ConsensusMessageType::Commit) {}

void CommitMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    writer.WriteVarBytes(signature_);
}

void CommitMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    signature_ = reader.ReadVarBytes();
}

// RecoveryRequestMessage implementation
RecoveryRequestMessage::RecoveryRequestMessage() : ConsensusMessage(ConsensusMessageType::RecoveryRequest) {}

void RecoveryRequestMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
}

void RecoveryRequestMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
}

// RecoveryMessage implementation
RecoveryMessage::RecoveryMessage() : ConsensusMessage(ConsensusMessageType::RecoveryMessage) {}

void RecoveryMessage::AddPrepareResponse(std::unique_ptr<PrepareResponseMessage> msg)
{
    prepare_responses_.push_back(std::move(msg));
}

void RecoveryMessage::AddCommit(std::unique_ptr<CommitMessage> msg)
{
    commits_.push_back(std::move(msg));
}

void RecoveryMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);

    // Write view change
    writer.Write(view_change_ != nullptr);
    if (view_change_)
    {
        view_change_->Serialize(writer);
    }

    // Write prepare request
    writer.Write(prepare_request_ != nullptr);
    if (prepare_request_)
    {
        prepare_request_->Serialize(writer);
    }

    // Write prepare responses
    writer.Write(static_cast<uint32_t>(prepare_responses_.size()));
    for (const auto& response : prepare_responses_)
    {
        response->Serialize(writer);
    }

    // Write commits
    writer.Write(static_cast<uint32_t>(commits_.size()));
    for (const auto& commit : commits_)
    {
        commit->Serialize(writer);
    }
}

void RecoveryMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);

    // Read view change
    if (reader.ReadBool())
    {
        view_change_ = std::make_unique<ViewChangeMessage>();
        view_change_->Deserialize(reader);
    }

    // Read prepare request
    if (reader.ReadBool())
    {
        prepare_request_ = std::make_unique<PrepareRequestMessage>();
        prepare_request_->Deserialize(reader);
    }

    // Read prepare responses
    auto response_count = reader.ReadUInt32();
    prepare_responses_.clear();
    prepare_responses_.reserve(response_count);
    for (uint32_t i = 0; i < response_count; ++i)
    {
        auto response = std::make_unique<PrepareResponseMessage>();
        response->Deserialize(reader);
        prepare_responses_.push_back(std::move(response));
    }

    // Read commits
    auto commit_count = reader.ReadUInt32();
    commits_.clear();
    commits_.reserve(commit_count);
    for (uint32_t i = 0; i < commit_count; ++i)
    {
        auto commit = std::make_unique<CommitMessage>();
        commit->Deserialize(reader);
        commits_.push_back(std::move(commit));
    }
}
}  // namespace neo::consensus