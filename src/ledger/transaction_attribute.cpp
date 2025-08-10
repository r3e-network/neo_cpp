#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/transaction_attribute.h>

#include <stdexcept>

namespace neo::ledger
{
TransactionAttribute::TransactionAttribute() : usage_(Usage::ContractHash) {}

TransactionAttribute::TransactionAttribute(Usage usage, const io::ByteVector& data) : usage_(usage), data_(data) {}

TransactionAttribute::Usage TransactionAttribute::GetUsage() const { return usage_; }

void TransactionAttribute::SetUsage(Usage usage) { usage_ = usage; }

const io::ByteVector& TransactionAttribute::GetData() const { return data_; }

void TransactionAttribute::SetData(const io::ByteVector& data) { data_ = data; }

void TransactionAttribute::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(usage_));

    // Neo N3 TransactionAttributeType handling
    if (usage_ == Usage::HighPriority)
    {
        // HighPriority has no additional data
    }
    else if (usage_ == Usage::OracleResponse || usage_ == Usage::NotValidBefore || usage_ == Usage::Conflicts ||
             usage_ == Usage::NotaryAssisted)
    {
        // Neo N3 attributes store their data directly
        writer.Write(data_.AsSpan());
    }
    // Legacy Neo 2.x attribute handling
    else if (usage_ == Usage::ContractHash || usage_ == Usage::Vote ||
             (static_cast<uint8_t>(usage_) >= static_cast<uint8_t>(Usage::Hash1) &&
              static_cast<uint8_t>(usage_) <= static_cast<uint8_t>(Usage::Hash15)))
    {
        writer.Write(data_.AsSpan());
    }
    else if (usage_ == Usage::ECDH02 || usage_ == Usage::ECDH03)
    {
        writer.Write(data_.AsSpan());
    }
    else if (usage_ == Usage::Script)
    {
        // Script attribute should be exactly 20 bytes (UInt160)
        if (data_.Size() != 20)
        {
            throw std::invalid_argument("Script attribute data must be exactly 20 bytes");
        }
        writer.Write(data_.AsSpan());
    }
    else if (usage_ == Usage::DescriptionUrl)
    {
        writer.Write(static_cast<uint8_t>(data_.Size()));
        writer.Write(data_.AsSpan());
    }
    else if (usage_ == Usage::Description || (static_cast<uint8_t>(usage_) >= static_cast<uint8_t>(Usage::Remark) &&
                                              static_cast<uint8_t>(usage_) <= static_cast<uint8_t>(Usage::Remark15)))
    {
        writer.WriteVarBytes(data_.AsSpan());
    }
    else
    {
        throw std::invalid_argument("Invalid transaction attribute usage");
    }
}

void TransactionAttribute::Deserialize(io::BinaryReader& reader)
{
    usage_ = static_cast<Usage>(reader.ReadUInt8());

    // Neo N3 TransactionAttributeType handling
    if (usage_ == Usage::HighPriority)
    {
        // HighPriority has no additional data
        data_ = io::ByteVector();
    }
    else if (usage_ == Usage::OracleResponse)
    {
        // OracleResponse: read the remaining data as-is
        // The specific OracleResponse parsing is handled in GetOracleResponse()
        // Read Id (8 bytes) + Code (1 byte) + Result (var bytes)
        std::vector<uint8_t> oracleData;

        // Read Id
        auto idBytes = reader.ReadBytes(8);
        oracleData.insert(oracleData.end(), idBytes.begin(), idBytes.end());

        // Read Code
        auto codeBytes = reader.ReadBytes(1);
        oracleData.insert(oracleData.end(), codeBytes.begin(), codeBytes.end());

        // Read Result as var bytes
        auto resultBytes = reader.ReadVarBytes();
        // Add the result bytes directly to our data
        oracleData.insert(oracleData.end(), resultBytes.begin(), resultBytes.end());

        data_ = io::ByteVector(oracleData);
    }
    // Legacy Neo 2.x attribute handling - check Script first due to value collision with NotValidBefore
    else if (usage_ == Usage::Script)
    {
        data_ = reader.ReadBytes(20);
    }
    else if (usage_ == Usage::NotValidBefore)
    {
        // NotValidBefore: read uint32 height
        data_ = reader.ReadBytes(sizeof(uint32_t));
    }
    else if (usage_ == Usage::Conflicts)
    {
        // Conflicts: read UInt256 hash
        data_ = reader.ReadBytes(32);
    }
    else if (usage_ == Usage::NotaryAssisted)
    {
        // NotaryAssisted: read byte NKeys
        data_ = reader.ReadBytes(1);
    }
    else if (usage_ == Usage::ContractHash || usage_ == Usage::Vote ||
             (static_cast<uint8_t>(usage_) >= static_cast<uint8_t>(Usage::Hash1) &&
              static_cast<uint8_t>(usage_) <= static_cast<uint8_t>(Usage::Hash15)))
    {
        data_ = reader.ReadBytes(32);
    }
    else if (usage_ == Usage::ECDH02 || usage_ == Usage::ECDH03)
    {
        data_ = reader.ReadBytes(33);
    }
    else if (usage_ == Usage::DescriptionUrl)
    {
        uint8_t length = reader.ReadUInt8();
        data_ = reader.ReadBytes(length);
    }
    else if (usage_ == Usage::Description || (static_cast<uint8_t>(usage_) >= static_cast<uint8_t>(Usage::Remark) &&
                                              static_cast<uint8_t>(usage_) <= static_cast<uint8_t>(Usage::Remark15)))
    {
        data_ = reader.ReadVarBytes();
    }
    else
    {
        throw std::invalid_argument("Invalid transaction attribute usage");
    }
}

void TransactionAttribute::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("usage", static_cast<uint8_t>(usage_));
    writer.WriteProperty("data", data_.ToHexString());
    writer.WriteEndObject();
}

void TransactionAttribute::DeserializeJson(const io::JsonReader& reader)
{
    // Read usage
    uint8_t usageValue = reader.ReadUInt8("usage");
    usage_ = static_cast<Usage>(usageValue);

    // Read data as hex string
    std::string dataHex = reader.ReadString("data");
    data_ = io::ByteVector::ParseHex(dataHex);
}

bool TransactionAttribute::operator==(const TransactionAttribute& other) const
{
    return usage_ == other.usage_ && data_ == other.data_;
}

bool TransactionAttribute::operator!=(const TransactionAttribute& other) const { return !(*this == other); }
}  // namespace neo::ledger
