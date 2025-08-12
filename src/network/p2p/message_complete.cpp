#include <neo/core/logging.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/ipayload.h>
#include <neo/network/payload_factory.h>
#include <zlib.h>

#include <array>
#include <cstring>
#include <sstream>

namespace neo::network::p2p
{

// Neo N3 Network Magic Numbers
static constexpr uint32_t MAINNET_MAGIC = 0x334F454E;  // "NEO3"
static constexpr uint32_t TESTNET_MAGIC = 0x4E454F54;  // "TEON"

Message::Message() : flags_(MessageFlags::None), command_(MessageCommand::Version), payload_(nullptr) {}

Message::Message(MessageCommand command, std::shared_ptr<IPayload> payload)
    : flags_(MessageFlags::None), command_(command), payload_(payload)
{
    // Auto-compress if payload is large enough
    if (payload && ShouldCompress(command))
    {
        io::ByteVector payloadData = payload->ToArray();
        if (payloadData.Size() >= CompressionMinSize)
        {
            CompressPayload(payloadData);
        }
        else
        {
            payloadRaw_ = std::move(payloadData);
        }
    }
    else if (payload)
    {
        payloadRaw_ = payload->ToArray();
    }
}

MessageFlags Message::GetFlags() const { return flags_; }

MessageCommand Message::GetCommand() const { return command_; }

std::shared_ptr<IPayload> Message::GetPayload() const { return payload_; }

uint32_t Message::GetSize() const
{
    // Header size: magic(4) + command(12) + length(4) + checksum(4) = 24
    uint32_t headerSize = 24;
    uint32_t payloadSize = IsCompressed() ? payloadCompressed_.Size() : payloadRaw_.Size();
    return headerSize + payloadSize;
}

bool Message::IsCompressed() const { return (flags_ & MessageFlags::Compressed) != MessageFlags::None; }

Message Message::Create(MessageCommand command, std::shared_ptr<IPayload> payload) { return Message(command, payload); }

void Message::Serialize(io::BinaryWriter& writer) const
{
    // 1. Write magic number (using mainnet by default)
    writer.Write(MAINNET_MAGIC);

    // 2. Write command (12 bytes, null-padded)
    std::string commandStr = GetCommandString(command_);
    std::array<uint8_t, 12> commandBytes = {};
    std::memcpy(commandBytes.data(), commandStr.c_str(), std::min(commandStr.length(), size_t(12)));
    writer.Write(commandBytes.data(), 12);

    // 3. Determine payload data
    io::ByteSpan payloadData;
    if (IsCompressed())
    {
        payloadData = payloadCompressed_.AsSpan();
    }
    else
    {
        payloadData = payloadRaw_.AsSpan();
    }

    // 4. Write payload length
    writer.Write(static_cast<uint32_t>(payloadData.Size()));

    // 5. Write payload checksum
    uint32_t checksum = CalculatePayloadChecksum(payloadData);
    writer.Write(checksum);

    // 6. Write payload data
    if (payloadData.Size() > 0)
    {
        writer.Write(payloadData.Data(), payloadData.Size());
    }
}

void Message::Deserialize(io::BinaryReader& reader)
{
    // 1. Read and verify magic number
    uint32_t magic = reader.ReadUInt32();
    if (magic != MAINNET_MAGIC && magic != TESTNET_MAGIC)
    {
        throw std::runtime_error("Invalid network magic number");
    }

    // 2. Read command
    std::array<uint8_t, 12> commandBytes;
    reader.Read(commandBytes.data(), 12);

    // Extract command string (null-terminated)
    std::string commandStr;
    for (uint8_t byte : commandBytes)
    {
        if (byte == 0) break;
        commandStr += static_cast<char>(byte);
    }
    command_ = GetCommandFromString(commandStr);

    // 3. Read payload length
    uint32_t payloadLength = reader.ReadUInt32();
    if (payloadLength > PayloadMaxSize)
    {
        throw std::runtime_error("Payload size exceeds maximum");
    }

    // 4. Read checksum
    uint32_t expectedChecksum = reader.ReadUInt32();

    // 5. Read payload data
    if (payloadLength > 0)
    {
        io::ByteVector payloadData(payloadLength);
        reader.Read(payloadData.Data(), payloadLength);

        // Verify checksum
        uint32_t actualChecksum = CalculatePayloadChecksum(payloadData.ToSpan());
        if (actualChecksum != expectedChecksum)
        {
            throw std::runtime_error("Payload checksum mismatch");
        }

        // Check if compressed
        if ((flags_ & MessageFlags::Compressed) != MessageFlags::None)
        {
            payloadCompressed_ = std::move(payloadData);
            DecompressPayload();
        }
        else
        {
            payloadRaw_ = std::move(payloadData);
        }

        // Deserialize payload object
        io::BinaryReader payloadReader(payloadRaw_.ToSpan());
        payload_ = PayloadFactory::DeserializePayload(command_, payloadReader);
    }
}

void Message::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("command", GetCommandString(command_));
    writer.WriteProperty("flags", static_cast<uint8_t>(flags_));
    writer.WriteProperty("compressed", IsCompressed());
    writer.WriteProperty("size", GetSize());

    if (payload_)
    {
        writer.WritePropertyName("payload");
        payload_->SerializeJson(writer);
    }

    writer.WriteEndObject();
}

void Message::DeserializeJson(const io::JsonReader& reader)
{
    auto obj = reader.GetObject();

    if (obj.contains("command"))
    {
        command_ = GetCommandFromString(obj["command"].get<std::string>());
    }

    if (obj.contains("flags"))
    {
        flags_ = static_cast<MessageFlags>(obj["flags"].get<uint8_t>());
    }

    // Note: Payload deserialization from JSON would require payload type information
    // This is typically not done directly from JSON in the protocol
}

io::ByteVector Message::ToArray(bool enableCompression) const
{
    // Create a copy of the message with compression if needed
    Message msg = *this;

    if (enableCompression && !IsCompressed() && ShouldCompress(command_))
    {
        if (payloadRaw_.Size() >= CompressionMinSize)
        {
            msg.CompressPayload(payloadRaw_);
        }
    }

    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    msg.Serialize(writer);

    std::string data = stream.str();
    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

uint32_t Message::TryDeserialize(const io::ByteSpan& data, Message& message)
{
    try
    {
        io::BinaryReader reader(data);
        message.Deserialize(reader);
        return reader.GetPosition();
    }
    catch (const std::exception& e)
    {
        LOG_DEBUG("Failed to deserialize message: {}", e.what());
        return 0;
    }
}

void Message::CompressPayload(const io::ByteVector& uncompressed)
{
    // Use zlib compression
    uLongf compressedSize = compressBound(uncompressed.Size());
    payloadCompressed_.Resize(compressedSize);

    int result = compress2(payloadCompressed_.Data(), &compressedSize, uncompressed.Data(), uncompressed.Size(),
                           Z_BEST_COMPRESSION);

    if (result == Z_OK)
    {
        payloadCompressed_.Resize(compressedSize);

        // Only use compression if it actually reduces size
        if (payloadCompressed_.Size() < uncompressed.Size() - CompressionThreshold)
        {
            flags_ = flags_ | MessageFlags::Compressed;
            payloadRaw_ = uncompressed;
        }
        else
        {
            // Compression didn't help, use uncompressed
            payloadCompressed_.Clear();
            payloadRaw_ = uncompressed;
        }
    }
    else
    {
        // Compression failed, use uncompressed
        payloadCompressed_.Clear();
        payloadRaw_ = uncompressed;
    }
}

void Message::DecompressPayload()
{
    if (!IsCompressed() || payloadCompressed_.IsEmpty())
    {
        return;
    }

    // Start with a reasonable buffer size
    uLongf uncompressedSize = payloadCompressed_.Size() * 4;
    payloadRaw_.Resize(uncompressedSize);

    int result =
        uncompress(payloadRaw_.Data(), &uncompressedSize, payloadCompressed_.Data(), payloadCompressed_.Size());

    if (result == Z_OK)
    {
        payloadRaw_.Resize(uncompressedSize);
        flags_ = flags_ & ~MessageFlags::Compressed;
    }
    else if (result == Z_BUF_ERROR)
    {
        // Buffer too small, try with larger size
        uncompressedSize = payloadCompressed_.Size() * 10;
        payloadRaw_.Resize(uncompressedSize);

        result =
            uncompress(payloadRaw_.Data(), &uncompressedSize, payloadCompressed_.Data(), payloadCompressed_.Size());

        if (result == Z_OK)
        {
            payloadRaw_.Resize(uncompressedSize);
            flags_ = flags_ & ~MessageFlags::Compressed;
        }
        else
        {
            throw std::runtime_error("Failed to decompress payload");
        }
    }
    else
    {
        throw std::runtime_error("Failed to decompress payload");
    }
}

bool Message::ShouldCompress(MessageCommand command)
{
    // Compress large payloads except for already compressed or small messages
    switch (command)
    {
        case MessageCommand::Version:
        case MessageCommand::Verack:
        case MessageCommand::Ping:
        case MessageCommand::Pong:
        case MessageCommand::GetAddr:
        case MessageCommand::Mempool:
        case MessageCommand::FilterClear:
            return false;  // These are small messages

        case MessageCommand::Block:
        case MessageCommand::Transaction:
        case MessageCommand::Headers:
        case MessageCommand::Addr:
        case MessageCommand::Inv:
            return true;  // These can be large

        default:
            return true;  // Compress by default for unknown messages
    }
}

std::string Message::GetCommandString(MessageCommand command)
{
    switch (command)
    {
        case MessageCommand::Version:
            return "version";
        case MessageCommand::Verack:
            return "verack";
        case MessageCommand::Addr:
            return "addr";
        case MessageCommand::GetAddr:
            return "getaddr";
        case MessageCommand::Ping:
            return "ping";
        case MessageCommand::Pong:
            return "pong";
        case MessageCommand::Inv:
            return "inv";
        case MessageCommand::GetData:
            return "getdata";
        case MessageCommand::Block:
            return "block";
        case MessageCommand::Transaction:
            return "tx";
        case MessageCommand::GetBlocks:
            return "getblocks";
        case MessageCommand::GetHeaders:
            return "getheaders";
        case MessageCommand::Headers:
            return "headers";
        case MessageCommand::GetBlockByIndex:
            return "getblockbyindex";
        case MessageCommand::Mempool:
            return "mempool";
        case MessageCommand::NotFound:
            return "notfound";
        case MessageCommand::FilterLoad:
            return "filterload";
        case MessageCommand::FilterAdd:
            return "filteradd";
        case MessageCommand::FilterClear:
            return "filterclear";
        case MessageCommand::MerkleBlock:
            return "merkleblock";
        case MessageCommand::Reject:
            return "reject";
        case MessageCommand::Alert:
            return "alert";
        case MessageCommand::Extensible:
            return "extensible";
        default:
            return "unknown";
    }
}

MessageCommand Message::GetCommandFromString(const std::string& commandStr)
{
    if (commandStr == "version") return MessageCommand::Version;
    if (commandStr == "verack") return MessageCommand::Verack;
    if (commandStr == "addr") return MessageCommand::Addr;
    if (commandStr == "getaddr") return MessageCommand::GetAddr;
    if (commandStr == "ping") return MessageCommand::Ping;
    if (commandStr == "pong") return MessageCommand::Pong;
    if (commandStr == "inv") return MessageCommand::Inv;
    if (commandStr == "getdata") return MessageCommand::GetData;
    if (commandStr == "block") return MessageCommand::Block;
    if (commandStr == "tx") return MessageCommand::Transaction;
    if (commandStr == "getblocks") return MessageCommand::GetBlocks;
    if (commandStr == "getheaders") return MessageCommand::GetHeaders;
    if (commandStr == "headers") return MessageCommand::Headers;
    if (commandStr == "getblockbyindex") return MessageCommand::GetBlockByIndex;
    if (commandStr == "mempool") return MessageCommand::Mempool;
    if (commandStr == "notfound") return MessageCommand::NotFound;
    if (commandStr == "filterload") return MessageCommand::FilterLoad;
    if (commandStr == "filteradd") return MessageCommand::FilterAdd;
    if (commandStr == "filterclear") return MessageCommand::FilterClear;
    if (commandStr == "merkleblock") return MessageCommand::MerkleBlock;
    if (commandStr == "reject") return MessageCommand::Reject;
    if (commandStr == "alert") return MessageCommand::Alert;
    if (commandStr == "extensible") return MessageCommand::Extensible;

    LOG_WARNING("Unknown message command: {}", commandStr);
    return MessageCommand::Version;  // Default fallback
}

uint32_t Message::CalculatePayloadChecksum(const io::ByteSpan& payload)
{
    if (payload.IsEmpty())
    {
        return 0;
    }

    // Neo uses double SHA256 and takes first 4 bytes
    auto hash = cryptography::Hash::Hash256(payload);
    uint32_t checksum = 0;
    std::memcpy(&checksum, hash.Data(), 4);
    return checksum;
}

}  // namespace neo::network::p2p