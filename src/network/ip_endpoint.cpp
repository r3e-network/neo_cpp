#include <cstring>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/ip_endpoint.h>
#include <regex>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace neo::network
{
// IPAddress implementation
IPAddress::IPAddress() : length_(0)
{
    std::memset(address_, 0, sizeof(address_));
}

IPAddress::IPAddress(const std::string& address)
{
    if (!TryParse(address, *this))
        throw std::invalid_argument("Invalid IP address");
}

IPAddress::IPAddress(uint32_t address) : length_(4)
{
    std::memset(address_, 0, sizeof(address_));
    address_[0] = static_cast<uint8_t>((address >> 24) & 0xFF);
    address_[1] = static_cast<uint8_t>((address >> 16) & 0xFF);
    address_[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
    address_[3] = static_cast<uint8_t>(address & 0xFF);
}

IPAddress::IPAddress(const uint8_t* address, size_t length) : length_(length)
{
    if (length > 16)
        throw std::invalid_argument("Invalid IP address length");

    std::memset(address_, 0, sizeof(address_));
    std::memcpy(address_, address, length);
}

std::string IPAddress::ToString() const
{
    if (length_ == 4)
    {
        // IPv4
        char buffer[INET_ADDRSTRLEN];
        struct in_addr addr;
        addr.s_addr = htonl((address_[0] << 24) | (address_[1] << 16) | (address_[2] << 8) | address_[3]);
        inet_ntop(AF_INET, &addr, buffer, INET_ADDRSTRLEN);
        return buffer;
    }
    else if (length_ == 16)
    {
        // IPv6
        char buffer[INET6_ADDRSTRLEN];
        struct in6_addr addr;
        std::memcpy(addr.s6_addr, address_, 16);
        inet_ntop(AF_INET6, &addr, buffer, INET6_ADDRSTRLEN);
        return buffer;
    }
    else
    {
        return "";
    }
}

const uint8_t* IPAddress::GetAddressBytes() const
{
    return address_;
}

size_t IPAddress::GetAddressLength() const
{
    return length_;
}

bool IPAddress::operator==(const IPAddress& other) const
{
    if (length_ != other.length_)
        return false;

    return std::memcmp(address_, other.address_, length_) == 0;
}

bool IPAddress::operator!=(const IPAddress& other) const
{
    return !(*this == other);
}

IPAddress IPAddress::Loopback()
{
    IPAddress address;
    address.length_ = 4;
    address.address_[0] = 127;
    address.address_[1] = 0;
    address.address_[2] = 0;
    address.address_[3] = 1;
    return address;
}

IPAddress IPAddress::Any()
{
    IPAddress address;
    address.length_ = 4;
    address.address_[0] = 0;
    address.address_[1] = 0;
    address.address_[2] = 0;
    address.address_[3] = 0;
    return address;
}

IPAddress IPAddress::Parse(const std::string& address)
{
    IPAddress result;
    if (!TryParse(address, result))
        throw std::invalid_argument("Invalid IP address");
    return result;
}

bool IPAddress::TryParse(const std::string& address, IPAddress& result)
{
    struct in_addr addr4;
    if (inet_pton(AF_INET, address.c_str(), &addr4) == 1)
    {
        // IPv4
        result.length_ = 4;
        uint32_t addr = ntohl(addr4.s_addr);
        result.address_[0] = static_cast<uint8_t>((addr >> 24) & 0xFF);
        result.address_[1] = static_cast<uint8_t>((addr >> 16) & 0xFF);
        result.address_[2] = static_cast<uint8_t>((addr >> 8) & 0xFF);
        result.address_[3] = static_cast<uint8_t>(addr & 0xFF);
        return true;
    }

    struct in6_addr addr6;
    if (inet_pton(AF_INET6, address.c_str(), &addr6) == 1)
    {
        // IPv6
        result.length_ = 16;
        std::memcpy(result.address_, addr6.s6_addr, 16);
        return true;
    }

    return false;
}

// IPEndPoint implementation
IPEndPoint::IPEndPoint() : port_(0) {}

IPEndPoint::IPEndPoint(const IPAddress& address, uint16_t port) : address_(address), port_(port) {}

IPEndPoint::IPEndPoint(const std::string& address, uint16_t port) : address_(address), port_(port) {}

const IPAddress& IPEndPoint::GetAddress() const
{
    return address_;
}

uint16_t IPEndPoint::GetPort() const
{
    return port_;
}

std::string IPEndPoint::ToString() const
{
    std::ostringstream oss;
    if (address_.GetAddressLength() == 4)
    {
        // IPv4
        oss << address_.ToString() << ":" << port_;
    }
    else if (address_.GetAddressLength() == 16)
    {
        // IPv6
        oss << "[" << address_.ToString() << "]:" << port_;
    }
    return oss.str();
}

void IPEndPoint::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(address_.GetAddressLength()));
    writer.Write(io::ByteSpan(address_.GetAddressBytes(), address_.GetAddressLength()));
    writer.Write(port_);
}

void IPEndPoint::Deserialize(io::BinaryReader& reader)
{
    uint8_t length = reader.ReadUInt8();
    io::ByteVector bytes = reader.ReadBytes(length);
    address_ = IPAddress(bytes.Data(), bytes.Size());
    port_ = reader.ReadUInt16();
}

bool IPEndPoint::operator==(const IPEndPoint& other) const
{
    return address_ == other.address_ && port_ == other.port_;
}

bool IPEndPoint::operator!=(const IPEndPoint& other) const
{
    return !(*this == other);
}

IPEndPoint IPEndPoint::Parse(const std::string& endpoint)
{
    IPEndPoint result;
    if (!TryParse(endpoint, result))
        throw std::invalid_argument("Invalid IP endpoint");
    return result;
}

bool IPEndPoint::TryParse(const std::string& endpoint, IPEndPoint& result)
{
    // IPv4 endpoint: address:port
    std::regex ipv4_regex("^([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+):([0-9]+)$");
    std::smatch ipv4_match;
    if (std::regex_match(endpoint, ipv4_match, ipv4_regex))
    {
        IPAddress address;
        if (!IPAddress::TryParse(ipv4_match[1].str(), address))
            return false;

        uint16_t port;
        try
        {
            port = static_cast<uint16_t>(std::stoi(ipv4_match[2].str()));
        }
        catch (const std::exception&)
        {
            return false;
        }

        result = IPEndPoint(address, port);
        return true;
    }

    // IPv6 endpoint: [address]:port
    std::regex ipv6_regex("^\\[([0-9a-fA-F:]+)\\]:([0-9]+)$");
    std::smatch ipv6_match;
    if (std::regex_match(endpoint, ipv6_match, ipv6_regex))
    {
        IPAddress address;
        if (!IPAddress::TryParse(ipv6_match[1].str(), address))
            return false;

        uint16_t port;
        try
        {
            port = static_cast<uint16_t>(std::stoi(ipv6_match[2].str()));
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
}  // namespace neo::network
