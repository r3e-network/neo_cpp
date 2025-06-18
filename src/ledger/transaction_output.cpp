#include <neo/ledger/transaction_output.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>

namespace neo::ledger
{
    TransactionOutput::TransactionOutput()
        : value_(0)
    {
    }

    TransactionOutput::TransactionOutput(const io::UInt256& assetId, const io::Fixed8& value, const io::UInt160& scriptHash)
        : assetId_(assetId), value_(value), scriptHash_(scriptHash)
    {
    }

    const io::UInt256& TransactionOutput::GetAssetId() const
    {
        return assetId_;
    }

    void TransactionOutput::SetAssetId(const io::UInt256& assetId)
    {
        assetId_ = assetId;
    }

    const io::Fixed8& TransactionOutput::GetValue() const
    {
        return value_;
    }

    void TransactionOutput::SetValue(const io::Fixed8& value)
    {
        value_ = value;
    }

    const io::UInt160& TransactionOutput::GetScriptHash() const
    {
        return scriptHash_;
    }

    void TransactionOutput::SetScriptHash(const io::UInt160& scriptHash)
    {
        scriptHash_ = scriptHash;
    }

    void TransactionOutput::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(assetId_);
        writer.Write(value_);
        writer.Write(scriptHash_);
    }

    void TransactionOutput::Deserialize(io::BinaryReader& reader)
    {
        assetId_ = reader.ReadUInt256();
        value_ = reader.ReadFixed8();
        scriptHash_ = reader.ReadUInt160();
    }

    void TransactionOutput::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("asset", assetId_.ToHexString());
        writer.WriteProperty("value", value_.ToString());
        writer.WriteProperty("address", scriptHash_.ToHexString());
        writer.WriteEndObject();
    }

    void TransactionOutput::DeserializeJson(const io::JsonReader& reader)
    {
        // Read asset as hex string
        std::string assetHex = reader.ReadString("asset");
        assetId_ = io::UInt256::Parse(assetHex);
        
        // Read value as string and parse to Fixed8
        std::string valueStr = reader.ReadString("value");
        value_ = io::Fixed8::Parse(valueStr);
        
        // Read address as hex string
        std::string addressHex = reader.ReadString("address");
        scriptHash_ = io::UInt160::Parse(addressHex);
    }

    bool TransactionOutput::operator==(const TransactionOutput& other) const
    {
        return assetId_ == other.assetId_ && value_ == other.value_ && scriptHash_ == other.scriptHash_;
    }

    bool TransactionOutput::operator!=(const TransactionOutput& other) const
    {
        return !(*this == other);
    }
}
