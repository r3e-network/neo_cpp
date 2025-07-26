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
        // Complete JSON serialization for CoinReference
        try{
            writer.WriteStartObject();
            
            // Write transaction hash (txid)
            writer.WritePropertyName("txid");
            writer.WriteValue(prevHash_.ToString());
            
            // Write output index (vout)
            writer.WritePropertyName("vout");
            writer.WriteValue(static_cast<uint32_t>(prevIndex_));
            
            writer.WriteEndObject();
            
        } catch (const std::exception& e) {
            // If JSON serialization fails, write minimal object
            writer.WriteStartObject();
            writer.WritePropertyName("error");
            writer.WriteValue("CoinReference serialization failed: " + std::string(e.what()));
            writer.WriteEndObject();
        }
    }

    void CoinReference::DeserializeJson(const io::JsonReader& reader)
    {
        // Complete JSON deserialization for CoinReference
        try {
            reader.ReadStartObject();
            
            while (reader.Read()) {
                if (reader.TokenType() == io::JsonToken::EndObject) {
                    break;
                }
                
                if (reader.TokenType() == io::JsonToken::PropertyName) {
                    std::string propertyName = reader.GetString();
                    reader.Read(); // Move to property value
                    
                    if (propertyName == "txid") {
                        if (reader.TokenType() != io::JsonToken::Null) {
                            std::string hashStr = reader.GetString();
                            prevHash_ = io::UInt256::Parse(hashStr);
                        }
                    }
                    else if (propertyName == "vout") {
                        if (reader.TokenType() == io::JsonToken::Number) {
                            prevIndex_ = static_cast<uint16_t>(reader.GetUInt32());
                        }
                    }
                    else {
                        // Skip unknown properties
                        reader.Skip();
                    }
                }
            }
            
        } catch (const std::exception& e) {
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

    bool CoinReference::operator!=(const CoinReference& other) const
    {
        return !(*this == other);
    }
}
