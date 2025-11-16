/**
 * @file network_address_with_time.cpp
 * @brief Network Address With Time payload
 */

#include <neo/network/p2p/payloads/network_address_with_time.h>
#include <neo/network/p2p/payloads/version_payload.h>

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>

#include <algorithm>
#include <array>
#include <vector>

namespace neo::network::p2p::payloads
{
namespace
{
std::array<uint8_t, NetworkAddressWithTime::AddressSize> MapToIPv6(const network::IPAddress& address)
{
    std::array<uint8_t, NetworkAddressWithTime::AddressSize> bytes{};
    bytes.fill(0);
    auto length = address.GetAddressLength();
    const auto* addrBytes = address.GetAddressBytes();

    if (length == 16)
    {
        std::copy(addrBytes, addrBytes + 16, bytes.begin());
    }
    else if (length == 4)
    {
        bytes[10] = 0xff;
        bytes[11] = 0xff;
        std::copy(addrBytes, addrBytes + 4, bytes.begin() + 12);
    }
    else
    {
        throw std::invalid_argument("Unsupported IP address length");
    }

    return bytes;
}

network::IPAddress UnmapIPv6(const std::array<uint8_t, NetworkAddressWithTime::AddressSize>& bytes)
{
    bool isMappedIPv4 = true;
    for (int i = 0; i < 10; ++i)
    {
        if (bytes[i] != 0)
        {
            isMappedIPv4 = false;
            break;
        }
    }
    if (isMappedIPv4 && bytes[10] == 0xff && bytes[11] == 0xff)
    {
        return network::IPAddress(bytes.data() + 12, 4);
    }
    return network::IPAddress(bytes.data(), 16);
}
size_t GetVarIntSize(uint64_t value)
{
    if (value < 0xFD) return 1;
    if (value <= 0xFFFF) return 3;
    if (value <= 0xFFFFFFFF) return 5;
    return 9;
}

size_t GetCapabilitySerializedSize(const NodeCapability& capability)
{
    size_t size = 1;  // type byte
    switch (capability.GetType())
    {
        case NodeCapabilityType::TcpServer:
        case NodeCapabilityType::WsServer:
            size += sizeof(uint16_t);
            break;
        case NodeCapabilityType::FullNode:
            size += sizeof(uint32_t);
            break;
        case NodeCapabilityType::DisableCompression:
        case NodeCapabilityType::ArchivalNode:
            size += 1;  // single zero byte
            break;
        default:
            size += capability.GetData().GetVarSize();
            break;
    }
    return size;
}

size_t GetCapabilitiesSerializedLength(const std::vector<NodeCapability>& capabilities)
{
    size_t size = GetVarIntSize(capabilities.size());
    for (const auto& capability : capabilities)
    {
        size += GetCapabilitySerializedSize(capability);
    }
    return size;
}

}  // namespace

NetworkAddressWithTime::NetworkAddressWithTime() : timestamp_(0), address_(network::IPAddress::Any()) {}

NetworkAddressWithTime::NetworkAddressWithTime(uint32_t timestamp, const network::IPAddress& address,
                                               const std::vector<NodeCapability>& capabilities)
    : timestamp_(timestamp), address_(address), capabilities_(capabilities)
{
}

uint32_t NetworkAddressWithTime::GetTimestamp() const { return timestamp_; }

void NetworkAddressWithTime::SetTimestamp(uint32_t timestamp) { timestamp_ = timestamp; }

const network::IPAddress& NetworkAddressWithTime::GetIPAddress() const { return address_; }

std::string NetworkAddressWithTime::GetAddress() const { return address_.ToString(); }

void NetworkAddressWithTime::SetAddress(const network::IPAddress& address) { address_ = address; }

void NetworkAddressWithTime::SetAddress(const std::string& address) { address_ = network::IPAddress(address); }

const std::vector<NodeCapability>& NetworkAddressWithTime::GetCapabilities() const { return capabilities_; }

void NetworkAddressWithTime::SetCapabilities(const std::vector<NodeCapability>& capabilities)
{
    capabilities_ = capabilities;
}

NodeCapability* NetworkAddressWithTime::FindTcpCapability()
{
    auto it = std::find_if(capabilities_.begin(), capabilities_.end(), [](const NodeCapability& capability)
                           { return capability.GetType() == NodeCapabilityType::TcpServer; });
    if (it == capabilities_.end()) return nullptr;
    return &(*it);
}

const NodeCapability* NetworkAddressWithTime::FindTcpCapability() const
{
    auto it = std::find_if(capabilities_.begin(), capabilities_.end(), [](const NodeCapability& capability)
                           { return capability.GetType() == NodeCapabilityType::TcpServer; });
    if (it == capabilities_.end()) return nullptr;
    return &(*it);
}

uint16_t NetworkAddressWithTime::GetPort() const
{
    if (const auto* capability = FindTcpCapability())
    {
        return capability->GetPort();
    }
    return 0;
}

void NetworkAddressWithTime::SetPort(uint16_t port)
{
    if (auto* capability = FindTcpCapability())
    {
        capability->SetPort(port);
        return;
    }

    NodeCapability tcpCapability(NodeCapabilityType::TcpServer);
    tcpCapability.SetPort(port);
    capabilities_.push_back(tcpCapability);
}

size_t NetworkAddressWithTime::GetSize() const
{
    return sizeof(uint32_t) + AddressSize + GetCapabilitiesSerializedLength(capabilities_);
}

void NetworkAddressWithTime::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(timestamp_);
    auto bytes = MapToIPv6(address_);
    writer.WriteBytes(bytes.data(), AddressSize);
    writer.WriteVarArray(capabilities_);
}

void NetworkAddressWithTime::Deserialize(io::BinaryReader& reader)
{
    timestamp_ = reader.ReadUInt32();
    auto raw = reader.ReadBytes(AddressSize);
    std::array<uint8_t, AddressSize> bytes{};
    std::copy(raw.begin(), raw.end(), bytes.begin());
    address_ = UnmapIPv6(bytes);

    auto count = reader.ReadVarInt(VersionPayload::MaxCapabilities);
    capabilities_.clear();
    capabilities_.reserve(count);
    for (uint64_t i = 0; i < count; ++i)
    {
        NodeCapability capability;
        capability.Deserialize(reader);
        capabilities_.push_back(capability);
    }
}

void NetworkAddressWithTime::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("timestamp", timestamp_);
    writer.WriteProperty("address", GetAddress());
    writer.WriteProperty("port", GetPort());
    writer.WritePropertyName("capabilities");
    writer.WriteStartArray();
    for (const auto& capability : capabilities_)
    {
        capability.SerializeJson(writer);
    }
    writer.WriteEndArray();
    writer.WriteEndObject();
}

void NetworkAddressWithTime::DeserializeJson(const io::JsonReader& reader)
{
    const auto& json = reader.GetJson();
    if (json.contains("timestamp"))
    {
        timestamp_ = json["timestamp"].get<uint32_t>();
    }
    if (json.contains("address"))
    {
        SetAddress(json["address"].get<std::string>());
    }
    capabilities_.clear();
    bool readCapabilities = false;
    if (json.contains("capabilities") && json["capabilities"].is_array())
    {
        for (const auto& entry : json["capabilities"])
        {
            NodeCapability capability;
            io::JsonReader capReader(entry);
            capability.DeserializeJson(capReader);
            capabilities_.push_back(capability);
        }
        readCapabilities = true;
    }
    if (!readCapabilities && json.contains("port"))
    {
        SetPort(json["port"].get<uint16_t>());
    }
    else if (readCapabilities && json.contains("port"))
    {
        // Ensure TCP capability reflects explicit port override
        SetPort(json["port"].get<uint16_t>());
    }
}

bool NetworkAddressWithTime::operator==(const NetworkAddressWithTime& other) const
{
    return timestamp_ == other.timestamp_ && address_ == other.address_ && capabilities_ == other.capabilities_;
}

bool NetworkAddressWithTime::operator!=(const NetworkAddressWithTime& other) const { return !(*this == other); }

std::array<uint8_t, NetworkAddressWithTime::AddressSize> NetworkAddressWithTime::ToIPv6Bytes() const
{
    return MapToIPv6(address_);
}

void NetworkAddressWithTime::FromIPv6Bytes(const std::array<uint8_t, AddressSize>& bytes)
{
    address_ = UnmapIPv6(bytes);
}

NetworkAddressWithTime NetworkAddressWithTime::FromIPv4(uint32_t timestamp, const std::string& address, uint16_t port,
                                                        const std::vector<NodeCapability>& capabilities)
{
    NetworkAddressWithTime result(timestamp, network::IPAddress(address), capabilities);
    if (port != 0)
    {
        result.SetPort(port);
    }
    return result;
}

NetworkAddressWithTime NetworkAddressWithTime::FromIPv6(uint32_t timestamp, const std::string& address, uint16_t port,
                                                        const std::vector<NodeCapability>& capabilities)
{
    NetworkAddressWithTime result(timestamp, network::IPAddress(address), capabilities);
    if (port != 0)
    {
        result.SetPort(port);
    }
    return result;
}
}  // namespace neo::network::p2p::payloads
