#include <neo/extensions/ipaddress_extensions.h>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <regex>

namespace neo::extensions
{
    bool IpAddressExtensions::IsValidIPv4(const std::string& address)
    {
        std::istringstream iss(address);
        std::string octet;
        std::vector<std::string> octets;
        
        // Split by dots
        while (std::getline(iss, octet, '.'))
        {
            octets.push_back(octet);
        }
        
        // Must have exactly 4 octets
        if (octets.size() != 4)
            return false;
            
        // Validate each octet
        for (const auto& oct : octets)
        {
            if (!IsValidIPv4Octet(oct))
                return false;
        }
        
        return true;
    }

    bool IpAddressExtensions::IsValidIPv6(const std::string& address)
    {
        // Basic IPv6 validation using regex
        // This is a simplified version - full IPv6 validation is complex
        std::regex ipv6Regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$|^::1$|^::$)");
        return std::regex_match(address, ipv6Regex);
    }

    std::array<uint8_t, 4> IpAddressExtensions::ParseIPv4(const std::string& address)
    {
        if (!IsValidIPv4(address))
            throw std::invalid_argument("Invalid IPv4 address: " + address);
            
        std::array<uint8_t, 4> result{};
        std::istringstream iss(address);
        std::string octet;
        int index = 0;
        
        while (std::getline(iss, octet, '.') && index < 4)
        {
            result[index++] = static_cast<uint8_t>(std::stoi(octet));
        }
        
        return result;
    }

    std::array<uint8_t, 16> IpAddressExtensions::ParseIPv6(const std::string& address)
    {
        // Simplified IPv6 parsing - would need full implementation for production
        std::array<uint8_t, 16> result{};
        
        if (address == "::1")
        {
            result[15] = 1; // Loopback
        }
        else if (address == "::")
        {
            // All zeros (already initialized)
        }
        else
        {
            throw std::invalid_argument("IPv6 parsing not fully implemented: " + address);
        }
        
        return result;
    }

    std::string IpAddressExtensions::IPv4ToString(const std::array<uint8_t, 4>& bytes)
    {
        return std::to_string(bytes[0]) + "." +
               std::to_string(bytes[1]) + "." +
               std::to_string(bytes[2]) + "." +
               std::to_string(bytes[3]);
    }

    std::string IpAddressExtensions::IPv6ToString(const std::array<uint8_t, 16>& bytes)
    {
        // Check for loopback
        bool isLoopback = true;
        for (int i = 0; i < 15; ++i)
        {
            if (bytes[i] != 0)
            {
                isLoopback = false;
                break;
            }
        }
        if (isLoopback && bytes[15] == 1)
            return "::1";
            
        // Check for all zeros
        bool isAllZeros = true;
        for (uint8_t byte : bytes)
        {
            if (byte != 0)
            {
                isAllZeros = false;
                break;
            }
        }
        if (isAllZeros)
            return "::";
            
        // Simplified IPv6 formatting
        return "IPv6 formatting not fully implemented";
    }

    bool IpAddressExtensions::IsPrivateIPv4(const std::string& address)
    {
        if (!IsValidIPv4(address))
            return false;
            
        auto bytes = ParseIPv4(address);
        
        // Check private ranges:
        // 10.0.0.0/8 (10.0.0.0 - 10.255.255.255)
        if (bytes[0] == 10)
            return true;
            
        // 172.16.0.0/12 (172.16.0.0 - 172.31.255.255)
        if (bytes[0] == 172 && bytes[1] >= 16 && bytes[1] <= 31)
            return true;
            
        // 192.168.0.0/16 (192.168.0.0 - 192.168.255.255)
        if (bytes[0] == 192 && bytes[1] == 168)
            return true;
            
        return false;
    }

    bool IpAddressExtensions::IsLoopbackIPv4(const std::string& address)
    {
        if (!IsValidIPv4(address))
            return false;
            
        auto bytes = ParseIPv4(address);
        return bytes[0] == 127; // 127.x.x.x
    }

    bool IpAddressExtensions::IsMulticastIPv4(const std::string& address)
    {
        if (!IsValidIPv4(address))
            return false;
            
        auto bytes = ParseIPv4(address);
        return bytes[0] >= 224 && bytes[0] <= 239; // 224.0.0.0 - 239.255.255.255
    }

    bool IpAddressExtensions::IsLinkLocalIPv4(const std::string& address)
    {
        if (!IsValidIPv4(address))
            return false;
            
        auto bytes = ParseIPv4(address);
        return bytes[0] == 169 && bytes[1] == 254; // 169.254.x.x
    }

    bool IpAddressExtensions::IsLoopbackIPv6(const std::string& address)
    {
        return address == "::1";
    }

    bool IpAddressExtensions::IsLinkLocalIPv6(const std::string& address)
    {
        // Link-local IPv6 addresses start with fe80:
        return address.length() >= 4 && address.substr(0, 4) == "fe80";
    }

    uint32_t IpAddressExtensions::IPv4ToUInt32(const std::string& address)
    {
        auto bytes = ParseIPv4(address);
        return (static_cast<uint32_t>(bytes[0]) << 24) |
               (static_cast<uint32_t>(bytes[1]) << 16) |
               (static_cast<uint32_t>(bytes[2]) << 8) |
               static_cast<uint32_t>(bytes[3]);
    }

    std::string IpAddressExtensions::UInt32ToIPv4(uint32_t value)
    {
        std::array<uint8_t, 4> bytes = {
            static_cast<uint8_t>((value >> 24) & 0xFF),
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>(value & 0xFF)
        };
        return IPv4ToString(bytes);
    }

    std::string IpAddressExtensions::GetNetworkAddressIPv4(const std::string& address, const std::string& subnetMask)
    {
        uint32_t addr = IPv4ToUInt32(address);
        uint32_t mask = IPv4ToUInt32(subnetMask);
        uint32_t network = addr & mask;
        return UInt32ToIPv4(network);
    }

    std::string IpAddressExtensions::GetBroadcastAddressIPv4(const std::string& address, const std::string& subnetMask)
    {
        uint32_t addr = IPv4ToUInt32(address);
        uint32_t mask = IPv4ToUInt32(subnetMask);
        uint32_t broadcast = (addr & mask) | (~mask);
        return UInt32ToIPv4(broadcast);
    }

    bool IpAddressExtensions::IsInSameSubnetIPv4(const std::string& address1, const std::string& address2, const std::string& subnetMask)
    {
        uint32_t addr1 = IPv4ToUInt32(address1);
        uint32_t addr2 = IPv4ToUInt32(address2);
        uint32_t mask = IPv4ToUInt32(subnetMask);
        
        return (addr1 & mask) == (addr2 & mask);
    }

    std::string IpAddressExtensions::ExpandIPv6(const std::string& address)
    {
        // Simplified implementation
        if (address == "::1")
            return "0000:0000:0000:0000:0000:0000:0000:0001";
        if (address == "::")
            return "0000:0000:0000:0000:0000:0000:0000:0000";
        return address; // Return as-is for now
    }

    std::string IpAddressExtensions::CompressIPv6(const std::string& address)
    {
        // Simplified implementation
        if (address == "0000:0000:0000:0000:0000:0000:0000:0001")
            return "::1";
        if (address == "0000:0000:0000:0000:0000:0000:0000:0000")
            return "::";
        return address; // Return as-is for now
    }

    bool IpAddressExtensions::IsValidIPv4Octet(const std::string& octet)
    {
        if (octet.empty() || octet.length() > 3)
            return false;
            
        // Check for leading zeros (not allowed except for "0")
        if (octet.length() > 1 && octet[0] == '0')
            return false;
            
        // Check all characters are digits
        for (char c : octet)
        {
            if (!std::isdigit(c))
                return false;
        }
        
        // Check range 0-255
        int value = std::stoi(octet);
        return value >= 0 && value <= 255;
    }

    bool IpAddressExtensions::IsValidIPv6Hextet(const std::string& hextet)
    {
        if (hextet.empty() || hextet.length() > 4)
            return false;
            
        // Check all characters are hex digits
        for (char c : hextet)
        {
            if (!std::isxdigit(c))
                return false;
        }
        
        return true;
    }
}
