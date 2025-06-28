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
        writer.Write(value_.GetRawValue()); // Write the raw int64_t value
        writer.Write(scriptHash_);
    }

    void TransactionOutput::Deserialize(io::BinaryReader& reader)
    {
        assetId_ = reader.Read<io::UInt256>();
        int64_t rawValue = reader.Read<int64_t>();
        value_ = core::Fixed8(rawValue);
        scriptHash_ = reader.Read<io::UInt160>();
    }

    int TransactionOutput::GetSize() const
    {
        return 32 + 8 + 20; // UInt256 + Fixed8 (int64_t) + UInt160
    }

    void TransactionOutput::SerializeJson(io::JsonWriter& writer) const
    {
        // TODO: Implement JSON serialization when JsonWriter is complete
        (void)writer; // Suppress unused parameter warning
    }

    void TransactionOutput::DeserializeJson(const io::JsonReader& reader)
    {
        // TODO: Implement JSON deserialization when JsonReader is complete
        (void)reader; // Suppress unused parameter warning
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
