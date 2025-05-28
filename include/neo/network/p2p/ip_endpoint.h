#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <string>
#include <cstdint>

namespace neo::network::p2p
{
    /**
     * @brief Represents an IP address and port.
     */
    class IPEndPoint : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty IPEndPoint.
         */
        IPEndPoint();

        /**
         * @brief Constructs an IPEndPoint with the specified parameters.
         * @param address The address.
         * @param port The port.
         */
        IPEndPoint(const std::string& address, uint16_t port);

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
         * @brief Serializes the IPEndPoint to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the IPEndPoint from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Checks if this IPEndPoint is equal to another IPEndPoint.
         * @param other The other IPEndPoint.
         * @return True if the IPEndPoint objects are equal, false otherwise.
         */
        bool operator==(const IPEndPoint& other) const;

        /**
         * @brief Checks if this IPEndPoint is not equal to another IPEndPoint.
         * @param other The other IPEndPoint.
         * @return True if the IPEndPoint objects are not equal, false otherwise.
         */
        bool operator!=(const IPEndPoint& other) const;

        /**
         * @brief Gets a string representation of the IPEndPoint.
         * @return A string representation of the IPEndPoint.
         */
        std::string ToString() const;

        /**
         * @brief Parses a string representation of an IPEndPoint.
         * @param str The string representation of an IPEndPoint.
         * @return The parsed IPEndPoint.
         */
        static IPEndPoint Parse(const std::string& str);

        /**
         * @brief Tries to parse a string representation of an IPEndPoint.
         * @param str The string representation of an IPEndPoint.
         * @param result The parsed IPEndPoint.
         * @return True if the parsing was successful, false otherwise.
         */
        static bool TryParse(const std::string& str, IPEndPoint& result);

    private:
        std::string address_;
        uint16_t port_;
    };
}
