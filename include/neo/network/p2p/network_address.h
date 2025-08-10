#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/node_capability.h>

#include <cstdint>
#include <vector>

namespace neo::network::p2p
{
/**
 * @brief Represents a network address with a timestamp.
 */
class NetworkAddressWithTime : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs an empty NetworkAddressWithTime.
     */
    NetworkAddressWithTime();

    /**
     * @brief Constructs a NetworkAddressWithTime with the specified timestamp, address, and capabilities.
     * @param timestamp The timestamp.
     * @param address The address.
     * @param capabilities The capabilities.
     */
    NetworkAddressWithTime(uint32_t timestamp, const IPAddress& address,
                           const std::vector<NodeCapability>& capabilities);

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
     * @brief Gets the address.
     * @return The address.
     */
    const IPAddress& GetAddress() const;

    /**
     * @brief Sets the address.
     * @param address The address.
     */
    void SetAddress(const IPAddress& address);

    /**
     * @brief Gets the capabilities.
     * @return The capabilities.
     */
    const std::vector<NodeCapability>& GetCapabilities() const;

    /**
     * @brief Sets the capabilities.
     * @param capabilities The capabilities.
     */
    void SetCapabilities(const std::vector<NodeCapability>& capabilities);

    /**
     * @brief Gets the endpoint.
     * @return The endpoint.
     */
    IPEndPoint GetEndPoint() const;

    /**
     * @brief Gets the size of the NetworkAddressWithTime in bytes.
     * @return The size of the NetworkAddressWithTime.
     */
    uint32_t GetSize() const;

    /**
     * @brief Serializes the NetworkAddressWithTime to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the NetworkAddressWithTime from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the NetworkAddressWithTime to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the NetworkAddressWithTime from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   private:
    uint32_t timestamp_;
    IPAddress address_;
    std::vector<NodeCapability> capabilities_;
};
}  // namespace neo::network::p2p
