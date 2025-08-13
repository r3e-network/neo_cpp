/**
 * @file recovery_request.cpp
 * @brief Recovery Request
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/recovery_request.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

#include <sstream>

namespace neo::consensus
{
RecoveryRequest::RecoveryRequest(uint8_t viewNumber, uint64_t timestamp)
    : ConsensusMessage(MessageType::RecoveryRequest, viewNumber), timestamp_(timestamp)
{
}

uint64_t RecoveryRequest::GetTimestamp() const { return timestamp_; }

void RecoveryRequest::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    writer.WriteUInt64(timestamp_);
}

void RecoveryRequest::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    timestamp_ = reader.ReadUInt64();
}

io::ByteVector RecoveryRequest::GetData() const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.WriteByte(static_cast<uint8_t>(GetType()));
    writer.WriteByte(GetViewNumber());
    writer.WriteUInt16(GetValidatorIndex());
    writer.WriteUInt64(timestamp_);
    std::string data = stream.str();

    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}
}  // namespace neo::consensus
