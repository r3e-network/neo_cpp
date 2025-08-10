#include <neo/network/p2p/node_capability.h>

namespace neo::network::p2p
{
// NodeCapability implementation
NodeCapability::NodeCapability() : type_(NodeCapabilityType::TcpServer) {}

NodeCapability::NodeCapability(NodeCapabilityType type) : type_(type) {}

NodeCapabilityType NodeCapability::GetType() const { return type_; }

void NodeCapability::SetType(NodeCapabilityType type) { type_ = type; }

void NodeCapability::Serialize(io::BinaryWriter& writer) const { writer.Write(static_cast<uint8_t>(type_)); }

void NodeCapability::Deserialize(io::BinaryReader& reader)
{
    type_ = static_cast<NodeCapabilityType>(reader.ReadUInt8());
}

void NodeCapability::SerializeJson(io::JsonWriter& writer) const { writer.Write("type", static_cast<uint8_t>(type_)); }

void NodeCapability::DeserializeJson(const io::JsonReader& reader)
{
    type_ = static_cast<NodeCapabilityType>(reader.ReadUInt8("type"));
}

// UnknownCapability implementation
UnknownCapability::UnknownCapability() : NodeCapability(NodeCapabilityType::Unknown), rawType_(0) {}

UnknownCapability::UnknownCapability(uint8_t type) : NodeCapability(NodeCapabilityType::Unknown), rawType_(type) {}

uint8_t UnknownCapability::GetRawType() const { return rawType_; }

void UnknownCapability::SetRawType(uint8_t rawType) { rawType_ = rawType; }

void UnknownCapability::Serialize(io::BinaryWriter& writer) const
{
    // Using the raw type for serialization to preserve the original value
    writer.Write(rawType_);
}

void UnknownCapability::Deserialize(io::BinaryReader& reader) { rawType_ = reader.ReadUInt8(); }

void UnknownCapability::SerializeJson(io::JsonWriter& writer) const { writer.Write("type", rawType_); }

void UnknownCapability::DeserializeJson(const io::JsonReader& reader) { rawType_ = reader.ReadUInt8("type"); }

// ServerCapability implementation
ServerCapability::ServerCapability() : NodeCapability(NodeCapabilityType::TcpServer), port_(0) {}

ServerCapability::ServerCapability(NodeCapabilityType type, uint16_t port) : NodeCapability(type), port_(port) {}

uint16_t ServerCapability::GetPort() const { return port_; }

void ServerCapability::SetPort(uint16_t port) { port_ = port; }

void ServerCapability::Serialize(io::BinaryWriter& writer) const
{
    NodeCapability::Serialize(writer);
    writer.Write(port_);
}

void ServerCapability::Deserialize(io::BinaryReader& reader)
{
    NodeCapability::Deserialize(reader);
    port_ = reader.ReadUInt16();
}

void ServerCapability::SerializeJson(io::JsonWriter& writer) const
{
    NodeCapability::SerializeJson(writer);
    writer.Write("port", port_);
}

void ServerCapability::DeserializeJson(const io::JsonReader& reader)
{
    NodeCapability::DeserializeJson(reader);
    port_ = reader.ReadUInt16("port");
}

// FullNodeCapability implementation
FullNodeCapability::FullNodeCapability() : NodeCapability(NodeCapabilityType::FullNode), startHeight_(0) {}

FullNodeCapability::FullNodeCapability(uint32_t startHeight)
    : NodeCapability(NodeCapabilityType::FullNode), startHeight_(startHeight)
{
}

uint32_t FullNodeCapability::GetStartHeight() const { return startHeight_; }

void FullNodeCapability::SetStartHeight(uint32_t startHeight) { startHeight_ = startHeight; }

void FullNodeCapability::Serialize(io::BinaryWriter& writer) const
{
    NodeCapability::Serialize(writer);
    writer.Write(startHeight_);
}

void FullNodeCapability::Deserialize(io::BinaryReader& reader)
{
    NodeCapability::Deserialize(reader);
    startHeight_ = reader.ReadUInt32();
}

void FullNodeCapability::SerializeJson(io::JsonWriter& writer) const
{
    NodeCapability::SerializeJson(writer);
    writer.Write("start_height", startHeight_);
}

void FullNodeCapability::DeserializeJson(const io::JsonReader& reader)
{
    NodeCapability::DeserializeJson(reader);
    startHeight_ = reader.ReadUInt32("start_height");
}
}  // namespace neo::network::p2p
