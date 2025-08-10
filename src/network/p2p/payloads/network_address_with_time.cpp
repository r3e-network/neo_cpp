#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/payloads/network_address_with_time.h>

#include <regex>
#include <sstream>
#include <stdexcept>

namespace neo::network::p2p::payloads
{
NetworkAddressWithTime::NetworkAddressWithTime() : timestamp_(0), services_(0), port_(0) { address_.fill(0); }

NetworkAddressWithTime::NetworkAddressWithTime(uint32_t timestamp, uint64_t services, const std::string& address,
                                               uint16_t port)
    : timestamp_(timestamp), services_(services), port_(port)
{
    address_.fill(0);
    ParseIPAddress(address);
}

uint32_t NetworkAddressWithTime::GetTimestamp() const { return timestamp_; }

void NetworkAddressWithTime::SetTimestamp(uint32_t timestamp) { timestamp_ = timestamp; }

uint64_t NetworkAddressWithTime::GetServices() const { return services_; }

void NetworkAddressWithTime::SetServices(uint64_t services) { services_ = services; }

std::string NetworkAddressWithTime::GetAddress() const { return FormatIPAddress(); }

void NetworkAddressWithTime::SetAddress(const std::string& address) { ParseIPAddress(address); }

const std::array<uint8_t, 16>& NetworkAddressWithTime::GetAddressBytes() const { return address_; }

void NetworkAddressWithTime::SetAddressBytes(const std::array<uint8_t, 16>& address) { address_ = address; }

uint16_t NetworkAddressWithTime::GetPort() const { return port_; }

void NetworkAddressWithTime::SetPort(uint16_t port) { port_ = port; }

std::string NetworkAddressWithTime::GetEndpoint() const
{
    std::string addr = GetAddress();
    if (IsIPv6())
    {
        return "[" + addr + "]:" + std::to_string(port_);
    }
    else
    {
        return addr + ":" + std::to_string(port_);
    }
}

bool NetworkAddressWithTime::IsIPv4() const
{
    // Check if this is an IPv4-mapped IPv6 address
    // IPv4-mapped IPv6 addresses have the form ::ffff:x.x.x.x
    for (int i = 0; i < 10; i++)
    {
        if (address_[i] != 0) return false;
    }
    return address_[10] == 0xff && address_[11] == 0xff;
}

bool NetworkAddressWithTime::IsIPv6() const { return !IsIPv4(); }

int NetworkAddressWithTime::GetSize() const { return Size; }

void NetworkAddressWithTime::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(timestamp_);
    writer.Write(services_);
    writer.WriteBytes(address_.data(), 16);
    writer.Write(port_);
}

void NetworkAddressWithTime::Deserialize(io::BinaryReader& reader)
{
    timestamp_ = reader.ReadUInt32();
    services_ = reader.ReadUInt64();
    auto addressBytes = reader.ReadBytes(16);
    std::copy(addressBytes.begin(), addressBytes.end(), address_.begin());
    port_ = reader.ReadUInt16();
}

void NetworkAddressWithTime::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("timestamp", timestamp_);
    writer.WriteProperty("services", std::to_string(services_));
    writer.WriteProperty("address", GetAddress());
    writer.WriteProperty("port", port_);
    writer.WriteEndObject();
}

void NetworkAddressWithTime::DeserializeJson(const io::JsonReader& reader)
{
    const auto& json = reader.GetJson();

    if (json.contains("timestamp") && json["timestamp"].is_number())
    {
        timestamp_ = json["timestamp"].get<uint32_t>();
    }

    if (json.contains("services") && json["services"].is_string())
    {
        services_ = std::stoull(json["services"].get<std::string>());
    }

    if (json.contains("address") && json["address"].is_string())
    {
        SetAddress(json["address"].get<std::string>());
    }

    if (json.contains("port") && json["port"].is_number())
    {
        port_ = json["port"].get<uint16_t>();
    }
}

bool NetworkAddressWithTime::operator==(const NetworkAddressWithTime& other) const
{
    return timestamp_ == other.timestamp_ && services_ == other.services_ && address_ == other.address_ &&
           port_ == other.port_;
}

bool NetworkAddressWithTime::operator!=(const NetworkAddressWithTime& other) const { return !(*this == other); }

NetworkAddressWithTime NetworkAddressWithTime::FromIPv4(uint32_t timestamp, uint64_t services, const std::string& ipv4,
                                                        uint16_t port)
{
    NetworkAddressWithTime addr;
    addr.timestamp_ = timestamp;
    addr.services_ = services;
    addr.port_ = port;
    addr.ParseIPAddress(ipv4);
    return addr;
}

NetworkAddressWithTime NetworkAddressWithTime::FromIPv6(uint32_t timestamp, uint64_t services, const std::string& ipv6,
                                                        uint16_t port)
{
    NetworkAddressWithTime addr;
    addr.timestamp_ = timestamp;
    addr.services_ = services;
    addr.port_ = port;
    addr.ParseIPAddress(ipv6);
    return addr;
}

void NetworkAddressWithTime::ParseIPAddress(const std::string& address)
{
    address_.fill(0);

    // Check if it's an IPv4 address
    std::regex ipv4_regex(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)");
    std::smatch match;

    if (std::regex_match(address, match, ipv4_regex))
    {
        // IPv4 address - map to IPv6 format (::ffff:x.x.x.x)
        address_[10] = 0xff;
        address_[11] = 0xff;

        for (int i = 0; i < 4; i++)
        {
            int octet = std::stoi(match[i + 1].str());
            if (octet < 0 || octet > 255)
            {
                throw std::invalid_argument("Invalid IPv4 address: " + address);
            }
            address_[12 + i] = static_cast<uint8_t>(octet);
        }
    }
    else
    {
        // Complete IPv6 address parsing implementation
        // Parse IPv6 address in standard format (e.g., "2001:db8::1" or "::1")

        try
        {
            std::vector<uint16_t> groups(8, 0);  // IPv6 has 8 groups of 16-bit values
            std::string address_str = address;   // Fixed undefined ipString variable

            // Handle compressed notation (::)
            size_t double_colon_pos = address_str.find("::");
            if (double_colon_pos != std::string::npos)
            {
                // Split into parts before and after ::
                std::string before = address_str.substr(0, double_colon_pos);
                std::string after = address_str.substr(double_colon_pos + 2);

                // Parse groups before ::
                std::vector<uint16_t> before_groups;
                if (!before.empty())
                {
                    std::stringstream ss(before);
                    std::string group;
                    while (std::getline(ss, group, ':'))
                    {
                        if (!group.empty())
                        {
                            before_groups.push_back(static_cast<uint16_t>(std::stoul(group, nullptr, 16)));
                        }
                    }
                }

                // Parse groups after ::
                std::vector<uint16_t> after_groups;
                if (!after.empty())
                {
                    std::stringstream ss(after);
                    std::string group;
                    while (std::getline(ss, group, ':'))
                    {
                        if (!group.empty())
                        {
                            after_groups.push_back(static_cast<uint16_t>(std::stoul(group, nullptr, 16)));
                        }
                    }
                }

                // Fill the groups array
                for (size_t i = 0; i < before_groups.size(); ++i)
                {
                    groups[i] = before_groups[i];
                }
                for (size_t i = 0; i < after_groups.size(); ++i)
                {
                    groups[8 - after_groups.size() + i] = after_groups[i];
                }
            }
            else
            {
                // No compression - parse all 8 groups
                std::stringstream ss(address_str);
                std::string group;
                size_t index = 0;
                while (std::getline(ss, group, ':') && index < 8)
                {
                    if (!group.empty())
                    {
                        groups[index] = static_cast<uint16_t>(std::stoul(group, nullptr, 16));
                    }
                    ++index;
                }

                if (index != 8)
                {
                    throw std::invalid_argument("Invalid IPv6 address format");
                }
            }

            // Convert to byte array (network byte order - big endian)
            for (size_t i = 0; i < 8; ++i)
            {
                address_[i * 2] = static_cast<uint8_t>(groups[i] >> 8);
                address_[i * 2 + 1] = static_cast<uint8_t>(groups[i] & 0xFF);
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Failed to parse IPv6 address: " + std::string(e.what()));
        }
    }
}

std::string NetworkAddressWithTime::FormatIPAddress() const
{
    if (IsIPv4())
    {
        // Extract IPv4 from IPv4-mapped IPv6
        return std::to_string(address_[12]) + "." + std::to_string(address_[13]) + "." + std::to_string(address_[14]) +
               "." + std::to_string(address_[15]);
    }
    else
    {
        // Format as IPv6
        std::stringstream ss;
        for (int i = 0; i < 16; i += 2)
        {
            if (i > 0) ss << ":";
            uint16_t segment = (static_cast<uint16_t>(address_[i]) << 8) | address_[i + 1];
            ss << std::hex << segment;
        }
        return ss.str();
    }
}
}  // namespace neo::network::p2p::payloads