#pragma once

#include <cstdint>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/node_capability.h>
#include <string>
#include <vector>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a version message payload.
 */
class VersionPayload : public IPayload
{
  public:
    /**
     * @brief Indicates the maximum number of capabilities contained in a VersionPayload.
     */
    static constexpr int MaxCapabilities = 32;

    /**
     * @brief Constructs an empty VersionPayload.
     */
    VersionPayload();

    /**
     * @brief Gets the magic number of the network.
     * @return The network magic number.
     */
    uint32_t GetNetwork() const;

    /**
     * @brief Sets the magic number of the network.
     * @param network The network magic number.
     */
    void SetNetwork(uint32_t network);

    /**
     * @brief Gets the protocol version.
     * @return The protocol version.
     */
    uint32_t GetVersion() const;

    /**
     * @brief Sets the protocol version.
     * @param version The protocol version.
     */
    void SetVersion(uint32_t version);

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
     * @brief Gets the nonce.
     * @return The nonce.
     */
    uint32_t GetNonce() const;

    /**
     * @brief Sets the nonce.
     * @param nonce The nonce.
     */
    void SetNonce(uint32_t nonce);

    /**
     * @brief Gets the user agent.
     * @return The user agent.
     */
    const std::string& GetUserAgent() const;

    /**
     * @brief Sets the user agent.
     * @param userAgent The user agent.
     */
    void SetUserAgent(const std::string& userAgent);

    /**
     * @brief Gets whether compression is allowed.
     * @return Whether compression is allowed.
     */
    bool GetAllowCompression() const;

    /**
     * @brief Sets whether compression is allowed.
     * @param allowCompression Whether compression is allowed.
     */
    void SetAllowCompression(bool allowCompression);

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
     * @brief Creates a new VersionPayload with the specified parameters.
     * @param network The magic number of the network.
     * @param nonce The nonce.
     * @param userAgent The user agent.
     * @param capabilities The capabilities.
     * @return A new VersionPayload.
     */
    static VersionPayload Create(uint32_t network, uint32_t nonce, const std::string& userAgent,
                                 const std::vector<NodeCapability>& capabilities);

    /**
     * @brief Serializes the VersionPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the VersionPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the VersionPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const;

    /**
     * @brief Deserializes the VersionPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader);

    /**
     * @brief Gets the start height from the FullNode capability.
     * @return The start height if a FullNode capability exists, 0 otherwise.
     */
    uint32_t GetStartHeight() const;

    /**
     * @brief Gets the size of the payload.
     * @return The size of the payload.
     */
    size_t GetSize() const;

  private:
    uint32_t network_;
    uint32_t version_;
    uint32_t timestamp_;
    uint32_t nonce_;
    std::string userAgent_;
    bool allowCompression_;
    std::vector<NodeCapability> capabilities_;
};
}  // namespace neo::network::p2p::payloads
