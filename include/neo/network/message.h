#pragma once

#include <neo/io/serializable.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <string>
#include <memory>

namespace neo {
namespace network {

/**
 * @brief Network message types
 */
enum class MessageType : uint8_t {
    // Handshaking
    Version = 0x00,
    Verack = 0x01,
    
    // Connectivity
    GetAddr = 0x10,
    Addr = 0x11,
    Ping = 0x18,
    Pong = 0x19,
    
    // Synchronization
    GetHeaders = 0x20,
    Headers = 0x21,
    GetBlocks = 0x24,
    Mempool = 0x25,
    Inv = 0x27,
    GetData = 0x28,
    GetBlockByIndex = 0x29,
    NotFound = 0x2a,
    Transaction = 0x2b,
    Block = 0x2c,
    Consensus = 0x2d,
    Reject = 0x2f,
    
    // SPV
    FilterLoad = 0x30,
    FilterAdd = 0x31,
    FilterClear = 0x32,
    MerkleBlock = 0x38,
    
    // Others
    Alert = 0x40
};

/**
 * @brief Base network message class
 */
class Message : public io::ISerializable {
public:
    /**
     * @brief Message type
     */
    MessageType Type;
    
    /**
     * @brief Message payload
     */
    io::ByteVector Payload;
    
    /**
     * @brief Default constructor
     */
    Message() : Type(MessageType::Ping) {}
    
    /**
     * @brief Constructor with type
     */
    explicit Message(MessageType type) : Type(type) {}
    
    /**
     * @brief Constructor with type and payload
     */
    Message(MessageType type, const io::ByteVector& payload) 
        : Type(type), Payload(payload) {}
    
    /**
     * @brief Get the command string for the message type
     */
    std::string GetCommand() const {
        switch (Type) {
            case MessageType::Version: return "version";
            case MessageType::Verack: return "verack";
            case MessageType::GetAddr: return "getaddr";
            case MessageType::Addr: return "addr";
            case MessageType::Ping: return "ping";
            case MessageType::Pong: return "pong";
            case MessageType::GetHeaders: return "getheaders";
            case MessageType::Headers: return "headers";
            case MessageType::GetBlocks: return "getblocks";
            case MessageType::Mempool: return "mempool";
            case MessageType::Inv: return "inv";
            case MessageType::GetData: return "getdata";
            case MessageType::NotFound: return "notfound";
            case MessageType::Transaction: return "tx";
            case MessageType::Block: return "block";
            case MessageType::Consensus: return "consensus";
            case MessageType::Reject: return "reject";
            default: return "unknown";
        }
    }
    
    /**
     * @brief Get the size of the serialized message
     */
    size_t Size() const override {
        return sizeof(Type) + sizeof(uint32_t) + Payload.Size();
    }
    
    /**
     * @brief Serialize the message
     */
    void Serialize(io::BinaryWriter& writer) const override {
        writer.Write(static_cast<uint8_t>(Type));
        writer.Write(static_cast<uint32_t>(Payload.Size()));
        writer.Write(Payload);
    }
    
    /**
     * @brief Deserialize the message
     */
    void Deserialize(io::BinaryReader& reader) override {
        Type = static_cast<MessageType>(reader.ReadByte());
        uint32_t size = reader.ReadUInt32();
        Payload = reader.ReadBytes(size);
    }
    
    /**
     * @brief Create a ping message
     */
    static std::shared_ptr<Message> CreatePing() {
        auto msg = std::make_shared<Message>(MessageType::Ping);
        // Add timestamp as payload
        io::BinaryWriter writer;
        writer.Write(static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()));
        msg->Payload = writer.ToByteVector();
        return msg;
    }
    
    /**
     * @brief Create a pong message in response to ping
     */
    static std::shared_ptr<Message> CreatePong(const io::ByteVector& pingPayload) {
        return std::make_shared<Message>(MessageType::Pong, pingPayload);
    }
    
    /**
     * @brief Check if message is valid
     */
    bool IsValid() const {
        // Check payload size limits
        const size_t MAX_PAYLOAD_SIZE = 32 * 1024 * 1024; // 32MB
        return Payload.Size() <= MAX_PAYLOAD_SIZE;
    }
};

} // namespace network
} // namespace neo