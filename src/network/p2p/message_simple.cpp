#include <neo/core/logging.h>
#include <neo/network/p2p/message.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <array>

namespace neo::network::p2p
{

Message::Message() : flags_(MessageFlags::None), command_(MessageCommand::Version) {}

Message::Message(MessageCommand command, std::shared_ptr<IPayload> payload)
    : flags_(MessageFlags::None), command_(command), payload_(payload) {}

MessageFlags Message::GetFlags() const { return flags_; }
MessageCommand Message::GetCommand() const { return command_; }
std::shared_ptr<IPayload> Message::GetPayload() const { return payload_; }
uint32_t Message::GetSize() const { return 24; } // Header size
bool Message::IsCompressed() const { return false; }

Message Message::Create(MessageCommand command, std::shared_ptr<IPayload> payload)
{
    return Message(command, payload);
}

void Message::Serialize(io::BinaryWriter& writer) const
{
    // Neo protocol: magic(4) + command(12) + length(4) + checksum(4) + payload
    
    // 1. Magic number for Neo N3 mainnet
    writer.Write(static_cast<uint32_t>(0x334F454E));
    
    // 2. Command string padded to 12 bytes
    std::string commandStr = GetCommandString(command_);
    std::array<uint8_t, 12> commandBytes = {};
    for (size_t i = 0; i < std::min(commandStr.length(), size_t(12)); ++i) {
        commandBytes[i] = static_cast<uint8_t>(commandStr[i]);
    }
    for (size_t i = 0; i < 12; ++i) {
        writer.Write(commandBytes[i]);
    }
    
    // 3. Payload length (0 for now)
    writer.Write(static_cast<uint32_t>(0));
    
    // 4. Payload checksum (0 for empty payload)
    writer.Write(static_cast<uint32_t>(0));
    
    // No payload data for now
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
    switch (command) {
        case MessageCommand::Version: return "version";
        case MessageCommand::Verack: return "verack";
        case MessageCommand::Addr: return "addr";
        case MessageCommand::GetAddr: return "getaddr";
        case MessageCommand::Ping: return "ping";
        case MessageCommand::Pong: return "pong";
        case MessageCommand::Inv: return "inv";
        case MessageCommand::GetData: return "getdata";
        case MessageCommand::Block: return "block";
        case MessageCommand::Transaction: return "tx";
        case MessageCommand::GetBlocks: return "getblocks";
        case MessageCommand::GetHeaders: return "getheaders";
        case MessageCommand::Headers: return "headers";
        case MessageCommand::GetBlockByIndex: return "getblockbyindex";
        case MessageCommand::Mempool: return "mempool";
        default: return "unknown";
    }
}

uint32_t Message::TryDeserialize(const io::ByteSpan& /* data */, Message& /* message */)
{
    return 0; // Stub for now
}

} // namespace neo::network::p2p