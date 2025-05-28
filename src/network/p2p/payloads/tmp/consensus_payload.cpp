#include <neo/network/payloads/consensus_payload.h>

namespace neo::network::payloads
{
    ConsensusPayload::ConsensusPayload()
        : version_(0), blockIndex_(0), validatorIndex_(0)
    {
    }

    ConsensusPayload::ConsensusPayload(uint32_t version, const io::UInt256& prevHash, uint32_t blockIndex, 
                                      uint16_t validatorIndex, const io::ByteVector& data)
        : version_(version), prevHash_(prevHash), blockIndex_(blockIndex), 
          validatorIndex_(validatorIndex), data_(data)
    {
    }

    uint32_t ConsensusPayload::GetVersion() const
    {
        return version_;
    }

    void ConsensusPayload::SetVersion(uint32_t version)
    {
        version_ = version;
    }

    const io::UInt256& ConsensusPayload::GetPrevHash() const
    {
        return prevHash_;
    }

    void ConsensusPayload::SetPrevHash(const io::UInt256& prevHash)
    {
        prevHash_ = prevHash;
    }

    uint32_t ConsensusPayload::GetBlockIndex() const
    {
        return blockIndex_;
    }

    void ConsensusPayload::SetBlockIndex(uint32_t blockIndex)
    {
        blockIndex_ = blockIndex;
    }

    uint16_t ConsensusPayload::GetValidatorIndex() const
    {
        return validatorIndex_;
    }

    void ConsensusPayload::SetValidatorIndex(uint16_t validatorIndex)
    {
        validatorIndex_ = validatorIndex;
    }

    const io::ByteVector& ConsensusPayload::GetData() const
    {
        return data_;
    }

    void ConsensusPayload::SetData(const io::ByteVector& data)
    {
        data_ = data;
    }

    void ConsensusPayload::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(version_);
        writer.Write(prevHash_);
        writer.Write(blockIndex_);
        writer.Write(validatorIndex_);
        writer.WriteVarBytes(data_.AsSpan());
    }

    void ConsensusPayload::Deserialize(io::BinaryReader& reader)
    {
        version_ = reader.ReadUInt32();
        prevHash_ = reader.ReadUInt256();
        blockIndex_ = reader.ReadUInt32();
        validatorIndex_ = reader.ReadUInt16();
        data_ = reader.ReadVarBytes(10 * 1024 * 1024); // Max 10 MB
    }

    void ConsensusPayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("version", version_);
        writer.Write("prevHash", prevHash_);
        writer.Write("blockIndex", blockIndex_);
        writer.Write("validatorIndex", validatorIndex_);
        writer.Write("data", data_);
    }

    void ConsensusPayload::DeserializeJson(const io::JsonReader& reader)
    {
        version_ = reader.ReadUInt32("version");
        prevHash_ = reader.ReadUInt256("prevHash");
        blockIndex_ = reader.ReadUInt32("blockIndex");
        validatorIndex_ = reader.ReadUInt16("validatorIndex");
        data_ = reader.ReadByteVector("data");
    }
}
