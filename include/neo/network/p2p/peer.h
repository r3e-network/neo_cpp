/**
 * @file peer.h
 * @brief Peer
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

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
#include <string>
#include <vector>

namespace neo::network::p2p
{
/**
 * @brief Represents a peer in the P2P network.
 */
class Peer : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs an empty Peer.
     */
    Peer();

    /**
     * @brief Constructs a Peer with the specified endpoint.
     * @param endpoint The endpoint.
     */
    explicit Peer(const IPEndPoint& endpoint);

    /**
     * @brief Constructs a Peer with the specified endpoint, version, and capabilities.
     * @param endpoint The endpoint.
     * @param version The version.
     * @param capabilities The capabilities.
     */
    Peer(const IPEndPoint& endpoint, uint32_t version, const std::vector<NodeCapability>& capabilities);

    /**
     * @brief Gets the endpoint of the peer.
     * @return The endpoint.
     */
    const IPEndPoint& GetEndPoint() const;

    /**
     * @brief Sets the endpoint of the peer.
     * @param endpoint The endpoint.
     */
    void SetEndPoint(const IPEndPoint& endpoint);

    /**
     * @brief Gets the version of the peer.
     * @return The version.
     */
    uint32_t GetVersion() const;

    /**
     * @brief Sets the version of the peer.
     * @param version The version.
     */
    void SetVersion(uint32_t version);

    /**
     * @brief Gets the capabilities of the peer.
     * @return The capabilities.
     */
    const std::vector<NodeCapability>& GetCapabilities() const;

    /**
     * @brief Sets the capabilities of the peer.
     * @param capabilities The capabilities.
     */
    void SetCapabilities(const std::vector<NodeCapability>& capabilities);

    /**
     * @brief Gets the last connection time of the peer.
     * @return The last connection time.
     */
    uint64_t GetLastConnectionTime() const;

    /**
     * @brief Sets the last connection time of the peer.
     * @param lastConnectionTime The last connection time.
     */
    void SetLastConnectionTime(uint64_t lastConnectionTime);

    /**
     * @brief Gets the last seen time of the peer.
     * @return The last seen time.
     */
    uint64_t GetLastSeenTime() const;

    /**
     * @brief Sets the last seen time of the peer.
     * @param lastSeenTime The last seen time.
     */
    void SetLastSeenTime(uint64_t lastSeenTime);

    /**
     * @brief Gets the connection attempts of the peer.
     * @return The connection attempts.
     */
    uint32_t GetConnectionAttempts() const;

    /**
     * @brief Sets the connection attempts of the peer.
     * @param connectionAttempts The connection attempts.
     */
    void SetConnectionAttempts(uint32_t connectionAttempts);

    /**
     * @brief Increments the connection attempts of the peer.
     */
    void IncrementConnectionAttempts();

    /**
     * @brief Gets whether the peer is connected.
     * @return True if the peer is connected, false otherwise.
     */
    bool IsConnected() const;

    /**
     * @brief Sets whether the peer is connected.
     * @param connected Whether the peer is connected.
     */
    void SetConnected(bool connected);

    /**
     * @brief Gets whether the peer is bad.
     * @return True if the peer is bad, false otherwise.
     */
    bool IsBad() const;

    /**
     * @brief Sets whether the peer is bad.
     * @param bad Whether the peer is bad.
     */
    void SetBad(bool bad);

    /**
     * @brief Serializes the Peer to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the Peer from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the Peer to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the Peer from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Checks if this Peer is equal to another Peer.
     * @param other The other Peer.
     * @return True if the Peers are equal, false otherwise.
     */
    bool operator==(const Peer& other) const;

    /**
     * @brief Checks if this Peer is not equal to another Peer.
     * @param other The other Peer.
     * @return True if the Peers are not equal, false otherwise.
     */
    bool operator!=(const Peer& other) const;

   private:
    IPEndPoint endpoint_;
    uint32_t version_;
    std::vector<NodeCapability> capabilities_;
    uint64_t lastConnectionTime_;
    uint64_t lastSeenTime_;
    uint32_t connectionAttempts_;
    bool connected_;
    bool bad_;
};
}  // namespace neo::network::p2p
