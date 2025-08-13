/**
 * @file nef_file.cpp
 * @brief Nef File
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/smartcontract/nef_file.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract
{
NefFile::NefFile() : compiler_(""), source_(""), checkSum_(0) {}

const std::string& NefFile::GetCompiler() const { return compiler_; }

void NefFile::SetCompiler(const std::string& compiler) { compiler_ = compiler; }

const std::string& NefFile::GetSource() const { return source_; }

void NefFile::SetSource(const std::string& source) { source_ = source; }

const std::vector<MethodToken>& NefFile::GetTokens() const { return tokens_; }

void NefFile::SetTokens(const std::vector<MethodToken>& tokens) { tokens_ = tokens; }

const io::ByteVector& NefFile::GetScript() const { return script_; }

void NefFile::SetScript(const io::ByteVector& script) { script_ = script; }

uint32_t NefFile::GetCheckSum() const { return checkSum_; }

void NefFile::SetCheckSum(uint32_t checkSum) { checkSum_ = checkSum; }

uint32_t NefFile::ComputeChecksum() const
{
    // Create a memory stream to serialize the NefFile without the checksum
    std::stringstream stream;
    io::BinaryWriter writer(stream);

    // Serialize the header
    writer.Write(Magic);
    writer.WriteFixedString(compiler_, 64);

    // Serialize the source
    writer.WriteVarString(source_);

    // Write reserved byte (0)
    writer.Write(static_cast<uint8_t>(0));

    // Serialize the tokens
    writer.WriteVarInt(tokens_.size());
    for (const auto& token : tokens_)
    {
        token.Serialize(writer);
    }

    // Write reserved bytes (0, 0)
    writer.Write(static_cast<uint16_t>(0));

    // Serialize the script
    writer.WriteVarBytes(script_.AsSpan());

    // Get the serialized data
    std::string data = stream.str();

    // Compute the hash
    auto hash = cryptography::Hash::Hash256(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));

    // Return the first 4 bytes as the checksum
    return *reinterpret_cast<const uint32_t*>(hash.Data());
}

void NefFile::Serialize(io::BinaryWriter& writer) const
{
    // Write the header
    writer.Write(Magic);
    writer.WriteFixedString(compiler_, 64);

    // Write the source
    writer.WriteVarString(source_);

    // Write reserved byte (0)
    writer.Write(static_cast<uint8_t>(0));

    // Write the tokens
    writer.WriteVarInt(tokens_.size());
    for (const auto& token : tokens_)
    {
        token.Serialize(writer);
    }

    // Write reserved bytes (0, 0)
    writer.Write(static_cast<uint16_t>(0));

    // Write the script
    writer.WriteVarBytes(script_.AsSpan());

    // Write the checksum
    writer.Write(checkSum_);
}

void NefFile::Deserialize(io::BinaryReader& reader)
{
    // Read the header
    uint32_t magic = reader.ReadUInt32();
    if (magic != Magic) throw std::runtime_error("Invalid NEF file: wrong magic");

    compiler_ = reader.ReadFixedString(64);

    // Read the source
    source_ = reader.ReadVarString();

    // Read reserved byte (must be 0)
    uint8_t reserved = reader.ReadUInt8();
    if (reserved != 0) throw std::runtime_error("Invalid NEF file: reserved byte must be 0");

    // Read the tokens
    int64_t tokenCount = reader.ReadVarInt();
    if (tokenCount < 0 || tokenCount > 128) throw std::runtime_error("Invalid NEF file: too many tokens");

    tokens_.clear();
    tokens_.reserve(static_cast<size_t>(tokenCount));
    for (int64_t i = 0; i < tokenCount; i++)
    {
        MethodToken token;
        token.Deserialize(reader);
        tokens_.push_back(token);
    }

    // Read reserved bytes (must be 0)
    uint16_t reserved2 = reader.ReadUInt16();
    if (reserved2 != 0) throw std::runtime_error("Invalid NEF file: reserved bytes must be 0");

    // Read the script
    script_ = reader.ReadVarBytes();
    if (script_.IsEmpty()) throw std::runtime_error("Invalid NEF file: script cannot be empty");

    // Read the checksum
    checkSum_ = reader.ReadUInt32();

    // Verify the checksum
    uint32_t computedChecksum = ComputeChecksum();
    if (checkSum_ != computedChecksum) throw std::runtime_error("Invalid NEF file: checksum verification failed");
}

void NefFile::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();

    writer.WritePropertyName("magic");
    writer.WriteNumber(static_cast<int>(Magic));

    writer.WritePropertyName("compiler");
    writer.WriteString(compiler_);

    writer.WritePropertyName("source");
    writer.WriteString(source_);

    writer.WritePropertyName("tokens");
    writer.WriteStartArray();
    for (const auto& token : tokens_)
    {
        token.SerializeJson(writer);
    }
    writer.WriteEndArray();

    writer.WritePropertyName("script");
    writer.WriteBase64String("script", script_.AsSpan());

    writer.WritePropertyName("checksum");
    writer.WriteNumber(static_cast<int>(checkSum_));

    writer.WriteEndObject();
}

void NefFile::DeserializeJson(const io::JsonReader& reader)
{
    compiler_ = reader.ReadString("compiler");
    source_ = reader.ReadString("source");

    // Read tokens
    tokens_.clear();
    auto tokensArray = reader.ReadArray("tokens");
    for (size_t i = 0; i < tokensArray.size(); i++)
    {
        MethodToken token;
        io::JsonReader tokenReader(tokensArray[i]);
        token.DeserializeJson(tokenReader);
        tokens_.push_back(token);
    }

    // Read script
    script_ = reader.ReadBase64String("script");

    // Read checksum
    checkSum_ = static_cast<uint32_t>(reader.ReadNumber("checksum"));
}
}  // namespace neo::smartcontract
