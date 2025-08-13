/**
 * @file ip_address.h
 * @brief Ip Address
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>
#include <string>

namespace neo::network
{
/**
 * @brief Represents an IP address.
 */
class IPAddress
{
   public:
    /**
     * @brief Constructs an empty IPAddress.
     */
    IPAddress();

    /**
     * @brief Constructs an IPAddress from a string.
     * @param address The IP address string.
     */
    explicit IPAddress(const std::string& address);

    /**
     * @brief Constructs an IPAddress from a 32-bit integer (IPv4).
     * @param address The IP address as a 32-bit integer.
     */
    explicit IPAddress(uint32_t address);

    /**
     * @brief Constructs an IPAddress from a byte array.
     * @param address The IP address as a byte array.
     * @param length The length of the byte array.
     */
    IPAddress(const uint8_t* address, size_t length);

    /**
     * @brief Gets the IP address as a string.
     * @return The IP address as a string.
     */
    std::string ToString() const;

    /**
     * @brief Gets the IP address as a byte array.
     * @return The IP address as a byte array.
     */
    const uint8_t* GetAddressBytes() const;

    /**
     * @brief Gets the length of the IP address in bytes.
     * @return The length of the IP address in bytes.
     */
    size_t GetAddressLength() const;

    /**
     * @brief Checks if this IPAddress is equal to another IPAddress.
     * @param other The other IPAddress.
     * @return True if the IPAddresses are equal, false otherwise.
     */
    bool operator==(const IPAddress& other) const;

    /**
     * @brief Checks if this IPAddress is not equal to another IPAddress.
     * @param other The other IPAddress.
     * @return True if the IPAddresses are not equal, false otherwise.
     */
    bool operator!=(const IPAddress& other) const;

    /**
     * @brief Gets the loopback address (127.0.0.1).
     * @return The loopback address.
     */
    static IPAddress Loopback();

    /**
     * @brief Gets the any address (0.0.0.0).
     * @return The any address.
     */
    static IPAddress Any();

    /**
     * @brief Parses an IP address string.
     * @param address The IP address string.
     * @return The parsed IPAddress.
     */
    static IPAddress Parse(const std::string& address);

    /**
     * @brief Tries to parse an IP address string.
     * @param address The IP address string.
     * @param result The parsed IPAddress.
     * @return True if the parsing was successful, false otherwise.
     */
    static bool TryParse(const std::string& address, IPAddress& result);

   private:
    uint8_t address_[16];
    size_t length_;
};
}  // namespace neo::network
