#include <neo/cryptography/crypto.h>
#include <neo/extensions/integer_extensions.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/network/p2p/inventory_type.h>
#include <sstream>
#include <stdexcept>

namespace neo::network::p2p::payloads
{



io::UInt256 ExtensiblePayload::GetHash() const
{
    if (!hash_calculated_)
    {
        // hash_cache_ = CalculateHash();
        hash_cache_ = io::UInt256(); // Temporary placeholder
        hash_calculated_ = true;
    }
    return hash_cache_.value();
}

size_t ExtensiblePayload::GetSize() const
{
    size_t size = 0;
    
    // Category string (VarString)
    size += extensions::IntegerExtensions::GetVarSize(static_cast<uint64_t>(category_.length())) + category_.length();
    
    // ValidBlockStart and ValidBlockEnd (uint32_t each)
    size += sizeof(uint32_t) * 2;
    
    // Sender (UInt160)
    size += io::UInt160::Size;
    
    // Data (VarBytes)
    size += extensions::IntegerExtensions::GetVarSize(static_cast<uint64_t>(data_.Size())) + data_.Size();
    
    // Witness count (1 byte) + witness size
    size += 1 + witness_.GetSize();
    
    return size;
}

void ExtensiblePayload::Serialize(io::BinaryWriter& writer) const
{
    // Serialize unsigned data
    writer.WriteVarString(category_);
    writer.Write(valid_block_start_);
    writer.Write(valid_block_end_);
    writer.Write(sender_);
    writer.WriteVarBytes(data_.AsSpan());

    // Serialize witness
    writer.Write(static_cast<uint8_t>(1));  // Witness count
    witness_.Serialize(writer);
}

void ExtensiblePayload::Deserialize(io::BinaryReader& reader)
{
    // Deserialize unsigned data
    category_ = reader.ReadVarString(32);
    valid_block_start_ = reader.ReadUInt32();
    valid_block_end_ = reader.ReadUInt32();

    if (valid_block_start_ >= valid_block_end_)
        throw std::runtime_error("Invalid valid block range: " + std::to_string(valid_block_start_) +
                                 " >= " + std::to_string(valid_block_end_));

    sender_ = reader.ReadSerializable<io::UInt160>();
    data_ = reader.ReadVarBytes();

    // Deserialize witness
    uint8_t witnessCount = reader.ReadUInt8();
    if (witnessCount != 1)
        throw std::runtime_error("Expected 1 witness, got " + std::to_string(witnessCount));

    witness_ = reader.ReadSerializable<ledger::Witness>();

    // Reset hash
    hash_calculated_ = false;
}

void ExtensiblePayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("category", category_);
    writer.Write("validBlockStart", valid_block_start_);
    writer.Write("validBlockEnd", valid_block_end_);
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
    valid_block_start_ = reader.ReadUInt32("validBlockStart");
    valid_block_end_ = reader.ReadUInt32("validBlockEnd");
    sender_ = io::UInt160::Parse(reader.ReadString("sender"));
    data_ = io::ByteVector::FromHexString(reader.ReadString("data"));

    io::JsonReader witnessReader(reader.ReadObject("witness"));
    witness_.DeserializeJson(witnessReader);

    // Reset hash
    hash_calculated_ = false;
}

// io::UInt256 ExtensiblePayload::CalculateHash() const
// {
//     std::ostringstream stream;
//     io::BinaryWriter writer(stream);

//     // Serialize unsigned data
//     writer.WriteVarString(category_);
//     writer.Write(valid_block_start_);
//     writer.Write(valid_block_end_);
//     writer.Write(sender_);
//     writer.WriteVarBytes(data_.AsSpan());

//     std::string data = stream.str();
//     return cryptography::Crypto::Hash256(reinterpret_cast<const uint8_t*>(data.data()), data.size());
// }
}  // namespace neo::network::p2p::payloads
