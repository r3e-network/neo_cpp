/**
 * @file transaction_output.cpp
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/transaction_output.h>

namespace neo::ledger
{
TransactionOutput::TransactionOutput() : assetId_(), value_(), scriptHash_() {}

TransactionOutput::TransactionOutput(const io::UInt256& assetId, const core::Fixed8& value,
                                     const io::UInt160& scriptHash)
    : assetId_(assetId), value_(value), scriptHash_(scriptHash)
{
}

const io::UInt256& TransactionOutput::GetAssetId() const { return assetId_; }

void TransactionOutput::SetAssetId(const io::UInt256& assetId) { assetId_ = assetId; }

const core::Fixed8& TransactionOutput::GetValue() const { return value_; }

void TransactionOutput::SetValue(const core::Fixed8& value) { value_ = value; }

const io::UInt160& TransactionOutput::GetScriptHash() const { return scriptHash_; }

void TransactionOutput::SetScriptHash(const io::UInt160& scriptHash) { scriptHash_ = scriptHash; }

void TransactionOutput::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(assetId_);
    writer.Write(value_.GetRawValue());  // Write the raw int64_t value
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
    return 32 + 8 + 20;  // UInt256 + Fixed8 (int64_t) + UInt160
}

void TransactionOutput::SerializeJson(io::JsonWriter& writer) const
{
    // Complete JSON serialization for TransactionOutput
    try
    {
        writer.WriteStartObject();

        // Write asset ID
        writer.Write("asset", assetId_.ToString());

        // Write value (Fixed8 amount)
        writer.Write("value", value_.ToString());

        // Write script hash (address)
        writer.Write("address", scriptHash_.ToString());

        // Write script hash as hex for compatibility
        writer.Write("scripthash", scriptHash_.ToString());

        // Transaction output index (n) is context-dependent
        // Set by transaction when outputs are serialized

        writer.WriteEndObject();
    }
    catch (const std::exception& e)
    {
        // If JSON serialization fails, write minimal object
        writer.WriteStartObject();
        writer.Write("error", "TransactionOutput serialization failed: " + std::string(e.what()));
        writer.WriteEndObject();
    }
}

void TransactionOutput::DeserializeJson(const io::JsonReader& reader)
{
    // Complete JSON deserialization for TransactionOutput
    try
    {
        // Read asset ID
        std::string assetStr = reader.ReadString("asset");
        if (!assetStr.empty())
        {
            assetId_ = io::UInt256::Parse(assetStr);
        }
        else
        {
            assetId_ = io::UInt256::Zero();
        }

        // Read value (Fixed8 amount)
        std::string valueStr = reader.ReadString("value");
        if (!valueStr.empty())
        {
            value_ = core::Fixed8::Parse(valueStr);
        }
        else
        {
            value_ = core::Fixed8::Zero();
        }

        // Read script hash (address or scripthash)
        std::string scriptHashStr = reader.ReadString("address");
        if (scriptHashStr.empty())
        {
            scriptHashStr = reader.ReadString("scripthash");
        }
        if (!scriptHashStr.empty())
        {
            scriptHash_ = io::UInt160::Parse(scriptHashStr);
        }
        else
        {
            scriptHash_ = io::UInt160::Zero();
        }
    }
    catch (const std::exception& e)
    {
        // Error parsing JSON - set safe default values
        assetId_ = io::UInt256::Zero();
        value_ = core::Fixed8::Zero();
        scriptHash_ = io::UInt160::Zero();

        throw std::runtime_error("Failed to deserialize TransactionOutput from JSON: " + std::string(e.what()));
    }
}

bool TransactionOutput::operator==(const TransactionOutput& other) const
{
    return assetId_ == other.assetId_ && value_ == other.value_ && scriptHash_ == other.scriptHash_;
}

bool TransactionOutput::operator!=(const TransactionOutput& other) const { return !(*this == other); }
}  // namespace neo::ledger
