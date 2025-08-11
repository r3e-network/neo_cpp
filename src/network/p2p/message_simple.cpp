#include <neo/core/logging.h>
#include <neo/io/binary_writer.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/ipayload.h>

#include <array>
#include <cstring>
#include <sstream>

namespace neo::network::p2p
{

Message::Message() : flags_(MessageFlags::None), command_(MessageCommand::Version) {}

Message::Message(MessageCommand command, std::shared_ptr<IPayload> payload)
    : flags_(MessageFlags::None), command_(command), payload_(payload)
{
}

MessageFlags Message::GetFlags() const { return flags_; }
MessageCommand Message::GetCommand() const { return command_; }
std::shared_ptr<IPayload> Message::GetPayload() const { return payload_; }
uint32_t Message::GetSize() const { return 24; }
bool Message::IsCompressed() const { return false; }

Message Message::Create(MessageCommand command, std::shared_ptr<IPayload> payload) { return Message(command, payload); }

void Message::Serialize(io::BinaryWriter& writer) const
{
    // Neo protocol: magic(4) + command(12) + length(4) + checksum(4) + payload

    // 1. Magic number for Neo N3 mainnet
    writer.Write(static_cast<uint32_t>(0x334F454E));

    // 2. Command string padded to 12 bytes
    std::string commandStr = GetCommandString(command_);
    std::array<uint8_t, 12> commandBytes = {};
    for (size_t i = 0; i < std::min(commandStr.length(), size_t(12)); ++i)
    {
        commandBytes[i] = static_cast<uint8_t>(commandStr[i]);
    }
    for (size_t i = 0; i < 12; ++i)
    {
        writer.Write(commandBytes[i]);
    }

    // 3. Payload length
    uint32_t payloadLength = payload_ ? payload_->GetSize() : 0;
    writer.Write(payloadLength);
    
    // 4. Payload checksum
    uint32_t checksum = 0;
    if (payload_ && payloadLength > 0)
    {
        io::BinaryWriter payloadWriter;
        payload_->Serialize(payloadWriter);
        auto payloadData = payloadWriter.ToArray();
        auto hash = cryptography::Hash::Sha256(cryptography::Hash::Sha256(payloadData.AsSpan()).AsSpan());
        checksum = *reinterpret_cast<const uint32_t*>(hash.data());
    }
    writer.Write(checksum);
}

void Message::Deserialize(io::BinaryReader& /* reader */) {}
void Message::SerializeJson(io::JsonWriter& /* writer */) const {}
void Message::DeserializeJson(const io::JsonReader& /* reader */) {}

io::ByteVector Message::ToArray(bool /* enableCompression */) const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    Serialize(writer);
    std::string data = stream.str();
    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
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
        default:
            return "unknown";
    }
}

uint32_t Message::TryDeserialize(const io::ByteSpan& data, Message& message)
{
    // Minimal parser: require at least header
    if (data.Size() < 24) return 0;
    const uint8_t* p = data.Data();
    uint32_t magic = *reinterpret_cast<const uint32_t*>(p);
    p += 4;
    if (magic != 0x334F454E) return 0;
    char cmd[13] = {0};
    std::memcpy(cmd, p, 12);
    p += 12;
    uint32_t len = *reinterpret_cast<const uint32_t*>(p);
    p += 4;
    (void)*reinterpret_cast<const uint32_t*>(p);
    p += 4;  // checksum (validated at higher layer)
    if (data.Size() < 24 + len) return 0;
    // Map command string to enum
    std::string cmdStr(cmd);
    MessageCommand command = MessageCommand::Version;
    // simple mapping
    if (cmdStr == "version")
        command = MessageCommand::Version;
    else if (cmdStr == "verack")
        command = MessageCommand::Verack;
    else if (cmdStr == "addr")
        command = MessageCommand::Addr;
    else if (cmdStr == "getaddr")
        command = MessageCommand::GetAddr;
    else if (cmdStr == "ping")
        command = MessageCommand::Ping;
    else if (cmdStr == "pong")
        command = MessageCommand::Pong;
    else if (cmdStr == "inv")
        command = MessageCommand::Inv;
    else if (cmdStr == "getdata")
        command = MessageCommand::GetData;
    else if (cmdStr == "block")
        command = MessageCommand::Block;
    else if (cmdStr == "tx")
        command = MessageCommand::Transaction;
    else if (cmdStr == "getblocks")
        command = MessageCommand::GetBlocks;
    else if (cmdStr == "getheaders")
        command = MessageCommand::GetHeaders;
    else if (cmdStr == "headers")
        command = MessageCommand::Headers;
    else if (cmdStr == "getblockbyindex")
        command = MessageCommand::GetBlockByIndex;
    else if (cmdStr == "mempool")
        command = MessageCommand::Mempool;
    message.command_ = command;
    // Payload parsing is deferred to higher layers for type-specific handling
    return 24 + len;
}

}  // namespace neo::network::p2p