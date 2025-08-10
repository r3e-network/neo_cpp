#include <neo/extensions/ipaddress_extensions.h>

#include <algorithm>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

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
    if (octets.size() != 4) return false;

    // Validate each octet
    for (const auto& oct : octets)
    {
        if (!IsValidIPv4Octet(oct)) return false;
    }

    return true;
}

bool IpAddressExtensions::IsValidIPv6(const std::string& address)
{
    // Complete IPv6 validation supporting all standard formats
    if (address.empty() || address.length() > 39)
    {
        return false;
    }

    // Handle special cases
    if (address == "::" || address == "::1")
    {
        return true;
    }

    // Check for invalid characters
    for (char c : address)
    {
        if (!std::isxdigit(c) && c != ':')
        {
            return false;
        }
    }

    // Check for double colon (can appear at most once)
    size_t double_colon_count = 0;
    size_t pos = 0;
    while ((pos = address.find("::", pos)) != std::string::npos)
    {
        double_colon_count++;
        pos += 2;
    }

    if (double_colon_count > 1)
    {
        return false;
    }

    // Check for invalid colon patterns
    if (address.front() == ':' && address.substr(0, 2) != "::")
    {
        return false;
    }
    if (address.back() == ':' && address.substr(address.length() - 2) != "::")
    {
        return false;
    }

    // Split by colons and validate groups
    std::vector<std::string> parts;
    std::stringstream ss(address);
    std::string part;

    while (std::getline(ss, part, ':'))
    {
        parts.push_back(part);
    }

    // Count non-empty parts
    int non_empty_parts = 0;
    for (const auto& p : parts)
    {
        if (!p.empty())
        {
            non_empty_parts++;
            // Validate each hextet (1-4 hex digits)
            if (!IsValidIPv6Hextet(p))
            {
                return false;
            }
        }
    }

    // If no double colon, must have exactly 8 groups
    if (double_colon_count == 0)
    {
        return non_empty_parts == 8 && parts.size() == 8;
    }
    else
    {
        // With double colon, total groups must be <= 8
        return non_empty_parts <= 8;
    }
}

std::array<uint8_t, 4> IpAddressExtensions::ParseIPv4(const std::string& address)
{
    if (!IsValidIPv4(address)) throw std::invalid_argument("Invalid IPv4 address: " + address);

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
    // Complete IPv6 parsing implementation for all standard formats
    std::array<uint8_t, 16> result{};

    if (!IsValidIPv6(address))
    {
        throw std::invalid_argument("Invalid IPv6 address: " + address);
    }

    // Handle common special cases
    if (address == "::1")
    {
        result[15] = 1;  // Loopback
        return result;
    }
    if (address == "::")
    {
        // All zeros (already initialized to 0)
        return result;
    }

    try
    {
        // Expand the address to full format for easier parsing
        std::string expanded = ExpandIPv6(address);

        // Parse each group (4 hex digits = 2 bytes)
        std::vector<std::string> groups;
        std::stringstream ss(expanded);
        std::string group;

        while (std::getline(ss, group, ':'))
        {
            groups.push_back(group);
        }

        if (groups.size() != 8)
        {
            throw std::invalid_argument("Invalid IPv6 format after expansion");
        }

        // Convert each group to 2 bytes
        for (size_t i = 0; i < 8; ++i)
        {
            if (groups[i].length() != 4)
            {
                throw std::invalid_argument("Invalid group length in expanded IPv6");
            }

            // Parse 4 hex digits into 2 bytes (big-endian)
            uint16_t group_value = 0;
            for (char c : groups[i])
            {
                group_value = (group_value << 4);
                if (c >= '0' && c <= '9')
                {
                    group_value |= (c - '0');
                }
                else if (c >= 'a' && c <= 'f')
                {
                    group_value |= (c - 'a' + 10);
                }
                else if (c >= 'A' && c <= 'F')
                {
                    group_value |= (c - 'A' + 10);
                }
                else
                {
                    throw std::invalid_argument("Invalid hex character in IPv6 group");
                }
            }

            // Store as big-endian bytes
            result[i * 2] = static_cast<uint8_t>((group_value >> 8) & 0xFF);
            result[i * 2 + 1] = static_cast<uint8_t>(group_value & 0xFF);
        }

        return result;
    }
    catch (const std::exception& e)
    {
        throw std::invalid_argument("Failed to parse IPv6 address '" + address + "': " + e.what());
    }
}

std::string IpAddressExtensions::IPv4ToString(const std::array<uint8_t, 4>& bytes)
{
    return std::to_string(bytes[0]) + "." + std::to_string(bytes[1]) + "." + std::to_string(bytes[2]) + "." +
           std::to_string(bytes[3]);
}

std::string IpAddressExtensions::IPv6ToString(const std::array<uint8_t, 16>& bytes)
{
    // Complete IPv6 string formatting with proper compression

    // Check for special cases first
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
    {
        return "::1";
    }

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
    {
        return "::";
    }

    // Convert bytes to 16-bit groups (big-endian)
    std::vector<uint16_t> groups(8);
    for (int i = 0; i < 8; ++i)
    {
        groups[i] = (static_cast<uint16_t>(bytes[i * 2]) << 8) | bytes[i * 2 + 1];
    }

    // Find the longest sequence of consecutive zero groups for compression
    int longest_start = -1;
    int longest_length = 0;
    int current_start = -1;
    int current_length = 0;

    for (int i = 0; i < 8; ++i)
    {
        if (groups[i] == 0)
        {
            if (current_start == -1)
            {
                current_start = i;
                current_length = 1;
            }
            else
            {
                current_length++;
            }
        }
        else
        {
            if (current_length > longest_length)
            {
                longest_start = current_start;
                longest_length = current_length;
            }
            current_start = -1;
            current_length = 0;
        }
    }

    // Check final sequence
    if (current_length > longest_length)
    {
        longest_start = current_start;
        longest_length = current_length;
    }

    // Build the formatted string
    std::stringstream result;

    // Only use compression (::) if we have at least 2 consecutive zeros
    if (longest_length >= 2)
    {
        // Format groups before compression
        for (int i = 0; i < longest_start; ++i)
        {
            if (i > 0) result << ":";
            result << std::hex << groups[i];
        }

        // Add double colon
        if (longest_start == 0)
        {
            result << "::";
        }
        else
        {
            result << "::";
        }

        // Format groups after compression
        int after_start = longest_start + longest_length;
        for (int i = after_start; i < 8; ++i)
        {
            if (i > after_start) result << ":";
            result << std::hex << groups[i];
        }

        // Handle edge case where compression is at the end
        if (after_start == 8 && longest_start > 0)
        {
            // Double colon at end is already added, nothing more needed
        }
    }
    else
    {
        // No compression - format all groups
        for (int i = 0; i < 8; ++i)
        {
            if (i > 0) result << ":";
            result << std::hex << groups[i];
        }
    }

    return result.str();
}

bool IpAddressExtensions::IsPrivateIPv4(const std::string& address)
{
    if (!IsValidIPv4(address)) return false;

    auto bytes = ParseIPv4(address);

    // Check private ranges:
    // 10.0.0.0/8 (10.0.0.0 - 10.255.255.255)
    if (bytes[0] == 10) return true;

    // 172.16.0.0/12 (172.16.0.0 - 172.31.255.255)
    if (bytes[0] == 172 && bytes[1] >= 16 && bytes[1] <= 31) return true;

    // 192.168.0.0/16 (192.168.0.0 - 192.168.255.255)
    if (bytes[0] == 192 && bytes[1] == 168) return true;

    return false;
}

bool IpAddressExtensions::IsLoopbackIPv4(const std::string& address)
{
    if (!IsValidIPv4(address)) return false;

    auto bytes = ParseIPv4(address);
    return bytes[0] == 127;  // 127.x.x.x
}

bool IpAddressExtensions::IsMulticastIPv4(const std::string& address)
{
    if (!IsValidIPv4(address)) return false;

    auto bytes = ParseIPv4(address);
    return bytes[0] >= 224 && bytes[0] <= 239;  // 224.0.0.0 - 239.255.255.255
}

bool IpAddressExtensions::IsLinkLocalIPv4(const std::string& address)
{
    if (!IsValidIPv4(address)) return false;

    auto bytes = ParseIPv4(address);
    return bytes[0] == 169 && bytes[1] == 254;  // 169.254.x.x
}

bool IpAddressExtensions::IsLoopbackIPv6(const std::string& address) { return address == "::1"; }

bool IpAddressExtensions::IsLinkLocalIPv6(const std::string& address)
{
    // Link-local IPv6 addresses start with fe80:
    return address.length() >= 4 && address.substr(0, 4) == "fe80";
}

uint32_t IpAddressExtensions::IPv4ToUInt32(const std::string& address)
{
    auto bytes = ParseIPv4(address);
    return (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) |
           (static_cast<uint32_t>(bytes[2]) << 8) | static_cast<uint32_t>(bytes[3]);
}

std::string IpAddressExtensions::UInt32ToIPv4(uint32_t value)
{
    std::array<uint8_t, 4> bytes = {static_cast<uint8_t>((value >> 24) & 0xFF),
                                    static_cast<uint8_t>((value >> 16) & 0xFF),
                                    static_cast<uint8_t>((value >> 8) & 0xFF), static_cast<uint8_t>(value & 0xFF)};
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

bool IpAddressExtensions::IsInSameSubnetIPv4(const std::string& address1, const std::string& address2,
                                             const std::string& subnetMask)
{
    uint32_t addr1 = IPv4ToUInt32(address1);
    uint32_t addr2 = IPv4ToUInt32(address2);
    uint32_t mask = IPv4ToUInt32(subnetMask);

    return (addr1 & mask) == (addr2 & mask);
}

std::string IpAddressExtensions::ExpandIPv6(const std::string& address)
{
    // Complete IPv6 expansion implementation
    try
    {
        // Handle common special cases
        if (address == "::1") return "0000:0000:0000:0000:0000:0000:0000:0001";
        if (address == "::") return "0000:0000:0000:0000:0000:0000:0000:0000";

        // Check if already expanded (8 groups of 4 hex digits)
        if (address.length() == 39 && std::count(address.begin(), address.end(), ':') == 7)
        {
            // Validate format and return if already expanded
            bool valid = true;
            size_t pos = 0;
            for (int i = 0; i < 8; ++i)
            {
                if (pos + 4 > address.length())
                {
                    valid = false;
                    break;
                }
                for (int j = 0; j < 4; ++j)
                {
                    char c = address[pos + j];
                    if (!std::isxdigit(c))
                    {
                        valid = false;
                        break;
                    }
                }
                if (!valid) break;
                pos += 4;
                if (i < 7)
                {
                    if (pos >= address.length() || address[pos] != ':')
                    {
                        valid = false;
                        break;
                    }
                    pos++;
                }
            }
            if (valid) return address;
        }

        // Handle double colon expansion
        std::string result = address;
        size_t double_colon_pos = result.find("::");

        if (double_colon_pos != std::string::npos)
        {
            // Split into left and right parts
            std::string left = result.substr(0, double_colon_pos);
            std::string right = result.substr(double_colon_pos + 2);

            // Count existing groups
            int left_groups = left.empty() ? 0 : std::count(left.begin(), left.end(), ':') + 1;
            int right_groups = right.empty() ? 0 : std::count(right.begin(), right.end(), ':') + 1;
            int missing_groups = 8 - left_groups - right_groups;

            // Build expanded address
            result = left;
            if (!left.empty()) result += ":";

            for (int i = 0; i < missing_groups; ++i)
            {
                result += "0000";
                if (i < missing_groups - 1 || !right.empty()) result += ":";
            }

            if (!right.empty())
            {
                if (!result.empty() && result.back() != ':') result += ":";
                result += right;
            }
        }

        // Expand each group to 4 digits
        std::vector<std::string> groups;
        std::stringstream ss(result);
        std::string group;

        while (std::getline(ss, group, ':'))
        {
            if (group.length() < 4)
            {
                group = std::string(4 - group.length(), '0') + group;
            }
            groups.push_back(group);
        }

        // Ensure we have exactly 8 groups
        while (groups.size() < 8)
        {
            groups.push_back("0000");
        }

        // Join groups with colons
        std::string expanded;
        for (size_t i = 0; i < groups.size() && i < 8; ++i)
        {
            if (i > 0) expanded += ":";
            expanded += groups[i];
        }

        return expanded;
    }
    catch (const std::exception& e)
    {
        // Error in expansion - return original address
        return address;
    }
}

std::string IpAddressExtensions::CompressIPv6(const std::string& address)
{
    // Complete IPv6 compression implementation
    try
    {
        // Handle common special cases
        if (address == "0000:0000:0000:0000:0000:0000:0000:0001") return "::1";
        if (address == "0000:0000:0000:0000:0000:0000:0000:0000") return "::";

        // First, expand the address to ensure consistent format
        std::string expanded = ExpandIPv6(address);

        // Split into groups
        std::vector<std::string> groups;
        std::stringstream ss(expanded);
        std::string group;

        while (std::getline(ss, group, ':'))
        {
            groups.push_back(group);
        }

        if (groups.size() != 8)
        {
            // Invalid format - return original
            return address;
        }

        // Remove leading zeros from each group
        for (auto& g : groups)
        {
            while (g.length() > 1 && g[0] == '0')
            {
                g = g.substr(1);
            }
        }

        // Find the longest sequence of consecutive zero groups
        int longest_start = -1;
        int longest_length = 0;
        int current_start = -1;
        int current_length = 0;

        for (int i = 0; i < 8; ++i)
        {
            if (groups[i] == "0")
            {
                if (current_start == -1)
                {
                    current_start = i;
                    current_length = 1;
                }
                else
                {
                    current_length++;
                }
            }
            else
            {
                if (current_length > longest_length)
                {
                    longest_start = current_start;
                    longest_length = current_length;
                }
                current_start = -1;
                current_length = 0;
            }
        }

        // Check final sequence
        if (current_length > longest_length)
        {
            longest_start = current_start;
            longest_length = current_length;
        }

        // Only compress if we have at least 2 consecutive zeros
        if (longest_length < 2)
        {
            // No compression possible - just join with colons
            std::string result;
            for (size_t i = 0; i < groups.size(); ++i)
            {
                if (i > 0) result += ":";
                result += groups[i];
            }
            return result;
        }

        // Build compressed address
        std::string result;

        // Add groups before the zero sequence
        for (int i = 0; i < longest_start; ++i)
        {
            if (i > 0) result += ":";
            result += groups[i];
        }

        // Add double colon
        if (longest_start == 0)
        {
            result += "::";
        }
        else
        {
            result += "::";
        }

        // Add groups after the zero sequence
        int after_start = longest_start + longest_length;
        for (int i = after_start; i < 8; ++i)
        {
            if (i > after_start) result += ":";
            result += groups[i];
        }

        // Handle edge cases for double colon at the end
        if (after_start == 8 && longest_start > 0)
        {
            // Double colon at the end, but not at the beginning
            // Result already ends with "::" which is correct
        }

        return result;
    }
    catch (const std::exception& e)
    {
        // Error in compression - return original address
        return address;
    }
}

bool IpAddressExtensions::IsValidIPv4Octet(const std::string& octet)
{
    if (octet.empty() || octet.length() > 3) return false;

    // Check for leading zeros (not allowed except for "0")
    if (octet.length() > 1 && octet[0] == '0') return false;

    // Check all characters are digits
    for (char c : octet)
    {
        if (!std::isdigit(c)) return false;
    }

    // Check range 0-255
    int value = std::stoi(octet);
    return value >= 0 && value <= 255;
}

bool IpAddressExtensions::IsValidIPv6Hextet(const std::string& hextet)
{
    if (hextet.empty() || hextet.length() > 4) return false;

    // Check all characters are hex digits
    for (char c : hextet)
    {
        if (!std::isxdigit(c)) return false;
    }

    return true;
}
}  // namespace neo::extensions
