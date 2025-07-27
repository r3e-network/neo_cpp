#pragma once

#include <cstdint>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <string>

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
     * @brief Constructs a NetworkAddressWithTime with the specified parameters.
     * @param timestamp The timestamp.
     * @param services The services provided by the node.
     * @param address The address.
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
     * @brief Gets the services provided by the node.
     * @return The services provided by the node.
     */
    uint64_t GetServices() const;

    /**
     * @brief Sets the services provided by the node.
     * @param services The services provided by the node.
     */
    void SetServices(uint64_t services);

    /**
     * @brief Gets the address.
     * @return The address.
     */
    const std::string& GetAddress() const;

    /**
     * @brief Sets the address.
     * @param address The address.
     */
    void SetAddress(const std::string& address);

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

    /**
     * @brief Gets the size of the network address with time.
     * @return The size of the network address with time.
     */
    int GetSize() const;

    /**
     * @brief Checks if this NetworkAddressWithTime is equal to another NetworkAddressWithTime.
     * @param other The other NetworkAddressWithTime.
     * @return True if the NetworkAddressWithTime objects are equal, false otherwise.
     */
    bool operator==(const NetworkAddressWithTime& other) const;

    /**
     * @brief Checks if this NetworkAddressWithTime is not equal to another NetworkAddressWithTime.
     * @param other The other NetworkAddressWithTime.
     * @return True if the NetworkAddressWithTime objects are not equal, false otherwise.
     */
    bool operator!=(const NetworkAddressWithTime& other) const;

  private:
    uint32_t timestamp_;
    uint64_t services_;
    std::string address_;
    uint16_t port_;
};
}  // namespace neo::network::p2p
