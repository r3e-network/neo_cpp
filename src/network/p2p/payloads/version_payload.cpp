#include <algorithm>
#include <chrono>
#include <functional>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/payloads/version_payload.h>

namespace neo::network::p2p::payloads
{
VersionPayload::VersionPayload() : network_(0), version_(0), timestamp_(0), nonce_(0), allowCompression_(true) {}

uint32_t VersionPayload::GetNetwork() const
{
    return network_;
}

void VersionPayload::SetNetwork(uint32_t network)
{
    network_ = network;
}

uint32_t VersionPayload::GetVersion() const
{
    return version_;
}

void VersionPayload::SetVersion(uint32_t version)
{
    version_ = version;
}

uint32_t VersionPayload::GetTimestamp() const
{
    return timestamp_;
}

void VersionPayload::SetTimestamp(uint32_t timestamp)
{
    timestamp_ = timestamp;
}

uint32_t VersionPayload::GetNonce() const
{
    return nonce_;
}

void VersionPayload::SetNonce(uint32_t nonce)
{
    nonce_ = nonce;
}

const std::string& VersionPayload::GetUserAgent() const
{
    return userAgent_;
}

void VersionPayload::SetUserAgent(const std::string& userAgent)
{
    userAgent_ = userAgent;
}

bool VersionPayload::GetAllowCompression() const
{
    return allowCompression_;
}

void VersionPayload::SetAllowCompression(bool allowCompression)
{
    allowCompression_ = allowCompression;
}

const std::vector<NodeCapability>& VersionPayload::GetCapabilities() const
{
    return capabilities_;
}

uint32_t VersionPayload::GetStartHeight() const
{
    for (const auto& capability : capabilities_)
    {
        if (capability.GetType() == NodeCapabilityType::FullNode)
        {
            // Cast to FullNodeCapability to access start height
            const auto* fullNodeCap = dynamic_cast<const FullNodeCapability*>(&capability);
            if (fullNodeCap)
            {
                return fullNodeCap->GetStartHeight();
            }
        }
    }
    return 0;  // Return 0 if no FullNode capability found
}

void VersionPayload::SetCapabilities(const std::vector<NodeCapability>& capabilities)
{
    capabilities_ = capabilities;
}

int VersionPayload::GetSize() const
{
    // Size calculation matching C# implementation:
    int userAgentSize = static_cast<int>(userAgent_.size() + 1);  // VarString size
    if (userAgent_.size() >= 0xFD)
    {
        userAgentSize += 2;  // Add 2 more bytes for length if string is long
    }

    int capabilitiesSize = 1;  // VarInt size for count
    if (capabilities_.size() >= 0xFD)
    {
        capabilitiesSize += 2;  // Add 2 more bytes for length if count is large
    }

    // Each capability has its own size
    for (const auto& capability : capabilities_)
    {
        // 1 byte for type + specific fields
        capabilitiesSize += 1;
        switch (capability.GetType())
        {
            case NodeCapabilityType::TcpServer:
            case NodeCapabilityType::WsServer:
                // Port is 2 bytes
                capabilitiesSize += 2;
                break;
            case NodeCapabilityType::FullNode:
                // StartHeight is 4 bytes
                capabilitiesSize += 4;
                break;
            default:
                break;
        }
    }

    return sizeof(uint32_t) +  // Network
           sizeof(uint32_t) +  // Version
           sizeof(uint32_t) +  // Timestamp
           sizeof(uint32_t) +  // Nonce
           userAgentSize +     // UserAgent
           capabilitiesSize;   // Capabilities
}

VersionPayload VersionPayload::Create(uint32_t network, uint32_t nonce, const std::string& userAgent,
                                      const std::vector<NodeCapability>& capabilities)
{
    VersionPayload payload;
    payload.SetNetwork(network);
    // Set protocol version consistent with Neo N3 specification
    const uint32_t PROTOCOL_VERSION = 0;  // Neo N3 protocol version
    payload.SetVersion(PROTOCOL_VERSION);
    payload.SetTimestamp(static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
    payload.SetNonce(nonce);
    payload.SetUserAgent(userAgent);
    payload.SetCapabilities(capabilities);

    // Compute allowCompression based on capabilities
    // !capabilities.Any(u => u is DisableCompressionCapability)
    payload.SetAllowCompression(std::none_of(capabilities.begin(), capabilities.end(), [](const NodeCapability& cap)
                                             { return cap.GetType() == NodeCapabilityType::DisableCompression; }));

    return payload;
}

void VersionPayload::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(network_);
    writer.Write(version_);
    writer.Write(timestamp_);
    writer.Write(nonce_);
    writer.WriteString(userAgent_);
    writer.WriteVarArray(capabilities_);
}

void VersionPayload::Deserialize(io::BinaryReader& reader)
{
    network_ = reader.ReadUInt32();
    version_ = reader.ReadUInt32();
    timestamp_ = reader.ReadUInt32();
    nonce_ = reader.ReadUInt32();
    userAgent_ = reader.ReadString();

    // Deserialize capabilities
    capabilities_.clear();
    capabilities_.resize(reader.ReadVarInt(MaxCapabilities));
    for (size_t i = 0; i < capabilities_.size(); i++)
    {
        NodeCapability capability;
        capability.Deserialize(reader);
        capabilities_[i] = capability;
    }

    // Compute allowCompression based on capabilities
    allowCompression_ = std::none_of(capabilities_.begin(), capabilities_.end(), [](const NodeCapability& cap)
                                     { return cap.GetType() == NodeCapabilityType::DisableCompression; });

    // Additional validation from C# implementation
    auto nonUnknownCapabilities = std::vector<NodeCapability>();
    for (const auto& cap : capabilities_)
    {
        if (cap.GetType() != NodeCapabilityType::Unknown)
        {
            nonUnknownCapabilities.push_back(cap);
        }
    }

    // Check if there are duplicate capability types (excluding Unknown)
    std::vector<NodeCapabilityType> types;
    for (const auto& cap : nonUnknownCapabilities)
    {
        types.push_back(cap.GetType());
    }

    std::sort(types.begin(), types.end());
    auto it = std::adjacent_find(types.begin(), types.end());
    if (it != types.end())
    {
        throw std::runtime_error("Duplicate capability types found");
    }
}

void VersionPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("network", network_);
    writer.Write("version", version_);
    writer.Write("timestamp", timestamp_);
    writer.Write("nonce", nonce_);
    writer.Write("userAgent", userAgent_);
    writer.Write("allowCompression", allowCompression_);
    std::function<void(io::JsonWriter&, const NodeCapability&)> capabilityWriter =
        [](io::JsonWriter& w, const NodeCapability& cap) { cap.SerializeJson(w); };
    writer.WriteArray("capabilities", capabilities_, capabilityWriter);
}

void VersionPayload::DeserializeJson(const io::JsonReader& reader)
{
    network_ = reader.ReadUInt32("network");
    version_ = reader.ReadUInt32("version");
    timestamp_ = reader.ReadUInt32("timestamp");
    nonce_ = reader.ReadUInt32("nonce");
    userAgent_ = reader.ReadString("userAgent");
    allowCompression_ = reader.ReadBool("allowCompression");

    // Read capabilities array
    auto capabilitiesJson = reader.ReadArray("capabilities");
    capabilities_.clear();
    if (capabilitiesJson.is_array())
    {
        for (const auto& capJson : capabilitiesJson)
        {
            NodeCapability cap;
            io::JsonReader capReader(capJson);
            cap.DeserializeJson(capReader);
            capabilities_.push_back(cap);
        }
    }
}
}  // namespace neo::network::p2p::payloads
