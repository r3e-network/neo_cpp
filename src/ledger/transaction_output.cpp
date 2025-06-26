#include <neo/ledger/transaction_output.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

namespace neo::ledger
{
    TransactionOutput::TransactionOutput()
        : assetId_(), value_(), scriptHash_()
    {
    }

    TransactionOutput::TransactionOutput(const io::UInt256& assetId, const core::Fixed8& value, const io::UInt160& scriptHash)
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

    const core::Fixed8& TransactionOutput::GetValue() const
    {
        return value_;
    }

    void TransactionOutput::SetValue(const core::Fixed8& value)
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
        writer.Write(value_.GetValue().ToString()); // Serialize as string for simplicity
        writer.Write(scriptHash_);
    }

    void TransactionOutput::Deserialize(io::BinaryReader& reader)
    {
        assetId_ = reader.Read<io::UInt256>();
        std::string valueStr = reader.ReadString();
        value_ = core::Fixed8(core::BigDecimal(valueStr));
        scriptHash_ = reader.Read<io::UInt160>();
    }

    int TransactionOutput::GetSize() const
    {
        return 32 + value_.GetValue().ToString().length() + 1 + 20; // UInt256 + value string + length + UInt160
    }

    void TransactionOutput::SerializeJson(io::JsonWriter& writer) const
    {
        // Basic JSON serialization for compatibility
        writer.WriteStartObject();
        writer.WritePropertyName("asset");
        writer.WriteValue(assetId_.ToString());
        writer.WritePropertyName("value");
        writer.WriteValue(value_.GetValue().ToString());
        writer.WritePropertyName("address");
        writer.WriteValue(scriptHash_.ToString());
        writer.WriteEndObject();
    }

    void TransactionOutput::DeserializeJson(const io::JsonReader& reader)
    {
        // Basic JSON deserialization for compatibility
        // Implementation would parse JSON object
        // For now, just a stub for compilation
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
