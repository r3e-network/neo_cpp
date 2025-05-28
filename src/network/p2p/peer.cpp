#include <neo/network/p2p/peer.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/node_capability.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <chrono>

namespace neo::network::p2p
{
    Peer::Peer()
        : version_(0), lastConnectionTime_(0), lastSeenTime_(0), connectionAttempts_(0), connected_(false), bad_(false)
    {
    }
    
    Peer::Peer(const IPEndPoint& endpoint)
        : endpoint_(endpoint), version_(0), lastConnectionTime_(0), lastSeenTime_(0), connectionAttempts_(0), connected_(false), bad_(false)
    {
    }
    
    Peer::Peer(const IPEndPoint& endpoint, uint32_t version, const std::vector<NodeCapability>& capabilities)
        : endpoint_(endpoint), version_(version), capabilities_(capabilities), lastConnectionTime_(0), lastSeenTime_(0), connectionAttempts_(0), connected_(false), bad_(false)
    {
    }
    
    const IPEndPoint& Peer::GetEndPoint() const
    {
        return endpoint_;
    }
    
    void Peer::SetEndPoint(const IPEndPoint& endpoint)
    {
        endpoint_ = endpoint;
    }
    
    uint32_t Peer::GetVersion() const
    {
        return version_;
    }
    
    void Peer::SetVersion(uint32_t version)
    {
        version_ = version;
    }
    
    const std::vector<NodeCapability>& Peer::GetCapabilities() const
    {
        return capabilities_;
    }
    
    void Peer::SetCapabilities(const std::vector<NodeCapability>& capabilities)
    {
        capabilities_ = capabilities;
    }
    
    uint64_t Peer::GetLastConnectionTime() const
    {
        return lastConnectionTime_;
    }
    
    void Peer::SetLastConnectionTime(uint64_t lastConnectionTime)
    {
        lastConnectionTime_ = lastConnectionTime;
    }
    
    uint64_t Peer::GetLastSeenTime() const
    {
        return lastSeenTime_;
    }
    
    void Peer::SetLastSeenTime(uint64_t lastSeenTime)
    {
        lastSeenTime_ = lastSeenTime;
    }
    
    uint32_t Peer::GetConnectionAttempts() const
    {
        return connectionAttempts_;
    }
    
    void Peer::SetConnectionAttempts(uint32_t connectionAttempts)
    {
        connectionAttempts_ = connectionAttempts;
    }
    
    void Peer::IncrementConnectionAttempts()
    {
        connectionAttempts_++;
    }
    
    bool Peer::IsConnected() const
    {
        return connected_;
    }
    
    void Peer::SetConnected(bool connected)
    {
        connected_ = connected;
        
        if (connected)
        {
            // Reset connection attempts
            connectionAttempts_ = 0;
            
            // Update last connection time
            lastConnectionTime_ = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
    }
    
    bool Peer::IsBad() const
    {
        return bad_;
    }
    
    void Peer::SetBad(bool bad)
    {
        bad_ = bad;
    }
    
    void Peer::Serialize(io::BinaryWriter& writer) const
    {
        // Write the endpoint
        writer.Write(endpoint_.GetAddress().ToString());
        writer.Write(endpoint_.GetPort());
        
        // Write the version
        writer.Write(version_);
        
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
        
        // Write the last connection time
        writer.Write(lastConnectionTime_);
        
        // Write the last seen time
        writer.Write(lastSeenTime_);
        
        // Write the connection attempts
        writer.Write(connectionAttempts_);
        
        // Write whether the peer is bad
        writer.Write(bad_);
    }
    
    void Peer::Deserialize(io::BinaryReader& reader)
    {
        // Read the endpoint
        std::string address = reader.ReadString();
        uint16_t port = reader.ReadUInt16();
        endpoint_ = IPEndPoint(IPAddress(address), port);
        
        // Read the version
        version_ = reader.ReadUInt32();
        
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
                default:
                {
                    NodeCapability capability;
                    capability.Deserialize(reader);
                    capabilities_.push_back(capability);
                    break;
                }
            }
        }
        
        // Read the last connection time
        lastConnectionTime_ = reader.ReadUInt64();
        
        // Read the last seen time
        lastSeenTime_ = reader.ReadUInt64();
        
        // Read the connection attempts
        connectionAttempts_ = reader.ReadUInt32();
        
        // Read whether the peer is bad
        bad_ = reader.ReadBool();
        
        // Set connected to false
        connected_ = false;
    }
    
    void Peer::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("address", endpoint_.GetAddress().ToString());
        writer.Write("port", endpoint_.GetPort());
        writer.Write("version", version_);
        
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
        writer.Write("last_connection_time", lastConnectionTime_);
        writer.Write("last_seen_time", lastSeenTime_);
        writer.Write("connection_attempts", connectionAttempts_);
        writer.Write("bad", bad_);
    }
    
    void Peer::DeserializeJson(const io::JsonReader& reader)
    {
        std::string address = reader.ReadString("address");
        uint16_t port = reader.ReadUInt16("port");
        endpoint_ = IPEndPoint(IPAddress(address), port);
        
        version_ = reader.ReadUInt32("version");
        
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
        
        lastConnectionTime_ = reader.ReadUInt64("last_connection_time");
        lastSeenTime_ = reader.ReadUInt64("last_seen_time");
        connectionAttempts_ = reader.ReadUInt32("connection_attempts");
        bad_ = reader.ReadBool("bad");
        
        connected_ = false;
    }
    
    bool Peer::operator==(const Peer& other) const
    {
        return endpoint_ == other.endpoint_;
    }
    
    bool Peer::operator!=(const Peer& other) const
    {
        return !(*this == other);
    }
}
