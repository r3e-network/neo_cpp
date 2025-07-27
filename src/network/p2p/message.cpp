#include <algorithm>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/lz4.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/block.h>
#include <neo/ledger/neo2_transaction.h>
#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/payload_factory.h>
#include <sstream>
#include <stdexcept>

namespace neo::network::p2p
{
Message::Message() : flags_(MessageFlags::None), command_(MessageCommand::Version) {}

Message::Message(MessageCommand command, std::shared_ptr<IPayload> payload)
    : flags_(MessageFlags::None), command_(command), payload_(payload)
{
    if (payload)
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        payload->Serialize(writer);
        std::string data = stream.str();
        payloadRaw_ = io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        payloadCompressed_ = payloadRaw_;
    }
}

MessageFlags Message::GetFlags() const
{
    return flags_;
}

MessageCommand Message::GetCommand() const
{
    return command_;
}

std::shared_ptr<IPayload> Message::GetPayload() const
{
    return payload_;
}

uint32_t Message::GetSize() const
{
    return 2 + (IsCompressed() ? payloadCompressed_.Size() : payloadRaw_.Size()) + 1;
}

bool Message::IsCompressed() const
{
    return HasFlag(flags_, MessageFlags::Compressed);
}

Message Message::Create(MessageCommand command, std::shared_ptr<IPayload> payload)
{
    bool tryCompression = ShouldCompress(command);

    Message message(command, payload);

    // Try compression
    if (tryCompression && message.payloadRaw_.Size() > CompressionMinSize)
    {
        // Implement LZ4 compression
        try
        {
            io::ByteVector compressed = cryptography::LZ4::Compress(message.payloadRaw_.AsSpan());

            if (compressed.Size() < message.payloadRaw_.Size() - CompressionThreshold)
            {
                message.payloadCompressed_ = compressed;
                message.flags_ = SetFlag(message.flags_, MessageFlags::Compressed);
            }
        }
        catch (const std::exception&)
        {
            // If compression fails, just use the raw payload
            message.payloadCompressed_ = message.payloadRaw_;
        }
    }

    return message;
}

void Message::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(flags_));
    writer.Write(static_cast<uint8_t>(command_));

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
    flags_ = static_cast<MessageFlags>(reader.ReadUInt8());
    command_ = static_cast<MessageCommand>(reader.ReadUInt8());
    payloadCompressed_ = reader.ReadVarBytes(PayloadMaxSize);
    DecompressPayload();
}

void Message::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("flags", static_cast<uint8_t>(flags_));
    writer.Write("command", static_cast<uint8_t>(command_));

    if (payload_)
    {
        nlohmann::json payloadJson = nlohmann::json::object();
        io::JsonWriter payloadWriter(payloadJson);
        payload_->SerializeJson(payloadWriter);
        writer.Write("payload", payloadJson);
    }
    else
    {
        writer.Write("payload", nlohmann::json::object());
    }
}

void Message::DeserializeJson(const io::JsonReader& reader)
{
    flags_ = static_cast<MessageFlags>(reader.ReadUInt8("flags"));
    command_ = static_cast<MessageCommand>(reader.ReadUInt8("command"));

    // Deserialize payload based on command matching C# ReflectionCache<MessageCommand>.CreateSerializable
    if (!payloadRaw_.empty())
    {
        try
        {
            std::istringstream stream(
                std::string(reinterpret_cast<const char*>(payloadRaw_.Data()), payloadRaw_.Size()));
            io::BinaryReader binaryReader(stream);

            switch (command_)
            {
                case MessageCommand::Version:
                {
                    auto versionPayload = std::make_shared<payloads::VersionPayload>();
                    versionPayload->Deserialize(binaryReader);
                    payload_ = versionPayload;
                }
                break;

                case MessageCommand::Addr:
                {
                    auto addrPayload = std::make_shared<payloads::AddrPayload>();
                    addrPayload->Deserialize(binaryReader);
                    payload_ = addrPayload;
                }
                break;

                case MessageCommand::Ping:
                case MessageCommand::Pong:
                {
                    auto pingPayload = std::make_shared<payloads::PingPayload>();
                    pingPayload->Deserialize(binaryReader);
                    payload_ = pingPayload;
                }
                break;

                case MessageCommand::Inv:
                case MessageCommand::GetData:
                case MessageCommand::NotFound:
                {
                    auto invPayload = std::make_shared<payloads::InvPayload>();
                    invPayload->Deserialize(binaryReader);
                    payload_ = invPayload;
                }
                break;

                case MessageCommand::GetHeaders:
                case MessageCommand::GetBlockByIndex:
                {
                    auto getBlockPayload = std::make_shared<payloads::GetBlockByIndexPayload>();
                    getBlockPayload->Deserialize(binaryReader);
                    payload_ = getBlockPayload;
                }
                break;

                case MessageCommand::Headers:
                {
                    auto headersPayload = std::make_shared<payloads::HeadersPayload>();
                    headersPayload->Deserialize(binaryReader);
                    payload_ = headersPayload;
                }
                break;

                case MessageCommand::Transaction:
                {
                    // TODO: Wrap transaction in a payload type
                    // auto transaction = std::make_shared<ledger::Neo2Transaction>();
                    // transaction->Deserialize(binaryReader);
                    // payload_ = transaction;
                }
                break;

                case MessageCommand::Block:
                {
                    // TODO: Wrap block in a payload type
                    // auto block = std::make_shared<ledger::Block>();
                    // block->Deserialize(binaryReader);
                    // payload_ = block;
                }
                break;

                case MessageCommand::Extensible:
                {
                    auto extensiblePayload = std::make_shared<payloads::ExtensiblePayload>();
                    extensiblePayload->Deserialize(binaryReader);
                    // TODO: ExtensiblePayload needs to implement IPayload
                    // payload_ = extensiblePayload;
                }
                break;

                // Commands without payloads
                case MessageCommand::Verack:
                case MessageCommand::GetAddr:
                case MessageCommand::Mempool:
                case MessageCommand::Reject:
                case MessageCommand::FilterClear:
                case MessageCommand::Alert:
                    payload_ = nullptr;
                    break;

                default:
                    // Unknown command, leave payload as nullptr
                    payload_ = nullptr;
                    break;
            }
        }
        catch (...)
        {
            // If deserialization fails, leave payload as nullptr
            payload_ = nullptr;
        }
    }
    else
    {
        // No payload data
        payload_ = nullptr;
    }
}

io::ByteVector Message::ToArray(bool enableCompression) const
{
    if (enableCompression || !IsCompressed())
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        Serialize(writer);
        std::string data = stream.str();
        return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
    else
    {
        // Avoid compression
        std::ostringstream stream;
        io::BinaryWriter writer(stream);

        writer.Write(static_cast<uint8_t>(ClearFlag(flags_, MessageFlags::Compressed)));
        writer.Write(static_cast<uint8_t>(command_));
        writer.WriteVarBytes(payloadRaw_.AsSpan());

        std::string data = stream.str();
        return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
}

uint32_t Message::TryDeserialize(const io::ByteSpan& data, Message& message)
{
    try
    {
        std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
        io::BinaryReader reader(stream);
        message.Deserialize(reader);
        return static_cast<uint32_t>(stream.tellg());
    }
    catch (const std::exception&)
    {
        return 0;
    }
}

void Message::DecompressPayload()
{
    if (payloadCompressed_.IsEmpty())
        return;

    if (IsCompressed())
    {
        // Implement LZ4 decompression
        try
        {
            payloadRaw_ = cryptography::LZ4::Decompress(payloadCompressed_.AsSpan(), PayloadMaxSize);
        }
        catch (const std::exception&)
        {
            // If decompression fails, just use the compressed payload
            payloadRaw_ = payloadCompressed_;
        }
    }
    else
    {
        payloadRaw_ = payloadCompressed_;
    }

    // Deserialize payload based on command matching C# ReflectionCache<MessageCommand>.CreateSerializable
    if (!payloadRaw_.empty())
    {
        try
        {
            std::istringstream stream(
                std::string(reinterpret_cast<const char*>(payloadRaw_.Data()), payloadRaw_.Size()));
            io::BinaryReader binaryReader(stream);

            switch (command_)
            {
                case MessageCommand::Version:
                {
                    auto versionPayload = std::make_shared<payloads::VersionPayload>();
                    versionPayload->Deserialize(binaryReader);
                    payload_ = versionPayload;
                }
                break;

                case MessageCommand::Addr:
                {
                    auto addrPayload = std::make_shared<payloads::AddrPayload>();
                    addrPayload->Deserialize(binaryReader);
                    payload_ = addrPayload;
                }
                break;

                case MessageCommand::Ping:
                case MessageCommand::Pong:
                {
                    auto pingPayload = std::make_shared<payloads::PingPayload>();
                    pingPayload->Deserialize(binaryReader);
                    payload_ = pingPayload;
                }
                break;

                case MessageCommand::Inv:
                case MessageCommand::GetData:
                case MessageCommand::NotFound:
                {
                    auto invPayload = std::make_shared<payloads::InvPayload>();
                    invPayload->Deserialize(binaryReader);
                    payload_ = invPayload;
                }
                break;

                case MessageCommand::GetHeaders:
                case MessageCommand::GetBlockByIndex:
                {
                    auto getBlockPayload = std::make_shared<payloads::GetBlockByIndexPayload>();
                    getBlockPayload->Deserialize(binaryReader);
                    payload_ = getBlockPayload;
                }
                break;

                case MessageCommand::Headers:
                {
                    auto headersPayload = std::make_shared<payloads::HeadersPayload>();
                    headersPayload->Deserialize(binaryReader);
                    payload_ = headersPayload;
                }
                break;

                case MessageCommand::Transaction:
                {
                    // TODO: Wrap transaction in a payload type
                    // auto transaction = std::make_shared<ledger::Neo2Transaction>();
                    // transaction->Deserialize(binaryReader);
                    // payload_ = transaction;
                }
                break;

                case MessageCommand::Block:
                {
                    // TODO: Wrap block in a payload type
                    // auto block = std::make_shared<ledger::Block>();
                    // block->Deserialize(binaryReader);
                    // payload_ = block;
                }
                break;

                case MessageCommand::Extensible:
                {
                    auto extensiblePayload = std::make_shared<payloads::ExtensiblePayload>();
                    extensiblePayload->Deserialize(binaryReader);
                    // TODO: ExtensiblePayload needs to implement IPayload
                    // payload_ = extensiblePayload;
                }
                break;

                // Commands without payloads
                case MessageCommand::Verack:
                case MessageCommand::GetAddr:
                case MessageCommand::Mempool:
                case MessageCommand::Reject:
                case MessageCommand::FilterClear:
                case MessageCommand::Alert:
                    payload_ = nullptr;
                    break;

                default:
                    // Unknown command, leave payload as nullptr
                    payload_ = nullptr;
                    break;
            }
        }
        catch (...)
        {
            // If deserialization fails, leave payload as nullptr
            payload_ = nullptr;
        }
    }
    else
    {
        // No payload data
        payload_ = nullptr;
    }
}

bool Message::ShouldCompress(MessageCommand command)
{
    switch (command)
    {
        case MessageCommand::Block:
        case MessageCommand::Transaction:
        case MessageCommand::Extensible:
        case MessageCommand::Headers:
        case MessageCommand::Inv:
        case MessageCommand::Addr:
            return true;
        default:
            return false;
    }
}
}  // namespace neo::network::p2p
