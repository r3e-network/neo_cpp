/**
 * @file prepare_response.cpp
 * @brief Prepare Response
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/prepare_response.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

#include <sstream>

namespace neo::consensus
{
PrepareResponse::PrepareResponse(uint8_t viewNumber, const io::UInt256& preparationHash)
    : ConsensusMessage(MessageType::PrepareResponse, viewNumber), preparationHash_(preparationHash)
{
}

const io::UInt256& PrepareResponse::GetPreparationHash() const { return preparationHash_; }

void PrepareResponse::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    writer.Write(preparationHash_);
}

void PrepareResponse::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    preparationHash_ = reader.ReadSerializable<io::UInt256>();
}

io::ByteVector PrepareResponse::GetData() const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.WriteByte(static_cast<uint8_t>(GetType()));
    writer.WriteByte(GetViewNumber());
    writer.WriteUInt16(GetValidatorIndex());
    writer.Write(preparationHash_);
    std::string data = stream.str();

    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}
}  // namespace neo::consensus
