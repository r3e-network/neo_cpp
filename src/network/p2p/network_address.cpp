#include <neo/network/p2p/network_address.h>
#include <algorithm>

namespace neo::network::p2p
{
    NetworkAddressWithTime::NetworkAddressWithTime()
        : timestamp_(0)
    {
    }

    NetworkAddressWithTime::NetworkAddressWithTime(uint32_t timestamp, const IPAddress& address, const std::vector<NodeCapability>& capabilities)
        : timestamp_(timestamp), address_(address), capabilities_(capabilities)
    {
    }

    uint32_t NetworkAddressWithTime::GetTimestamp() const
    {
        return timestamp_;
    }

    void NetworkAddressWithTime::SetTimestamp(uint32_t timestamp)
    {
        timestamp_ = timestamp;
    }

    const IPAddress& NetworkAddressWithTime::GetAddress() const
    {
        return address_;
    }

    void NetworkAddressWithTime::SetAddress(const IPAddress& address)
    {
        address_ = address;
    }

    const std::vector<NodeCapability>& NetworkAddressWithTime::GetCapabilities() const
    {
        return capabilities_;
    }

    void NetworkAddressWithTime::SetCapabilities(const std::vector<NodeCapability>& capabilities)
    {
        capabilities_ = capabilities;
    }

    IPEndPoint NetworkAddressWithTime::GetEndPoint() const
    {
        // Find the first TcpServer capability
        auto it = std::find_if(capabilities_.begin(), capabilities_.end(), [](const NodeCapability& capability) {
            return capability.GetType() == NodeCapabilityType::TcpServer;
        });

        if (it != capabilities_.end())
        {
            const auto& serverCapability = static_cast<const ServerCapability&>(*it);
            return IPEndPoint(address_, serverCapability.GetPort());
        }

        return IPEndPoint(address_, 0);
    }

    uint32_t NetworkAddressWithTime::GetSize() const
    {
        // Timestamp (4 bytes) + Address length (1 byte) + Address bytes
        uint32_t size = sizeof(uint32_t) + 1 + address_.GetAddressLength();

        // Add capabilities size
        size += 1; // VarInt for count
        if (capabilities_.size() >= 0xFD) {
            size += 2; // Add 2 more bytes for length if count is large
        }

        // Each capability has its own size
        for (const auto& capability : capabilities_) {
            // 1 byte for type + specific fields
            size += 1;
            switch (capability.GetType()) {
                case NodeCapabilityType::TcpServer:
                case NodeCapabilityType::WsServer:
                    // Port is 2 bytes
                    size += 2;
                    break;
                case NodeCapabilityType::FullNode:
                    // StartHeight is 4 bytes
                    size += 4;
                    break;
                default:
                    break;
            }
        }

        return size;
    }

    void NetworkAddressWithTime::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(timestamp_);

        // Write the address
        writer.Write(static_cast<uint8_t>(address_.GetAddressLength()));
        writer.Write(io::ByteSpan(address_.GetAddressBytes(), address_.GetAddressLength()));

        // Write the capabilities
        writer.WriteVarInt(capabilities_.size());

        for (const auto& capability : capabilities_)
        {
            switch (capability.GetType())
            {
                case NodeCapabilityType::TcpServer:
                case NodeCapabilityType::WsServer:
                {
                    const auto& serverCapability = static_cast<const ServerCapability&>(capability);
                    serverCapability.Serialize(writer);
                    break;
                }
                case NodeCapabilityType::FullNode:
                {
                    const auto& fullNodeCapability = static_cast<const FullNodeCapability&>(capability);
                    fullNodeCapability.Serialize(writer);
                    break;
                }
                default:
                    capability.Serialize(writer);
                    break;
            }
        }
    }

    void NetworkAddressWithTime::Deserialize(io::BinaryReader& reader)
    {
        timestamp_ = reader.ReadUInt32();

        // Read the address
        uint8_t length = reader.ReadUInt8();
        io::ByteVector bytes = reader.ReadBytes(length);
        address_ = IPAddress(bytes.Data(), bytes.Size());

        // Read the capabilities
        uint64_t count = reader.ReadVarInt();
        capabilities_.clear();
        capabilities_.reserve(count);

        for (uint64_t i = 0; i < count; i++)
        {
            // Peek at the type
            uint8_t type = reader.PeekUInt8();

            switch (static_cast<NodeCapabilityType>(type))
            {
                case NodeCapabilityType::TcpServer:
                case NodeCapabilityType::WsServer:
                {
                    ServerCapability capability;
                    capability.Deserialize(reader);
                    capabilities_.push_back(capability);
                    break;
                }
                case NodeCapabilityType::FullNode:
                {
                    FullNodeCapability capability;
                    capability.Deserialize(reader);
                    capabilities_.push_back(capability);
                    break;
                }
                case NodeCapabilityType::DisableCompression:
                {
                    NodeCapability capability;
                    capability.Deserialize(reader);
                    capabilities_.push_back(capability);
                    break;
                }
                default:
                {
                    // Handle unknown capability types
                    UnknownCapability capability(type);
                    // Type byte already consumed above
                    capabilities_.push_back(capability);
                    break;
                }
            }
        }
    }

    void NetworkAddressWithTime::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("timestamp", timestamp_);
        writer.Write("address", address_.ToString());

        nlohmann::json capabilitiesArray = nlohmann::json::array();

        for (const auto& capability : capabilities_)
        {
            nlohmann::json capabilityJson = nlohmann::json::object();
            io::JsonWriter capabilityWriter(capabilityJson);

            switch (capability.GetType())
            {
                case NodeCapabilityType::TcpServer:
                case NodeCapabilityType::WsServer:
                {
                    const auto& serverCapability = static_cast<const ServerCapability&>(capability);
                    serverCapability.SerializeJson(capabilityWriter);
                    break;
                }
                case NodeCapabilityType::FullNode:
                {
                    const auto& fullNodeCapability = static_cast<const FullNodeCapability&>(capability);
                    fullNodeCapability.SerializeJson(capabilityWriter);
                    break;
                }
                default:
                    capability.SerializeJson(capabilityWriter);
                    break;
            }

            capabilitiesArray.push_back(capabilityJson);
        }

        writer.Write("capabilities", capabilitiesArray);
    }

    void NetworkAddressWithTime::DeserializeJson(const io::JsonReader& reader)
    {
        timestamp_ = reader.ReadUInt32("timestamp");
        address_ = IPAddress(reader.ReadString("address"));

        auto capabilitiesArray = reader.ReadArray("capabilities");
        capabilities_.clear();
        capabilities_.reserve(capabilitiesArray.size());

        for (const auto& capabilityJson : capabilitiesArray)
        {
            io::JsonReader capabilityReader(capabilityJson);
            uint8_t type = capabilityReader.ReadUInt8("type");

            switch (static_cast<NodeCapabilityType>(type))
            {
                case NodeCapabilityType::TcpServer:
                case NodeCapabilityType::WsServer:
                {
                    ServerCapability capability;
                    capability.DeserializeJson(capabilityReader);
                    capabilities_.push_back(capability);
                    break;
                }
                case NodeCapabilityType::FullNode:
                {
                    FullNodeCapability capability;
                    capability.DeserializeJson(capabilityReader);
                    capabilities_.push_back(capability);
                    break;
                }
                default:
                {
                    NodeCapability capability;
                    capability.DeserializeJson(capabilityReader);
                    capabilities_.push_back(capability);
                    break;
                }
            }
        }
    }
}
