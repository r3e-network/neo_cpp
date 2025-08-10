#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/node_capability_types.h>

#include <cstdint>
#include <string>
#include <vector>

namespace neo::network::p2p
{
/**
 * @brief Represents a capability of a node.
 */
class NodeCapability : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs an empty NodeCapability.
     */
    NodeCapability();

    /**
     * @brief Constructs a NodeCapability with the specified type.
     * @param type The type.
     */
    explicit NodeCapability(NodeCapabilityType type);

    /**
     * @brief Gets the type of the capability.
     * @return The type.
     */
    NodeCapabilityType GetType() const;

    /**
     * @brief Sets the type of the capability.
     * @param type The type.
     */
    void SetType(NodeCapabilityType type);

    /**
     * @brief Serializes the NodeCapability to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the NodeCapability from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the NodeCapability to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the NodeCapability from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   protected:
    NodeCapabilityType type_;
};

/**
 * @brief Represents an unknown capability of a node.
 */
class UnknownCapability : public NodeCapability
{
   public:
    /**
     * @brief Constructs an empty UnknownCapability.
     */
    UnknownCapability();

    /**
     * @brief Constructs an UnknownCapability with the specified type.
     * @param type The type.
     */
    explicit UnknownCapability(uint8_t type);

    /**
     * @brief Gets the raw type.
     * @return The raw type.
     */
    uint8_t GetRawType() const;

    /**
     * @brief Sets the raw type.
     * @param rawType The raw type.
     */
    void SetRawType(uint8_t rawType);

    /**
     * @brief Serializes the UnknownCapability to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the UnknownCapability from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the UnknownCapability to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the UnknownCapability from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   private:
    uint8_t rawType_;
};

/**
 * @brief Represents a server capability of a node.
 */
class ServerCapability : public NodeCapability
{
   public:
    /**
     * @brief Constructs an empty ServerCapability.
     */
    ServerCapability();

    /**
     * @brief Constructs a ServerCapability with the specified type and port.
     * @param type The type.
     * @param port The port.
     */
    ServerCapability(NodeCapabilityType type, uint16_t port);

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
     * @brief Serializes the ServerCapability to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the ServerCapability from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the ServerCapability to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the ServerCapability from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   private:
    uint16_t port_;
};

/**
 * @brief Represents a full node capability.
 */
class FullNodeCapability : public NodeCapability
{
   public:
    /**
     * @brief Constructs an empty FullNodeCapability.
     */
    FullNodeCapability();

    /**
     * @brief Constructs a FullNodeCapability with the specified start height.
     * @param startHeight The start height.
     */
    explicit FullNodeCapability(uint32_t startHeight);

    /**
     * @brief Gets the start height.
     * @return The start height.
     */
    uint32_t GetStartHeight() const;

    /**
     * @brief Sets the start height.
     * @param startHeight The start height.
     */
    void SetStartHeight(uint32_t startHeight);

    /**
     * @brief Serializes the FullNodeCapability to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the FullNodeCapability from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the FullNodeCapability to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the FullNodeCapability from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   private:
    uint32_t startHeight_;
};
}  // namespace neo::network::p2p