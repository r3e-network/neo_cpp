#pragma once

#include <cstdint>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <string>

namespace neo::network
{
/**
 * @brief Represents a network address.
 */
class NetworkAddress : public io::ISerializable, public io::IJsonSerializable
{
  public:
    /**
     * @brief Constructs an empty NetworkAddress.
     */
    NetworkAddress();

    /**
     * @brief Constructs a NetworkAddress with the specified parameters.
     * @param timestamp The timestamp.
     * @param services The services provided by the node.
     * @param address The address.
     * @param port The port.
     */
    NetworkAddress(uint32_t timestamp, uint64_t services, const std::string& address, uint16_t port);

    /**
     * @brief Gets the timestamp.
     * @return The timestamp.
     */
    uint32_t GetTimestamp() const;

    /**
     * @brief Gets the services provided by the node.
     * @return The services provided by the node.
     */
    uint64_t GetServices() const;

    /**
     * @brief Gets the address.
     * @return The address.
     */
    const std::string& GetAddress() const;

    /**
     * @brief Gets the port.
     * @return The port.
     */
    uint16_t GetPort() const;

    /**
     * @brief Serializes the NetworkAddress to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the NetworkAddress from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the NetworkAddress to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the NetworkAddress from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

  private:
    uint32_t timestamp_;
    uint64_t services_;
    std::string address_;
    uint16_t port_;
};
}  // namespace neo::network
