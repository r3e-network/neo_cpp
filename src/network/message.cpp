#include <cstring>
#include <iostream>
#include <istream>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/lz4.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/message.h>
#include <neo/network/p2p/payload_factory.h>
#include <sstream>
#include <stdexcept>

namespace neo::network
{
Message::Message() : flags_(p2p::MessageFlags::None), command_(p2p::MessageCommand::Version) {}

Message::Message(p2p::MessageCommand command, std::shared_ptr<p2p::IPayload> payload)
    : flags_(p2p::MessageFlags::None), command_(command), payload_(payload)
{
    if (payload)
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        payload->Serialize(writer);
        std::string data = stream.str();
        payloadRaw_ = io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));

        // Try compression if applicable - following C# implementation exactly
        if (ShouldCompress(command) && payloadRaw_.Size() > CompressionMinSize)
        {
            io::ByteVector compressed = cryptography::LZ4::Compress(payloadRaw_.AsSpan());

            if (compressed.Size() < payloadRaw_.Size() - CompressionThreshold)
            {
                payloadCompressed_ = compressed;
                flags_ = SetFlag(flags_, p2p::MessageFlags::Compressed);
                return;
            }
        }

        payloadCompressed_ = payloadRaw_;
    }
}

Message::Message(p2p::MessageCommand command, const io::ByteVector& payload, p2p::MessageFlags flags)
    : flags_(flags), command_(command), payloadRaw_(payload), payloadCompressed_(payload)
{
    // If compressed, decompress the payload
    if (HasFlag(flags_, p2p::MessageFlags::Compressed))
    {
        DecompressPayload();
    }
    else
    {
        payloadRaw_ = payloadCompressed_;
    }

    // Create payload based on command if payload is not empty
    if (!payloadRaw_.empty())
    {
        try
        {
            std::istringstream stream(
                std::string(reinterpret_cast<const char*>(payloadRaw_.Data()), payloadRaw_.Size()));
            io::BinaryReader reader(stream);
            payload_ = p2p::PayloadFactory::DeserializePayload(command_, reader);
        }
        catch (const std::exception& ex)
        {
            // Log error
            std::cerr << "Failed to deserialize payload: " << ex.what() << std::endl;
            payload_ = nullptr;
        }
    }
}

p2p::MessageFlags Message::GetFlags() const
{
    return flags_;
}

p2p::MessageCommand Message::GetCommand() const
{
    return command_;
}

std::shared_ptr<p2p::IPayload> Message::GetPayload() const
{
    return payload_;
}

const io::ByteVector& Message::GetRawPayload() const
{
    return payloadRaw_;
}

void Message::SetPayload(std::shared_ptr<p2p::IPayload> payload)
{
    payload_ = payload;

    if (payload)
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        payload->Serialize(writer);
        std::string data = stream.str();
        payloadRaw_ = io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));

        // Reset flags
        flags_ = ClearFlag(flags_, p2p::MessageFlags::Compressed);

        // Determine if compression should be used
        if (ShouldCompress(command_) && payloadRaw_.Size() > CompressionMinSize)
        {
            io::ByteVector compressed = cryptography::LZ4::Compress(payloadRaw_.AsSpan());

            if (compressed.Size() < payloadRaw_.Size() - CompressionThreshold)
            {
                payloadCompressed_ = compressed;
                flags_ = SetFlag(flags_, p2p::MessageFlags::Compressed);
                return;
            }
        }

        // No compression or compression was not effective
        payloadCompressed_ = payloadRaw_;
    }
    else
    {
        // Clear the payloads if the payload is null
        payloadRaw_.Clear();
        payloadCompressed_.Clear();
    }
}

uint32_t Message::GetSize() const
{
    // Follow C# implementation: payload size + 3 bytes (1 for flags, 2 for command + varsize encapsulation)
    return (IsCompressed() ? payloadCompressed_.Size() : payloadRaw_.Size()) + 3;
}

bool Message::IsCompressed() const
{
    return HasFlag(flags_, p2p::MessageFlags::Compressed);
}

Message Message::Create(p2p::MessageCommand command, std::shared_ptr<p2p::IPayload> payload)
{
    // Create a new message with the given command and payload
    // Compression will be applied in the constructor if needed
    return Message(command, payload);
}

io::ByteVector Message::ToArray(uint32_t networkMagic) const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);

    // Magic (4 bytes)
    writer.Write(networkMagic);

    // Command (12 bytes, null-padded)
    std::string commandName = p2p::GetCommandName(command_);
    char commandBytes[12] = {0};
    std::strncpy(commandBytes, commandName.c_str(), std::min(commandName.size(), size_t(12)));
    writer.Write(io::ByteSpan(reinterpret_cast<const uint8_t*>(commandBytes), 12));

    // Payload size (4 bytes)
    const io::ByteVector& payload = IsCompressed() ? payloadCompressed_ : payloadRaw_;
    writer.Write(static_cast<uint32_t>(payload.Size()));

    // Checksum (4 bytes)
    uint32_t checksum = 0;
    if (payload.Size() > 0)
    {
        auto hash = cryptography::Crypto::Hash256(payload.Data(), payload.Size());
        // First 4 bytes of the hash
        checksum = static_cast<uint32_t>(hash[0]) | (static_cast<uint32_t>(hash[1]) << 8) |
                   (static_cast<uint32_t>(hash[2]) << 16) | (static_cast<uint32_t>(hash[3]) << 24);
    }
    writer.Write(checksum);

    // Flags (1 byte)
    writer.Write(static_cast<uint8_t>(flags_));

    // Payload
    if (payload.Size() > 0)
    {
        writer.WriteBytes(payload);
    }

    // Convert to ByteVector
    std::string data = stream.str();
    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

bool Message::FromArray(const io::ByteVector& data, uint32_t networkMagic)
{
    try
    {
        std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
        io::BinaryReader reader(stream);

        // Magic (4 bytes)
        uint32_t magic = reader.ReadUInt32();
        if (magic != networkMagic)
        {
            return false;  // Invalid magic
        }

        // Command (12 bytes)
        char commandBytes[12];
        reader.ReadBytes(reinterpret_cast<uint8_t*>(commandBytes), 12);
        std::string commandStr;
        for (int i = 0; i < 12 && commandBytes[i] != 0; i++)
        {
            commandStr.push_back(commandBytes[i]);
        }

        // Convert string to command enum (exact match with C# implementation)
        if (commandStr == "version")
            command_ = p2p::MessageCommand::Version;
        else if (commandStr == "verack")
            command_ = p2p::MessageCommand::Verack;
        else if (commandStr == "getaddr")
            command_ = p2p::MessageCommand::GetAddr;
        else if (commandStr == "addr")
            command_ = p2p::MessageCommand::Addr;
        else if (commandStr == "inv")
            command_ = p2p::MessageCommand::Inv;
        else if (commandStr == "getdata")
            command_ = p2p::MessageCommand::GetData;
        else if (commandStr == "getblocks")
            command_ = p2p::MessageCommand::GetBlocks;
        else if (commandStr == "getblockbyindex")
            command_ = p2p::MessageCommand::GetBlockByIndex;
        else if (commandStr == "getheaders")
            command_ = p2p::MessageCommand::GetHeaders;
        else if (commandStr == "tx")
            command_ = p2p::MessageCommand::Transaction;
        else if (commandStr == "block")
            command_ = p2p::MessageCommand::Block;
        else if (commandStr == "headers")
            command_ = p2p::MessageCommand::Headers;
        else if (commandStr == "ping")
            command_ = p2p::MessageCommand::Ping;
        else if (commandStr == "pong")
            command_ = p2p::MessageCommand::Pong;
        else if (commandStr == "merkleblock")
            command_ = p2p::MessageCommand::MerkleBlock;
        else if (commandStr == "filterload")
            command_ = p2p::MessageCommand::FilterLoad;
        else if (commandStr == "filteradd")
            command_ = p2p::MessageCommand::FilterAdd;
        else if (commandStr == "filterclear")
            command_ = p2p::MessageCommand::FilterClear;
        else if (commandStr == "reject")
            command_ = p2p::MessageCommand::Reject;
        else if (commandStr == "alert")
            command_ = p2p::MessageCommand::Alert;
        else if (commandStr == "extensible")
            command_ = p2p::MessageCommand::Extensible;
        else
            command_ = p2p::MessageCommand::Unknown;

        // Payload size (4 bytes)
        uint32_t payloadSize = reader.ReadUInt32();

        // Checksum (4 bytes)
        uint32_t checksum = reader.ReadUInt32();

        // Flags (1 byte)
        flags_ = static_cast<p2p::MessageFlags>(reader.ReadUInt8());

        // Payload
        if (payloadSize > 0)
        {
            if (payloadSize > PayloadMaxSize)
            {
                return false;  // Payload too large
            }

            payloadCompressed_ = reader.ReadBytes(payloadSize);

            // Verify checksum
            uint32_t calculatedChecksum = 0;
            if (payloadCompressed_.Size() > 0)
            {
                auto hash = cryptography::Crypto::Hash256(payloadCompressed_.Data(), payloadCompressed_.Size());
                // First 4 bytes of the hash
                calculatedChecksum = static_cast<uint32_t>(hash[0]) | (static_cast<uint32_t>(hash[1]) << 8) |
                                     (static_cast<uint32_t>(hash[2]) << 16) | (static_cast<uint32_t>(hash[3]) << 24);
            }

            if (checksum != calculatedChecksum)
            {
                return false;  // Invalid checksum
            }

            // Decompress payload if necessary
            DecompressPayload();
        }
        else
        {
            payloadRaw_.Clear();
            payloadCompressed_.Clear();
            payload_ = nullptr;
        }

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void Message::Serialize(io::BinaryWriter& writer) const
{
    // First byte: flags
    writer.Write(static_cast<uint8_t>(flags_));

    // Second byte: command
    writer.Write(static_cast<uint8_t>(command_));

    // Payload - write as variable length bytes
    if (IsCompressed())
    {
        writer.WriteVarBytes(payloadCompressed_.AsSpan());
    }
    else
    {
        writer.WriteVarBytes(payloadRaw_.AsSpan());
    }
}

void Message::Deserialize(io::BinaryReader& reader)
{
    flags_ = static_cast<p2p::MessageFlags>(reader.ReadUInt8());
    command_ = static_cast<p2p::MessageCommand>(reader.ReadUInt8());

    // Read payload as variable length bytes with maximum size limit
    payloadCompressed_ = reader.ReadVarBytes(PayloadMaxSize);

    // Decompress if necessary
    DecompressPayload();
}

void Message::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("flags", static_cast<uint8_t>(flags_));
    writer.Write("command", static_cast<uint8_t>(command_));

    if (payload_)
    {
        writer.WritePropertyName("payload");
        writer.WriteStartObject();
        payload_->SerializeJson(writer);
        writer.WriteEndObject();
    }
    else
    {
        writer.Write("payload", nlohmann::json(nullptr));
    }
}

void Message::DeserializeJson(const io::JsonReader& reader)
{
    flags_ = static_cast<p2p::MessageFlags>(reader.ReadUInt8("flags"));
    command_ = static_cast<p2p::MessageCommand>(reader.ReadUInt8("command"));

    // In C# neo implementation, payload is not fully deserialized from JSON
    // We follow the same pattern
    payload_ = nullptr;
}

void Message::DecompressPayload()
{
    if (payloadCompressed_.empty())
        return;

    if (IsCompressed())
    {
        payloadRaw_ = cryptography::LZ4::Decompress(payloadCompressed_.AsSpan(), PayloadMaxSize);
    }
    else
    {
        payloadRaw_ = payloadCompressed_;
    }

    // Create payload based on command
    try
    {
        std::istringstream stream(std::string(reinterpret_cast<const char*>(payloadRaw_.Data()), payloadRaw_.Size()));
        io::BinaryReader reader(stream);
        payload_ = p2p::PayloadFactory::DeserializePayload(command_, reader);
    }
    catch (const std::exception& ex)
    {
        // Log error
        std::cerr << "Failed to deserialize payload: " << ex.what() << std::endl;
        payload_ = nullptr;
    }
}

bool Message::ShouldCompress(p2p::MessageCommand command)
{
    // Match C# implementation exactly
    switch (command)
    {
        case p2p::MessageCommand::Block:
        case p2p::MessageCommand::Transaction:
        case p2p::MessageCommand::Extensible:
        case p2p::MessageCommand::Headers:
        case p2p::MessageCommand::Inv:
        case p2p::MessageCommand::Addr:
        case p2p::MessageCommand::MerkleBlock:
        case p2p::MessageCommand::FilterLoad:
        case p2p::MessageCommand::FilterAdd:
            return true;
        default:
            return false;
    }
}
}  // namespace neo::network
