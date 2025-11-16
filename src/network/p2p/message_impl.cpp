/**
 * @file message_impl.cpp
 * @brief Neo N3 message serialization compatible with the C# node
 */

#include <neo/core/logging.h>
#include <neo/cryptography/lz4.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/message.h>
#include <neo/network/payload_factory.h>

#include <array>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace neo::network::p2p
{
namespace
{
size_t GetVarSize(size_t length)
{
    if (length < 0xFD)
    {
        return 1 + length;
    }
    if (length <= 0xFFFF)
    {
        return 3 + length;
    }
    if (length <= 0xFFFFFFFF)
    {
        return 5 + length;
    }
    return 9 + length;
}
}  // namespace

Message::Message() : flags_(MessageFlags::None), command_(MessageCommand::Version), payload_(nullptr) {}

Message::Message(MessageCommand command, std::shared_ptr<IPayload> payload)
    : flags_(MessageFlags::None), command_(command), payload_(payload)
{
    if (payload)
    {
        payloadRaw_ = payload->ToArray();
        payloadCompressed_ = payloadRaw_;

        if (ShouldCompress(command) && payloadRaw_.Size() >= CompressionMinSize)
        {
            CompressPayload(payloadRaw_);
        }
    }
}

MessageFlags Message::GetFlags() const { return flags_; }

MessageCommand Message::GetCommand() const { return command_; }

std::shared_ptr<IPayload> Message::GetPayload() const { return payload_; }

uint32_t Message::GetSize() const
{
    const auto& payloadData = IsCompressed() ? payloadCompressed_ : payloadRaw_;
    return static_cast<uint32_t>(sizeof(uint8_t) + sizeof(uint8_t) + GetVarSize(payloadData.Size()));
}

bool Message::IsCompressed() const { return HasFlag(flags_, MessageFlags::Compressed); }

Message Message::Create(MessageCommand command, std::shared_ptr<IPayload> payload) { return Message(command, payload); }

void Message::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(flags_));
    writer.Write(static_cast<uint8_t>(command_));
    const auto& payloadData = IsCompressed() ? payloadCompressed_ : payloadRaw_;
    writer.WriteVarBytes(payloadData.AsSpan());
}

void Message::Deserialize(io::BinaryReader& reader)
{
    flags_ = static_cast<MessageFlags>(reader.ReadUInt8());
    command_ = static_cast<MessageCommand>(reader.ReadUInt8());

    auto payloadData = reader.ReadVarBytes(PayloadMaxSize);
    payloadCompressed_ = io::ByteVector(payloadData);
    payloadRaw_.Clear();
    payload_.reset();

    if (!payloadCompressed_.IsEmpty())
    {
        DecompressPayload();
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

void Message::DeserializeJson(const io::JsonReader& /* reader */)
{
    // JSON deserialization requires payload type information and is not used here.
}

io::ByteVector Message::ToArray(bool enableCompression) const
{
    if (enableCompression || !IsCompressed())
    {
        const auto& payloadData = IsCompressed() ? payloadCompressed_ : payloadRaw_;
        return SerializeWithPayload(flags_, payloadData.AsSpan());
    }

    const auto uncompressedFlags = ClearFlag(flags_, MessageFlags::Compressed);
    return SerializeWithPayload(uncompressedFlags, payloadRaw_.AsSpan());
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
    payloadRaw_ = uncompressed;
    payloadCompressed_ = uncompressed;

    if (uncompressed.IsEmpty())
    {
        flags_ = ClearFlag(flags_, MessageFlags::Compressed);
        return;
    }

    try
    {
        auto compressed = cryptography::LZ4::Compress(uncompressed.AsSpan());
        if (compressed.Size() + CompressionThreshold < uncompressed.Size())
        {
            payloadCompressed_ = std::move(compressed);
            flags_ = SetFlag(flags_, MessageFlags::Compressed);
        }
        else
        {
            flags_ = ClearFlag(flags_, MessageFlags::Compressed);
        }
    }
    catch (const std::exception& ex)
    {
        LOG_WARNING("Failed to LZ4-compress payload: {}", ex.what());
        flags_ = ClearFlag(flags_, MessageFlags::Compressed);
        payloadCompressed_ = uncompressed;
    }
}

void Message::DecompressPayload()
{
    if (payloadCompressed_.IsEmpty())
    {
        payloadRaw_.Clear();
        payload_.reset();
        return;
    }

    if (HasFlag(flags_, MessageFlags::Compressed))
    {
        auto decompressed = cryptography::LZ4::Decompress(payloadCompressed_.AsSpan(), PayloadMaxSize);
        payloadRaw_ = std::move(decompressed);
    }
    else
    {
        payloadRaw_ = payloadCompressed_;
    }

    io::BinaryReader payloadReader(payloadRaw_.AsSpan());
    payload_ = PayloadFactory::DeserializePayload(command_, payloadReader);
}

bool Message::ShouldCompress(MessageCommand command)
{
    switch (command)
    {
        case MessageCommand::Block:
        case MessageCommand::Extensible:
        case MessageCommand::Transaction:
        case MessageCommand::Headers:
        case MessageCommand::Addr:
        case MessageCommand::MerkleBlock:
        case MessageCommand::FilterLoad:
        case MessageCommand::FilterAdd:
            return true;
        default:
            return false;
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
    static const std::array<std::pair<const char*, MessageCommand>, 22> mapping = {{
        {"version", MessageCommand::Version},
        {"verack", MessageCommand::Verack},
        {"addr", MessageCommand::Addr},
        {"getaddr", MessageCommand::GetAddr},
        {"ping", MessageCommand::Ping},
        {"pong", MessageCommand::Pong},
        {"inv", MessageCommand::Inv},
        {"getdata", MessageCommand::GetData},
        {"block", MessageCommand::Block},
        {"tx", MessageCommand::Transaction},
        {"getblocks", MessageCommand::GetBlocks},
        {"getheaders", MessageCommand::GetHeaders},
        {"headers", MessageCommand::Headers},
        {"getblockbyindex", MessageCommand::GetBlockByIndex},
        {"mempool", MessageCommand::Mempool},
        {"notfound", MessageCommand::NotFound},
        {"filterload", MessageCommand::FilterLoad},
        {"filteradd", MessageCommand::FilterAdd},
        {"filterclear", MessageCommand::FilterClear},
        {"merkleblock", MessageCommand::MerkleBlock},
        {"reject", MessageCommand::Reject},
        {"extensible", MessageCommand::Extensible},
    }};

    for (const auto& entry : mapping)
    {
        if (commandStr == entry.first)
        {
            return entry.second;
        }
    }
    throw std::invalid_argument("Unknown message command: " + commandStr);
}

io::ByteVector Message::SerializeWithPayload(MessageFlags flags, const io::ByteSpan& payload) const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.Write(static_cast<uint8_t>(flags));
    writer.Write(static_cast<uint8_t>(command_));
    writer.WriteVarBytes(payload);

    const auto data = stream.str();
    return io::ByteVector(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}
}  // namespace neo::network::p2p
