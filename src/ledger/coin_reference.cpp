#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/coin_reference.h>

namespace neo::ledger
{
CoinReference::CoinReference() : prevHash_(), prevIndex_(0) {}

CoinReference::CoinReference(const io::UInt256& prevHash, uint16_t prevIndex)
    : prevHash_(prevHash), prevIndex_(prevIndex)
{
}

const io::UInt256& CoinReference::GetPrevHash() const { return prevHash_; }

void CoinReference::SetPrevHash(const io::UInt256& prevHash) { prevHash_ = prevHash; }

uint16_t CoinReference::GetPrevIndex() const { return prevIndex_; }

void CoinReference::SetPrevIndex(uint16_t prevIndex) { prevIndex_ = prevIndex; }

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
    return 32 + 2;  // UInt256 + uint16_t
}

void CoinReference::SerializeJson(io::JsonWriter& writer) const
{
    // Complete JSON serialization for CoinReference
    try
    {
        writer.WriteStartObject();

        // Write transaction hash (txid)
        writer.Write("txid", prevHash_.ToString());

        // Write output index (vout)
        writer.Write("vout", static_cast<uint32_t>(prevIndex_));

        writer.WriteEndObject();
    }
    catch (const std::exception& e)
    {
        // If JSON serialization fails, write minimal object
        writer.WriteStartObject();
        writer.Write("error", "CoinReference serialization failed: " + std::string(e.what()));
        writer.WriteEndObject();
    }
}

void CoinReference::DeserializeJson(const io::JsonReader& reader)
{
    // Complete JSON deserialization for CoinReference
    try
    {
        // Read transaction hash (txid)
        std::string txidStr = reader.ReadString("txid");
        if (!txidStr.empty())
        {
            prevHash_ = io::UInt256::Parse(txidStr);
        }
        else
        {
            prevHash_ = io::UInt256::Zero();
        }

        // Read output index (vout)
        prevIndex_ = static_cast<uint16_t>(reader.ReadUInt32("vout"));
    }
    catch (const std::exception& e)
    {
        // Error parsing JSON - set safe default values
        prevHash_ = io::UInt256::Zero();
        prevIndex_ = 0;

        throw std::runtime_error("Failed to deserialize CoinReference from JSON: " + std::string(e.what()));
    }
}

bool CoinReference::operator==(const CoinReference& other) const
{
    return prevHash_ == other.prevHash_ && prevIndex_ == other.prevIndex_;
}

bool CoinReference::operator!=(const CoinReference& other) const { return !(*this == other); }
}  // namespace neo::ledger
