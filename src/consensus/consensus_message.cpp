#include <neo/consensus/consensus_message.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <sstream>

namespace neo::consensus
{
    ConsensusMessage::ConsensusMessage(MessageType type, uint8_t viewNumber)
        : type_(type), viewNumber_(viewNumber), validatorIndex_(0)
    {
    }
    
    MessageType ConsensusMessage::GetType() const
    {
        return type_;
    }
    
    uint8_t ConsensusMessage::GetViewNumber() const
    {
        return viewNumber_;
    }
    
    uint16_t ConsensusMessage::GetValidatorIndex() const
    {
        return validatorIndex_;
    }
    
    void ConsensusMessage::SetValidatorIndex(uint16_t validatorIndex)
    {
        validatorIndex_ = validatorIndex;
    }
    
    const io::ByteVector& ConsensusMessage::GetSignature() const
    {
        return signature_;
    }
    
    void ConsensusMessage::SetSignature(const io::ByteVector& signature)
    {
        signature_ = signature;
    }
    
    void ConsensusMessage::Serialize(io::BinaryWriter& writer) const
    {
        writer.WriteByte(static_cast<uint8_t>(type_));
        writer.WriteByte(viewNumber_);
        writer.WriteUInt16(validatorIndex_);
        writer.WriteVarBytes(signature_.Data(), signature_.Size());
    }
    
    void ConsensusMessage::Deserialize(io::BinaryReader& reader)
    {
        type_ = static_cast<MessageType>(reader.ReadByte());
        viewNumber_ = reader.ReadByte();
        validatorIndex_ = reader.ReadUInt16();
        signature_ = reader.ReadVarBytes();
    }
    
    io::ByteVector ConsensusMessage::GetData() const
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.WriteByte(static_cast<uint8_t>(type_));
        writer.WriteByte(viewNumber_);
        writer.WriteUInt16(validatorIndex_);
        std::string data = stream.str();
        
        return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
    
    bool ConsensusMessage::VerifySignature(const cryptography::ecc::ECPoint& publicKey) const
    {
        if (signature_.IsEmpty())
            return false;
        
        auto data = GetData();
        auto hash = cryptography::Hash::Sha256(data.AsSpan());
        
        return cryptography::ecc::Secp256r1::VerifySignature(hash.AsSpan(), publicKey, signature_.AsSpan());
    }
    
    void ConsensusMessage::Sign(const cryptography::ecc::KeyPair& keyPair)
    {
        auto data = GetData();
        auto hash = cryptography::Hash::Sha256(data.AsSpan());
        
        signature_ = cryptography::ecc::Secp256r1::Sign(hash.AsSpan(), keyPair);
    }
}
