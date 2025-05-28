#include <neo/ledger/coin_reference.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>

namespace neo::ledger
{
    CoinReference::CoinReference()
        : prevIndex_(0)
    {
    }

    CoinReference::CoinReference(const io::UInt256& prevHash, uint16_t prevIndex)
        : prevHash_(prevHash), prevIndex_(prevIndex)
    {
    }

    const io::UInt256& CoinReference::GetPrevHash() const
    {
        return prevHash_;
    }

    void CoinReference::SetPrevHash(const io::UInt256& prevHash)
    {
        prevHash_ = prevHash;
    }

    uint16_t CoinReference::GetPrevIndex() const
    {
        return prevIndex_;
    }

    void CoinReference::SetPrevIndex(uint16_t prevIndex)
    {
        prevIndex_ = prevIndex;
    }

    void CoinReference::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(prevHash_);
        writer.Write(prevIndex_);
    }

    void CoinReference::Deserialize(io::BinaryReader& reader)
    {
        prevHash_ = reader.ReadUInt256();
        prevIndex_ = reader.ReadUInt16();
    }

    void CoinReference::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("txid", prevHash_.ToHexString());
        writer.WriteProperty("vout", prevIndex_);
        writer.WriteEndObject();
    }

    void CoinReference::DeserializeJson(const io::JsonReader& reader)
    {
        // JSON deserialization implementation
        // This would parse the JSON object and extract txid and vout
        // For now, this is a placeholder
    }

    bool CoinReference::operator==(const CoinReference& other) const
    {
        return prevHash_ == other.prevHash_ && prevIndex_ == other.prevIndex_;
    }

    bool CoinReference::operator!=(const CoinReference& other) const
    {
        return !(*this == other);
    }
}
