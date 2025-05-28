#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/cryptography/crypto.h>
#include <neo/types.h>
#include <stdexcept>
#include <sstream>

namespace neo::network::p2p::payloads
{
    ExtensiblePayload::ExtensiblePayload()
        : validBlockStart_(0), validBlockEnd_(0), hashCalculated_(false)
    {
    }

    const std::string& ExtensiblePayload::GetCategory() const
    {
        return category_;
    }

    void ExtensiblePayload::SetCategory(const std::string& category)
    {
        category_ = category;
        hashCalculated_ = false;
    }

    uint32_t ExtensiblePayload::GetValidBlockStart() const
    {
        return validBlockStart_;
    }

    void ExtensiblePayload::SetValidBlockStart(uint32_t validBlockStart)
    {
        validBlockStart_ = validBlockStart;
        hashCalculated_ = false;
    }

    uint32_t ExtensiblePayload::GetValidBlockEnd() const
    {
        return validBlockEnd_;
    }

    void ExtensiblePayload::SetValidBlockEnd(uint32_t validBlockEnd)
    {
        validBlockEnd_ = validBlockEnd;
        hashCalculated_ = false;
    }

    const types::UInt160& ExtensiblePayload::GetSender() const
    {
        return sender_;
    }

    void ExtensiblePayload::SetSender(const types::UInt160& sender)
    {
        sender_ = sender;
        hashCalculated_ = false;
    }

    const io::ByteVector& ExtensiblePayload::GetData() const
    {
        return data_;
    }

    void ExtensiblePayload::SetData(const io::ByteVector& data)
    {
        data_ = data;
        hashCalculated_ = false;
    }

    const cryptography::ecc::Witness& ExtensiblePayload::GetWitness() const
    {
        return witness_;
    }

    void ExtensiblePayload::SetWitness(const cryptography::ecc::Witness& witness)
    {
        witness_ = witness;
        hashCalculated_ = false;
    }

    const types::UInt256& ExtensiblePayload::GetHash() const
    {
        if (!hashCalculated_)
        {
            hash_ = CalculateHash();
            hashCalculated_ = true;
        }
        return hash_;
    }

    InventoryType ExtensiblePayload::GetInventoryType() const
    {
        return InventoryType::Extensible;
    }

    void ExtensiblePayload::Serialize(io::BinaryWriter& writer) const
    {
        // Serialize unsigned data
        writer.WriteVarString(category_);
        writer.Write(validBlockStart_);
        writer.Write(validBlockEnd_);
        writer.Write(sender_);
        writer.WriteVarBytes(data_.AsSpan());

        // Serialize witness
        writer.Write(static_cast<uint8_t>(1)); // Witness count
        witness_.Serialize(writer);
    }

    void ExtensiblePayload::Deserialize(io::BinaryReader& reader)
    {
        // Deserialize unsigned data
        category_ = reader.ReadVarString(32);
        validBlockStart_ = reader.ReadUInt32();
        validBlockEnd_ = reader.ReadUInt32();

        if (validBlockStart_ >= validBlockEnd_)
            throw std::runtime_error("Invalid valid block range: " + std::to_string(validBlockStart_) + " >= " + std::to_string(validBlockEnd_));

        sender_ = reader.ReadSerializable<types::UInt160>();
        data_ = reader.ReadVarBytes();

        // Deserialize witness
        uint8_t witnessCount = reader.ReadUInt8();
        if (witnessCount != 1)
            throw std::runtime_error("Expected 1 witness, got " + std::to_string(witnessCount));

        witness_ = reader.ReadSerializable<cryptography::ecc::Witness>();

        // Reset hash
        hashCalculated_ = false;
    }

    void ExtensiblePayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("category", category_);
        writer.Write("validBlockStart", validBlockStart_);
        writer.Write("validBlockEnd", validBlockEnd_);
        writer.Write("sender", sender_.ToString());
        writer.Write("data", data_.ToHexString());

        writer.WritePropertyName("witness");
        writer.WriteStartObject();
        witness_.SerializeJson(writer);
        writer.WriteEndObject();
    }

    void ExtensiblePayload::DeserializeJson(const io::JsonReader& reader)
    {
        category_ = reader.ReadString("category");
        validBlockStart_ = reader.ReadUInt32("validBlockStart");
        validBlockEnd_ = reader.ReadUInt32("validBlockEnd");
        sender_ = types::UInt160::Parse(reader.ReadString("sender"));
        data_ = io::ByteVector::FromHexString(reader.ReadString("data"));

        io::JsonReader witnessReader(reader.ReadObject("witness"));
        witness_.DeserializeJson(witnessReader);

        // Reset hash
        hashCalculated_ = false;
    }

    types::UInt256 ExtensiblePayload::CalculateHash() const
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);

        // Serialize unsigned data
        writer.WriteVarString(category_);
        writer.Write(validBlockStart_);
        writer.Write(validBlockEnd_);
        writer.Write(sender_);
        writer.WriteVarBytes(data_.AsSpan());

        std::string data = stream.str();
        return cryptography::Crypto::Hash256(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }
}
