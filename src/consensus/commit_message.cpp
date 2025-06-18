#include <neo/consensus/commit_message.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

namespace neo::consensus
{
    CommitMessage::CommitMessage(uint8_t viewNumber, const io::UInt256& commitHash, const io::ByteVector& signature)
        : ConsensusMessage(MessageType::Commit, viewNumber), commitHash_(commitHash), commitSignature_(signature)
    {
    }
    
    const io::UInt256& CommitMessage::GetCommitHash() const
    {
        return commitHash_;
    }
    
    const io::ByteVector& CommitMessage::GetCommitSignature() const
    {
        return commitSignature_;
    }
    
    void CommitMessage::Serialize(io::BinaryWriter& writer) const
    {
        ConsensusMessage::Serialize(writer);
        writer.Write(commitHash_);
        writer.WriteVarBytes(commitSignature_.AsSpan());
    }
    
    void CommitMessage::Deserialize(io::BinaryReader& reader)
    {
        ConsensusMessage::Deserialize(reader);
        commitHash_ = reader.ReadSerializable<io::UInt256>();
        commitSignature_ = reader.ReadVarBytes();
    }
    
    io::ByteVector CommitMessage::GetData() const
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.WriteByte(static_cast<uint8_t>(GetType()));
        writer.WriteByte(GetViewNumber());
        writer.WriteUInt16(GetValidatorIndex());
        writer.Write(commitHash_);
        std::string data = stream.str();
        
        return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
}
