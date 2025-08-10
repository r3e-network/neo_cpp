#include <neo/consensus/change_view_message.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

#include <sstream>

namespace neo::consensus
{
ChangeViewMessage::ChangeViewMessage(uint8_t viewNumber, uint8_t newViewNumber, uint64_t timestamp)
    : ConsensusMessage(MessageType::ChangeView, viewNumber), newViewNumber_(newViewNumber), timestamp_(timestamp)
{
}

uint8_t ChangeViewMessage::GetNewViewNumber() const { return newViewNumber_; }

uint64_t ChangeViewMessage::GetTimestamp() const { return timestamp_; }

void ChangeViewMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    writer.WriteByte(newViewNumber_);
    writer.WriteUInt64(timestamp_);
}

void ChangeViewMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    newViewNumber_ = reader.ReadByte();
    timestamp_ = reader.ReadUInt64();
}

io::ByteVector ChangeViewMessage::GetData() const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.WriteByte(static_cast<uint8_t>(GetType()));
    writer.WriteByte(GetViewNumber());
    writer.WriteUInt16(GetValidatorIndex());
    writer.WriteByte(newViewNumber_);
    writer.WriteUInt64(timestamp_);
    std::string data = stream.str();

    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}
}  // namespace neo::consensus
