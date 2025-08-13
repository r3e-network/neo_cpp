/**
 * @file consensus_message.cpp
 * @brief Network message handling
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/consensus_message.h>
#include <neo/consensus/recovery_message.h>
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
            // RecoveryMessage requires special handling due to complex state
            return std::make_unique<RecoveryMessage>(0);  // Default view number
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

void RecoveryRequestMessage::Serialize(io::BinaryWriter& writer) const { ConsensusMessage::Serialize(writer); }

void RecoveryRequestMessage::Deserialize(io::BinaryReader& reader) { ConsensusMessage::Deserialize(reader); }

// RecoveryMessage implementation is in recovery_message.cpp
}  // namespace neo::consensus