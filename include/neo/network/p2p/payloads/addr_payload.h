/**
 * @file addr_payload.h
 * @brief Addr Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/payloads/network_address_with_time.h>

#include <cstdint>
#include <vector>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents an address payload.
 *
 * This message is sent to respond to GetAddr messages.
 */
class AddrPayload : public IPayload
{
   public:
    /**
     * @brief Indicates the maximum number of nodes sent each time.
     */
    static constexpr int MaxCountToSend = 200;

    /**
     * @brief Constructs an empty AddrPayload.
     */
    AddrPayload();

    /**
     * @brief Constructs an AddrPayload with the specified parameters.
     * @param addresses The addresses.
     */
    AddrPayload(const std::vector<NetworkAddressWithTime>& addresses);

    /**
     * @brief Gets the addresses.
     * @return The addresses.
     */
    const std::vector<NetworkAddressWithTime>& GetAddressList() const;

    /**
     * @brief Sets the addresses.
     * @param addresses The addresses.
     */
    void SetAddressList(const std::vector<NetworkAddressWithTime>& addresses);

    /**
     * @brief Gets the size of the payload.
     * @return The size of the payload.
     */
    size_t GetSize() const;

    /**
     * @brief Serializes the AddrPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the AddrPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the AddrPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const;

    /**
     * @brief Deserializes the AddrPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader);

   private:
    std::vector<NetworkAddressWithTime> addressList_;
};
}  // namespace neo::network::p2p::payloads
