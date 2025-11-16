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
#include <neo/network/ip_address.h>
#include <neo/network/p2p/node_capability.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

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
    static constexpr size_t AddressSize = 16;

    NetworkAddressWithTime();
    NetworkAddressWithTime(uint32_t timestamp, const network::IPAddress& address,
                           const std::vector<NodeCapability>& capabilities);

    uint32_t GetTimestamp() const;
    void SetTimestamp(uint32_t timestamp);

    const network::IPAddress& GetIPAddress() const;
    std::string GetAddress() const;
    void SetAddress(const network::IPAddress& address);
    void SetAddress(const std::string& address);

    const std::vector<NodeCapability>& GetCapabilities() const;
    void SetCapabilities(const std::vector<NodeCapability>& capabilities);

    uint16_t GetPort() const;
    void SetPort(uint16_t port);

    size_t GetSize() const;

    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

    void SerializeJson(io::JsonWriter& writer) const override;
    void DeserializeJson(const io::JsonReader& reader) override;

    bool operator==(const NetworkAddressWithTime& other) const;
    bool operator!=(const NetworkAddressWithTime& other) const;

    static NetworkAddressWithTime FromIPv4(uint32_t timestamp, const std::string& address, uint16_t port,
                                           const std::vector<NodeCapability>& capabilities = {});
    static NetworkAddressWithTime FromIPv6(uint32_t timestamp, const std::string& address, uint16_t port,
                                           const std::vector<NodeCapability>& capabilities = {});

   private:
    std::array<uint8_t, AddressSize> ToIPv6Bytes() const;
    void FromIPv6Bytes(const std::array<uint8_t, AddressSize>& bytes);
    NodeCapability* FindTcpCapability();
    const NodeCapability* FindTcpCapability() const;

    uint32_t timestamp_;
    network::IPAddress address_;
    std::vector<NodeCapability> capabilities_;
};
}  // namespace neo::network::p2p::payloads
