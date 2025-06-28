#include <neo/ledger/coin_reference.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

namespace neo::ledger
{
    CoinReference::CoinReference()
        : prevHash_(), prevIndex_(0)
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
        prevHash_ = reader.Read<io::UInt256>();
        prevIndex_ = reader.Read<uint16_t>();
    }

    int CoinReference::GetSize() const
    {
        return 32 + 2; // UInt256 + uint16_t
    }

    void CoinReference::SerializeJson(io::JsonWriter& writer) const
    {
        // TODO: Implement JSON serialization when JsonWriter is complete
        (void)writer; // Suppress unused parameter warning
    }

    void CoinReference::DeserializeJson(const io::JsonReader& reader)
    {
        // TODO: Implement JSON deserialization when JsonReader is complete
        (void)reader; // Suppress unused parameter warning
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
