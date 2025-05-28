#pragma once

#include <neo/network/ipayload.h>
#include <neo/network/network_address.h>
#include <vector>
#include <cstdint>

namespace neo::network::payloads
{
    /**
     * @brief Represents an address payload.
     */
    class AddrPayload : public IPayload
    {
    public:
        /**
         * @brief Constructs an empty AddrPayload.
         */
        AddrPayload();

        /**
         * @brief Constructs an AddrPayload with the specified parameters.
         * @param addresses The addresses.
         */
        AddrPayload(const std::vector<NetworkAddress>& addresses);

        /**
         * @brief Gets the addresses.
         * @return The addresses.
         */
        const std::vector<NetworkAddress>& GetAddresses() const;

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
        void SerializeJson(io::JsonWriter& writer) const override;
        
        /**
         * @brief Deserializes the AddrPayload from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

    private:
        std::vector<NetworkAddress> addresses_;
    };
}
