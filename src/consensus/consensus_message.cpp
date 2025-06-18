#include <neo/consensus/consensus_message.h>
#include <neo/consensus/change_view_message.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <neo/io/memory_stream.h>
#include <stdexcept>

namespace neo::consensus
{
    ConsensusMessage::ConsensusMessage(MessageType type)
        : type_(type)
    {
    }

    void ConsensusMessage::Serialize(io::BinaryWriter& writer) const
    {
        writer.WriteByte(static_cast<uint8_t>(type_));
        writer.WriteUInt32(blockIndex_);
        writer.WriteByte(validatorIndex_);
        writer.WriteByte(viewNumber_);
    }

    void ConsensusMessage::Deserialize(io::BinaryReader& reader)
    {
        auto readType = static_cast<MessageType>(reader.ReadByte());
        if (readType != type_)
            throw std::runtime_error("Invalid message type");
        
        blockIndex_ = reader.ReadUInt32();
        validatorIndex_ = reader.ReadByte();
        viewNumber_ = reader.ReadByte();
    }

    size_t ConsensusMessage::GetSize() const
    {
        return sizeof(uint8_t) +    // Type
               sizeof(uint32_t) +   // BlockIndex
               sizeof(uint8_t) +    // ValidatorIndex
               sizeof(uint8_t);     // ViewNumber
    }

    bool ConsensusMessage::Verify(const ProtocolSettings& settings) const
    {
        return validatorIndex_ < settings.GetValidatorsCount();
    }

    std::shared_ptr<ConsensusMessage> ConsensusMessage::DeserializeFrom(const io::ByteVector& data)
    {
        if (data.IsEmpty())
            throw std::runtime_error("Empty data");
        
        auto type = static_cast<MessageType>(data[0]);
        
        std::shared_ptr<ConsensusMessage> message;
        switch (type)
        {
            case MessageType::ChangeView:
                message = std::make_shared<ChangeViewMessage>();
                break;
            case MessageType::PrepareRequest:
                message = std::make_shared<PrepareRequest>();
                break;
            case MessageType::PrepareResponse:
                message = std::make_shared<PrepareResponse>();
                break;
            case MessageType::Commit:
                message = std::make_shared<CommitMessage>();
                break;
            case MessageType::RecoveryRequest:
                message = std::make_shared<RecoveryRequest>();
                break;
            case MessageType::RecoveryMessage:
                message = std::make_shared<RecoveryMessage>();
                break;
            default:
                throw std::runtime_error("Unknown message type");
        }
        
        io::MemoryStream stream(data);
        io::BinaryReader reader(stream);
        message->Deserialize(reader);
        
        return message;
    }
}