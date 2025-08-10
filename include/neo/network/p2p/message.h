#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/message_flags.h>

#include <cstdint>
#include <memory>
#include <string>

namespace neo::network::p2p
{
// Forward declarations
class IPayload;

/**
 * @brief Represents a message on the NEO network.
 */
class Message : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Indicates the maximum size of the payload.
     */
    static constexpr uint32_t PayloadMaxSize = 0x02000000;

    /**
     * @brief The minimum size for compression.
     */
    static constexpr uint32_t CompressionMinSize = 128;

    /**
     * @brief The compression threshold.
     */
    static constexpr uint32_t CompressionThreshold = 64;

    /**
     * @brief Constructs an empty Message.
     */
    Message();

    /**
     * @brief Constructs a Message with the specified command and payload.
     * @param command The command.
     * @param payload The payload.
     */
    Message(MessageCommand command, std::shared_ptr<IPayload> payload);

    /**
     * @brief Gets the flags of the message.
     * @return The flags.
     */
    MessageFlags GetFlags() const;

    /**
     * @brief Gets the command of the message.
     * @return The command.
     */
    MessageCommand GetCommand() const;

    /**
     * @brief Gets the payload of the message.
     * @return The payload.
     */
    std::shared_ptr<IPayload> GetPayload() const;

    /**
     * @brief Gets the size of the message.
     * @return The size.
     */
    uint32_t GetSize() const;

    /**
     * @brief Checks if the message is compressed.
     * @return True if the message is compressed, false otherwise.
     */
    bool IsCompressed() const;

    /**
     * @brief Creates a new Message.
     * @param command The command.
     * @param payload The payload.
     * @return The message.
     */
    static Message Create(MessageCommand command, std::shared_ptr<IPayload> payload = nullptr);

    /**
     * @brief Serializes the message to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the message from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the message to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the message from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Converts the message to a byte array.
     * @param enableCompression Whether to enable compression.
     * @return The byte array.
     */
    io::ByteVector ToArray(bool enableCompression = true) const;

    /**
     * @brief Tries to deserialize a message from a byte array.
     * @param data The byte array.
     * @param message The message to deserialize into.
     * @return The number of bytes read, or 0 if deserialization failed.
     */
    static uint32_t TryDeserialize(const io::ByteSpan& data, Message& message);

   private:
    MessageFlags flags_;
    MessageCommand command_;
    std::shared_ptr<IPayload> payload_;
    io::ByteVector payloadRaw_;
    io::ByteVector payloadCompressed_;

    void DecompressPayload();
    static bool ShouldCompress(MessageCommand command);
    static std::string GetCommandString(MessageCommand command);
    static MessageCommand GetCommandFromString(const std::string& commandStr);
    static uint32_t CalculatePayloadChecksum(const io::ByteSpan& payload);
};
}  // namespace neo::network::p2p
