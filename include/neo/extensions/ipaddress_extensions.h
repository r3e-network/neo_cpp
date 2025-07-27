#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Extensions for IP address operations.
 *
 * ## Overview
 * Provides utility methods for IP address validation, conversion, and manipulation.
 * Supports both IPv4 and IPv6 addresses with parsing and formatting capabilities.
 *
 * ## API Reference
 * - **Validation**: Check if strings are valid IP addresses
 * - **Conversion**: Parse strings to binary representation
 * - **Utilities**: Address classification, subnet operations
 *
 * ## Usage Examples
 * ```cpp
 * // Validate IP address
 * bool valid = IpAddressExtensions::IsValidIPv4("192.168.1.1");
 *
 * // Parse to binary
 * auto ipBytes = IpAddressExtensions::ParseIPv4("10.0.0.1");
 *
 * // Check if private
 * bool isPrivate = IpAddressExtensions::IsPrivateIPv4("172.16.1.1");
 * ```
 */
class IpAddressExtensions
{
  public:
    /**
     * @brief Check if string is a valid IPv4 address
     * @param address String to validate
     * @return True if valid IPv4 address
     */
    static bool IsValidIPv4(const std::string& address);

    /**
     * @brief Check if string is a valid IPv6 address
     * @param address String to validate
     * @return True if valid IPv6 address
     */
    static bool IsValidIPv6(const std::string& address);

    /**
     * @brief Parse IPv4 address string to 4-byte array
     * @param address IPv4 address string
     * @return 4-byte array representing the address
     * @throws std::invalid_argument if address is invalid
     */
    static std::array<uint8_t, 4> ParseIPv4(const std::string& address);

    /**
     * @brief Parse IPv6 address string to 16-byte array
     * @param address IPv6 address string
     * @return 16-byte array representing the address
     * @throws std::invalid_argument if address is invalid
     */
    static std::array<uint8_t, 16> ParseIPv6(const std::string& address);

    /**
     * @brief Convert 4-byte array to IPv4 string
     * @param bytes 4-byte array
     * @return IPv4 address string
     */
    static std::string IPv4ToString(const std::array<uint8_t, 4>& bytes);

    /**
     * @brief Convert 16-byte array to IPv6 string
     * @param bytes 16-byte array
     * @return IPv6 address string
     */
    static std::string IPv6ToString(const std::array<uint8_t, 16>& bytes);

    /**
     * @brief Check if IPv4 address is in private range
     * @param address IPv4 address string
     * @return True if private address
     */
    static bool IsPrivateIPv4(const std::string& address);

    /**
     * @brief Check if IPv4 address is loopback (127.x.x.x)
     * @param address IPv4 address string
     * @return True if loopback address
     */
    static bool IsLoopbackIPv4(const std::string& address);

    /**
     * @brief Check if IPv4 address is multicast (224.0.0.0-239.255.255.255)
     * @param address IPv4 address string
     * @return True if multicast address
     */
    static bool IsMulticastIPv4(const std::string& address);

    /**
     * @brief Check if IPv4 address is link-local (169.254.x.x)
     * @param address IPv4 address string
     * @return True if link-local address
     */
    static bool IsLinkLocalIPv4(const std::string& address);

    /**
     * @brief Check if IPv6 address is loopback (::1)
     * @param address IPv6 address string
     * @return True if loopback address
     */
    static bool IsLoopbackIPv6(const std::string& address);

    /**
     * @brief Check if IPv6 address is link-local (fe80::/10)
     * @param address IPv6 address string
     * @return True if link-local address
     */
    static bool IsLinkLocalIPv6(const std::string& address);

    /**
     * @brief Get network address for IPv4 with subnet mask
     * @param address IPv4 address string
     * @param subnetMask Subnet mask (e.g., "255.255.255.0")
     * @return Network address string
     */
    static std::string GetNetworkAddressIPv4(const std::string& address, const std::string& subnetMask);

    /**
     * @brief Get broadcast address for IPv4 with subnet mask
     * @param address IPv4 address string
     * @param subnetMask Subnet mask (e.g., "255.255.255.0")
     * @return Broadcast address string
     */
    static std::string GetBroadcastAddressIPv4(const std::string& address, const std::string& subnetMask);

    /**
     * @brief Check if two IPv4 addresses are in the same subnet
     * @param address1 First IPv4 address
     * @param address2 Second IPv4 address
     * @param subnetMask Subnet mask
     * @return True if in same subnet
     */
    static bool IsInSameSubnetIPv4(const std::string& address1, const std::string& address2,
                                   const std::string& subnetMask);

    /**
     * @brief Convert IPv4 address to 32-bit integer (network byte order)
     * @param address IPv4 address string
     * @return 32-bit integer representation
     */
    static uint32_t IPv4ToUInt32(const std::string& address);

    /**
     * @brief Convert 32-bit integer to IPv4 address string (network byte order)
     * @param value 32-bit integer
     * @return IPv4 address string
     */
    static std::string UInt32ToIPv4(uint32_t value);

    /**
     * @brief Expand IPv6 address to full format (remove abbreviations)
     * @param address IPv6 address string (possibly abbreviated)
     * @return Expanded IPv6 address string
     */
    static std::string ExpandIPv6(const std::string& address);

    /**
     * @brief Compress IPv6 address (add abbreviations where possible)
     * @param address IPv6 address string
     * @return Compressed IPv6 address string
     */
    static std::string CompressIPv6(const std::string& address);

  private:
    /**
     * @brief Helper to validate IPv4 octet
     * @param octet String representation of octet
     * @return True if valid octet (0-255)
     */
    static bool IsValidIPv4Octet(const std::string& octet);

    /**
     * @brief Helper to validate IPv6 hextet
     * @param hextet String representation of hextet
     * @return True if valid hextet (0-FFFF)
     */
    static bool IsValidIPv6Hextet(const std::string& hextet);
};
}  // namespace neo::extensions
