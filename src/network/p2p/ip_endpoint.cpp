#include <neo/network/p2p/ip_endpoint.h>

#include <regex>
#include <stdexcept>

namespace neo::network::p2p
{
IPEndPoint::IPEndPoint() : port_(0) {}

IPEndPoint::IPEndPoint(const std::string& address, uint16_t port) : address_(address), port_(port) {}

const std::string& IPEndPoint::GetAddress() const { return address_; }

void IPEndPoint::SetAddress(const std::string& address) { address_ = address; }

uint16_t IPEndPoint::GetPort() const { return port_; }

void IPEndPoint::SetPort(uint16_t port) { port_ = port; }

void IPEndPoint::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarString(address_);
    writer.Write(port_);
}

void IPEndPoint::Deserialize(io::BinaryReader& reader)
{
    address_ = reader.ReadVarString();
    port_ = reader.ReadUInt16();
}

void IPEndPoint::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("address", address_);
    writer.Write("port", port_);
}

void IPEndPoint::DeserializeJson(const io::JsonReader& reader)
{
    address_ = reader.ReadString("address");
    port_ = reader.ReadUInt16("port");
}

bool IPEndPoint::operator==(const IPEndPoint& other) const
{
    return address_ == other.address_ && port_ == other.port_;
}

bool IPEndPoint::operator!=(const IPEndPoint& other) const { return !(*this == other); }

std::string IPEndPoint::ToString() const { return address_ + ":" + std::to_string(port_); }

IPEndPoint IPEndPoint::Parse(const std::string& str)
{
    IPEndPoint result;
    if (!TryParse(str, result)) throw std::invalid_argument("Invalid IP endpoint format");
    return result;
}

bool IPEndPoint::TryParse(const std::string& str, IPEndPoint& result)
{
    // Regular expression to match IPv4 address:port
    std::regex ipv4Regex("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d+)$");

    // Regular expression to match IPv6 address:port
    std::regex ipv6Regex("^\\[([0-9a-fA-F:]+)\\]:(\\d+)$");

    std::smatch match;

    if (std::regex_match(str, match, ipv4Regex))
    {
        // IPv4 address
        std::string address = match[1].str();
        uint16_t port;
        try
        {
            port = static_cast<uint16_t>(std::stoi(match[2].str()));
        }
        catch (const std::exception&)
        {
            return false;
        }

        result = IPEndPoint(address, port);
        return true;
    }
    else if (std::regex_match(str, match, ipv6Regex))
    {
        // IPv6 address
        std::string address = match[1].str();
        uint16_t port;
        try
        {
            port = static_cast<uint16_t>(std::stoi(match[2].str()));
        }
        catch (const std::exception&)
        {
            return false;
        }

        result = IPEndPoint(address, port);
        return true;
    }

    return false;
}
}  // namespace neo::network::p2p
