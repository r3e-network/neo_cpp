#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/byte_vector.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/message_flags.h>
#include <neo/network/p2p/ipayload.h>
#include <string>
#include <cstdint>
#include <memory>

namespace neo::io
{
    class BinaryWriter;
    class BinaryReader;
    class JsonWriter;
    class JsonReader;
}

namespace neo::network
{
    /**
     * @brief Represents a network message.
     */
    class Message : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief The magic number for the main network.
         */
        static constexpr uint32_t MainNetMagic = 0x4F454E;  // "NEO"

        /**
         * @brief The magic number for the test network.
         */
        static constexpr uint32_t TestNetMagic = 0x544E;  // "NT"

        /**
         * @brief The maximum size of a message payload.
         */
        static constexpr uint32_t PayloadMaxSize = 0x02000000;  // 32 MB

        /**
         * @brief The minimum size for compression.
         */
        static constexpr uint32_t CompressionMinSize = 128;

        /**
         * @brief The compression threshold.
         */
        static constexpr uint32_t CompressionThreshold = 16;

        /**
         * @brief Constructs a Message.
         */
        Message();

        /**
         * @brief Constructs a Message.
         * @param command The command.
         * @param payload The payload.
         */
        Message(p2p::MessageCommand command, std::shared_ptr<p2p::IPayload> payload = nullptr);

        /**
         * @brief Constructs a Message with a raw payload.
         * @param command The command.
         * @param payload The raw payload data.
         * @param flags Message flags.
         */
        Message(p2p::MessageCommand command, const io::ByteVector& payload, p2p::MessageFlags flags = p2p::MessageFlags::None);

        /**
         * @brief Gets the flags of the message.
         * @return The flags.
         */
        p2p::MessageFlags GetFlags() const;

        /**
         * @brief Gets the command of the message.
         * @return The command.
         */
        p2p::MessageCommand GetCommand() const;

        /**
         * @brief Gets the payload of the message.
         * @return The payload.
         */
        std::shared_ptr<p2p::IPayload> GetPayload() const;

        /**
         * @brief Gets the raw payload data.
         * @return The raw payload data.
         */
        const io::ByteVector& GetRawPayload() const;

        /**
         * @brief Sets the payload of the message.
         * @param payload The payload.
         */
        void SetPayload(std::shared_ptr<p2p::IPayload> payload);

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
        static Message Create(p2p::MessageCommand command, std::shared_ptr<p2p::IPayload> payload = nullptr);

        /**
         * @brief Serializes the Message to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the Message from a binary reader.
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
         * @brief Serializes the message to a byte vector.
         * @param networkMagic The network magic to use.
         * @return The serialized message.
         */
        io::ByteVector ToArray(uint32_t networkMagic = MainNetMagic) const;

        /**
         * @brief Deserializes the message from a byte vector.
         * @param data The serialized message.
         * @param networkMagic The network magic to expect.
         * @return Whether the deserialization was successful.
         */
        bool FromArray(const io::ByteVector& data, uint32_t networkMagic = MainNetMagic);

    private:
        p2p::MessageFlags flags_;
        p2p::MessageCommand command_;
        std::shared_ptr<p2p::IPayload> payload_;
        io::ByteVector payloadRaw_;
        io::ByteVector payloadCompressed_;

        void DecompressPayload();
        static bool ShouldCompress(p2p::MessageCommand command);
    };

}
