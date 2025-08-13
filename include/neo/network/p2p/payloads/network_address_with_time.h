/**
 * @file network_address_with_time.h
 * @brief Network Address With Time
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>

#include <array>
#include <cstdint>
#include <string>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a network address with timestamp for peer discovery.
 *
 * This matches the C# NetworkAddressWithTime.cs implementation exactly.
 */
class NetworkAddressWithTime : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /// <summary>
    /// The size of the network address with time in bytes.
    /// </summary>
    static constexpr int Size = sizeof(uint32_t) +  // Timestamp
                                sizeof(uint64_t) +  // Services
                                16 +                // Address (IPv6)
                                sizeof(uint16_t);   // Port

   private:
    uint32_t timestamp_;
    uint64_t services_;
    std::array<uint8_t, 16> address_;  // IPv6 address (IPv4 mapped if needed)
    uint16_t port_;

   public:
    /**
     * @brief Constructs an empty NetworkAddressWithTime.
     */
    NetworkAddressWithTime();

    /**
     * @brief Constructs a NetworkAddressWithTime with the specified parameters.
     * @param timestamp The timestamp.
     * @param services The services.
     * @param address The IP address.
     * @param port The port.
     */
    NetworkAddressWithTime(uint32_t timestamp, uint64_t services, const std::string& address, uint16_t port);

    /**
     * @brief Gets the timestamp.
     * @return The timestamp.
     */
    uint32_t GetTimestamp() const;

    /**
     * @brief Sets the timestamp.
     * @param timestamp The timestamp.
     */
    void SetTimestamp(uint32_t timestamp);

    /**
     * @brief Gets the services.
     * @return The services.
     */
    uint64_t GetServices() const;

    /**
     * @brief Sets the services.
     * @param services The services.
     */
    void SetServices(uint64_t services);

    /**
     * @brief Gets the IP address as a string.
     * @return The IP address.
     */
    std::string GetAddress() const;

    /**
     * @brief Sets the IP address from a string.
     * @param address The IP address.
     */
    void SetAddress(const std::string& address);

    /**
     * @brief Gets the raw address bytes.
     * @return The address bytes.
     */
    const std::array<uint8_t, 16>& GetAddressBytes() const;

    /**
     * @brief Sets the raw address bytes.
     * @param address The address bytes.
     */
    void SetAddressBytes(const std::array<uint8_t, 16>& address);

    /**
     * @brief Gets the port.
     * @return The port.
     */
    uint16_t GetPort() const;

    /**
     * @brief Sets the port.
     * @param port The port.
     */
    void SetPort(uint16_t port);

    /**
     * @brief Gets the endpoint as host:port string.
     * @return The endpoint string.
     */
    std::string GetEndpoint() const;

    /**
     * @brief Checks if this is an IPv4 address.
     * @return True if IPv4.
     */
    bool IsIPv4() const;

    /**
     * @brief Checks if this is an IPv6 address.
     * @return True if IPv6.
     */
    bool IsIPv6() const;

    /**
     * @brief Gets the size of the network address.
     * @return The size in bytes.
     */
    int GetSize() const;

    // ISerializable interface
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

    // IJsonSerializable interface
    void SerializeJson(io::JsonWriter& writer) const override;
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Checks if this address equals another address.
     * @param other The other address.
     * @return True if equal.
     */
    bool operator==(const NetworkAddressWithTime& other) const;

    /**
     * @brief Checks if this address does not equal another address.
     * @param other The other address.
     * @return True if not equal.
     */
    bool operator!=(const NetworkAddressWithTime& other) const;

    /**
     * @brief Creates a NetworkAddressWithTime from an IPv4 address.
     * @param timestamp The timestamp.
     * @param services The services.
     * @param ipv4 The IPv4 address string.
     * @param port The port.
     * @return The NetworkAddressWithTime.
     */
    static NetworkAddressWithTime FromIPv4(uint32_t timestamp, uint64_t services, const std::string& ipv4,
                                           uint16_t port);

    /**
     * @brief Creates a NetworkAddressWithTime from an IPv6 address.
     * @param timestamp The timestamp.
     * @param services The services.
     * @param ipv6 The IPv6 address string.
     * @param port The port.
     * @return The NetworkAddressWithTime.
     */
    static NetworkAddressWithTime FromIPv6(uint32_t timestamp, uint64_t services, const std::string& ipv6,
                                           uint16_t port);

   private:
    void ParseIPAddress(const std::string& address);
    std::string FormatIPAddress() const;
};
}  // namespace neo::network::p2p::payloads