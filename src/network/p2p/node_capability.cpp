/**
 * @file node_capability.cpp
 * @brief Node Capability
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p/node_capability.h>

#include <stdexcept>

namespace
{
constexpr size_t kMaxCapabilityDataSize = 1024;
}

namespace neo::network::p2p
{
// NodeCapability implementation
NodeCapability::NodeCapability()
    : type_(NodeCapabilityType::TcpServer), rawType_(static_cast<uint8_t>(NodeCapabilityType::TcpServer)), port_(0),
      startHeight_(0)
{
}

NodeCapability::NodeCapability(NodeCapabilityType type)
    : type_(type), rawType_(static_cast<uint8_t>(type)), port_(0), startHeight_(0)
{
}

NodeCapabilityType NodeCapability::GetType() const { return type_; }

void NodeCapability::SetType(NodeCapabilityType type)
{
    type_ = type;
    rawType_ = static_cast<uint8_t>(type);
}

uint8_t NodeCapability::GetRawType() const { return rawType_; }

void NodeCapability::SetRawType(uint8_t rawType)
{
    rawType_ = rawType;
    type_ = static_cast<NodeCapabilityType>(rawType);
}

uint16_t NodeCapability::GetPort() const { return port_; }

void NodeCapability::SetPort(uint16_t port) { port_ = port; }

uint32_t NodeCapability::GetStartHeight() const { return startHeight_; }

void NodeCapability::SetStartHeight(uint32_t startHeight) { startHeight_ = startHeight; }

const io::ByteVector& NodeCapability::GetData() const { return data_; }

void NodeCapability::SetData(const io::ByteVector& data) { data_ = data; }

void NodeCapability::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(rawType_);
    switch (type_)
    {
        case NodeCapabilityType::TcpServer:
        case NodeCapabilityType::WsServer:
            writer.Write(port_);
            break;
        case NodeCapabilityType::FullNode:
            writer.Write(startHeight_);
            break;
        case NodeCapabilityType::DisableCompression:
        case NodeCapabilityType::ArchivalNode:
            writer.Write(static_cast<uint8_t>(0));
            break;
        default:
            writer.WriteVarBytes(data_.AsSpan());
            break;
    }
}

void NodeCapability::Deserialize(io::BinaryReader& reader)
{
    rawType_ = reader.ReadUInt8();
    type_ = static_cast<NodeCapabilityType>(rawType_);
    data_.Clear();
    switch (type_)
    {
        case NodeCapabilityType::TcpServer:
        case NodeCapabilityType::WsServer:
            port_ = reader.ReadUInt16();
            break;
        case NodeCapabilityType::FullNode:
            startHeight_ = reader.ReadUInt32();
            break;
        case NodeCapabilityType::DisableCompression:
        case NodeCapabilityType::ArchivalNode:
        {
            uint8_t zero = reader.ReadUInt8();
            if (zero != 0)
            {
                throw std::runtime_error("Capability payload must be empty");
            }
            break;
        }
        default:
            data_ = io::ByteVector(reader.ReadVarBytes(kMaxCapabilityDataSize));
            break;
    }
}

void NodeCapability::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("type", rawType_);
    switch (type_)
    {
        case NodeCapabilityType::TcpServer:
        case NodeCapabilityType::WsServer:
            writer.Write("port", port_);
            break;
        case NodeCapabilityType::FullNode:
            writer.Write("start_height", startHeight_);
            break;
        case NodeCapabilityType::DisableCompression:
        case NodeCapabilityType::ArchivalNode:
            writer.Write("data", "");
            break;
        default:
            if (!data_.IsEmpty())
            {
                writer.Write("data", data_.ToHexString());
            }
            break;
    }
}

void NodeCapability::DeserializeJson(const io::JsonReader& reader)
{
    rawType_ = reader.ReadUInt8("type");
    type_ = static_cast<NodeCapabilityType>(rawType_);
    data_.Clear();
    switch (type_)
    {
        case NodeCapabilityType::TcpServer:
        case NodeCapabilityType::WsServer:
            port_ = reader.ReadUInt16("port");
            break;
        case NodeCapabilityType::FullNode:
            startHeight_ = reader.ReadUInt32("start_height");
            break;
        case NodeCapabilityType::DisableCompression:
        case NodeCapabilityType::ArchivalNode:
            break;
        default:
        {
            auto dataStr = reader.ReadString("data", "");
            if (!dataStr.empty())
            {
                data_ = io::ByteVector::FromHexString(dataStr);
            }
            break;
        }
    }
}

bool NodeCapability::operator==(const NodeCapability& other) const
{
    return type_ == other.type_ && rawType_ == other.rawType_ && port_ == other.port_ &&
           startHeight_ == other.startHeight_ && data_ == other.data_;
}

// UnknownCapability implementation
UnknownCapability::UnknownCapability() : NodeCapability(NodeCapabilityType::Unknown) {}

UnknownCapability::UnknownCapability(uint8_t type) : NodeCapability(NodeCapabilityType::Unknown) { SetRawType(type); }

uint8_t UnknownCapability::GetRawType() const { return NodeCapability::GetRawType(); }

void UnknownCapability::SetRawType(uint8_t rawType) { NodeCapability::SetRawType(rawType); }

void UnknownCapability::Serialize(io::BinaryWriter& writer) const { NodeCapability::Serialize(writer); }

void UnknownCapability::Deserialize(io::BinaryReader& reader) { NodeCapability::Deserialize(reader); }

void UnknownCapability::SerializeJson(io::JsonWriter& writer) const { NodeCapability::SerializeJson(writer); }

void UnknownCapability::DeserializeJson(const io::JsonReader& reader) { NodeCapability::DeserializeJson(reader); }

// ServerCapability implementation
ServerCapability::ServerCapability() : NodeCapability(NodeCapabilityType::TcpServer) {}

ServerCapability::ServerCapability(NodeCapabilityType type, uint16_t port) : NodeCapability(type)
{
    SetPort(port);
}

uint16_t ServerCapability::GetPort() const { return NodeCapability::GetPort(); }

void ServerCapability::SetPort(uint16_t port) { NodeCapability::SetPort(port); }

void ServerCapability::Serialize(io::BinaryWriter& writer) const { NodeCapability::Serialize(writer); }

void ServerCapability::Deserialize(io::BinaryReader& reader) { NodeCapability::Deserialize(reader); }

void ServerCapability::SerializeJson(io::JsonWriter& writer) const { NodeCapability::SerializeJson(writer); }

void ServerCapability::DeserializeJson(const io::JsonReader& reader) { NodeCapability::DeserializeJson(reader); }

// FullNodeCapability implementation
FullNodeCapability::FullNodeCapability() : NodeCapability(NodeCapabilityType::FullNode) {}

FullNodeCapability::FullNodeCapability(uint32_t startHeight) : NodeCapability(NodeCapabilityType::FullNode)
{
    SetStartHeight(startHeight);
}

uint32_t FullNodeCapability::GetStartHeight() const { return NodeCapability::GetStartHeight(); }

void FullNodeCapability::SetStartHeight(uint32_t startHeight) { NodeCapability::SetStartHeight(startHeight); }

void FullNodeCapability::Serialize(io::BinaryWriter& writer) const { NodeCapability::Serialize(writer); }

void FullNodeCapability::Deserialize(io::BinaryReader& reader) { NodeCapability::Deserialize(reader); }

void FullNodeCapability::SerializeJson(io::JsonWriter& writer) const { NodeCapability::SerializeJson(writer); }

void FullNodeCapability::DeserializeJson(const io::JsonReader& reader) { NodeCapability::DeserializeJson(reader); }
}  // namespace neo::network::p2p
