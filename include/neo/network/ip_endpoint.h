#pragma once

#include <neo/io/iserializable.h>
#include <neo/network/ip_address.h>
#include <string>
#include <cstdint>

namespace neo::network
{

    /**
     * @brief Represents an IP endpoint (address and port).
     */
    class IPEndPoint : public io::ISerializable
    {
    public:
        /**
         * @brief Constructs an empty IPEndPoint.
         */
        IPEndPoint();

        /**
         * @brief Constructs an IPEndPoint from an address and port.
         * @param address The IP address.
         * @param port The port.
         */
        IPEndPoint(const IPAddress& address, uint16_t port);

        /**
         * @brief Gets the IP address.
         * @return The IP address.
         */
        const IPAddress& GetAddress() const;

        /**
         * @brief Gets the port.
         * @return The port.
         */
        uint16_t GetPort() const;

        /**
         * @brief Gets the IP endpoint as a string.
         * @return The IP endpoint as a string.
         */
        std::string ToString() const;

        /**
         * @brief Serializes the IPEndPoint to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the IPEndPoint from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Checks if this IPEndPoint is equal to another IPEndPoint.
         * @param other The other IPEndPoint.
         * @return True if the IPEndPoints are equal, false otherwise.
         */
        bool operator==(const IPEndPoint& other) const;

        /**
         * @brief Checks if this IPEndPoint is not equal to another IPEndPoint.
         * @param other The other IPEndPoint.
         * @return True if the IPEndPoints are not equal, false otherwise.
         */
        bool operator!=(const IPEndPoint& other) const;

        /**
         * @brief Parses an IP endpoint string.
         * @param endpoint The IP endpoint string.
         * @return The parsed IPEndPoint.
         */
        static IPEndPoint Parse(const std::string& endpoint);

        /**
         * @brief Tries to parse an IP endpoint string.
         * @param endpoint The IP endpoint string.
         * @param result The parsed IPEndPoint.
         * @return True if the parsing was successful, false otherwise.
         */
        static bool TryParse(const std::string& endpoint, IPEndPoint& result);

    private:
        IPAddress address_;
        uint16_t port_;
    };
}
