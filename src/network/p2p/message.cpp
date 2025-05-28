#include <neo/network/p2p/message.h>
#include <neo/network/p2p/ipayload.h>
#include <neo/network/payload_factory.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/lz4.h>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace neo::network::p2p
{
    Message::Message()
        : flags_(MessageFlags::None), command_(MessageCommand::Version)
    {
    }

    Message::Message(MessageCommand command, std::shared_ptr<IPayload> payload)
        : flags_(MessageFlags::None), command_(command), payload_(payload)
    {
        if (payload)
        {
            std::ostringstream stream;
            io::BinaryWriter writer(stream);
            payload->Serialize(writer);
            std::string data = stream.str();
            payloadRaw_ = io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
            payloadCompressed_ = payloadRaw_;
        }
    }

    MessageFlags Message::GetFlags() const
    {
        return flags_;
    }

    MessageCommand Message::GetCommand() const
    {
        return command_;
    }

    std::shared_ptr<IPayload> Message::GetPayload() const
    {
        return payload_;
    }

    uint32_t Message::GetSize() const
    {
        return 2 + (IsCompressed() ? payloadCompressed_.Size() : payloadRaw_.Size()) + 1;
    }

    bool Message::IsCompressed() const
    {
        return HasFlag(flags_, MessageFlags::Compressed);
    }

    Message Message::Create(MessageCommand command, std::shared_ptr<IPayload> payload)
    {
        bool tryCompression = ShouldCompress(command);

        Message message(command, payload);

        // Try compression
        if (tryCompression && message.payloadRaw_.Size() > CompressionMinSize)
        {
            // Implement LZ4 compression
            try
            {
                io::ByteVector compressed = cryptography::LZ4::Compress(message.payloadRaw_.AsSpan());

                if (compressed.Size() < message.payloadRaw_.Size() - CompressionThreshold)
                {
                    message.payloadCompressed_ = compressed;
                    message.flags_ = SetFlag(message.flags_, MessageFlags::Compressed);
                }
            }
            catch (const std::exception&)
            {
                // If compression fails, just use the raw payload
                message.payloadCompressed_ = message.payloadRaw_;
            }
        }

        return message;
    }

    void Message::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(flags_));
        writer.Write(static_cast<uint8_t>(command_));

        if (IsCompressed())
        {
            writer.WriteVarBytes(payloadCompressed_.AsSpan());
        }
        else
        {
            writer.WriteVarBytes(payloadRaw_.AsSpan());
        }
    }

    void Message::Deserialize(io::BinaryReader& reader)
    {
        flags_ = static_cast<MessageFlags>(reader.ReadUInt8());
        command_ = static_cast<MessageCommand>(reader.ReadUInt8());
        payloadCompressed_ = reader.ReadVarBytes(PayloadMaxSize);
        DecompressPayload();
    }

    void Message::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("flags", static_cast<uint8_t>(flags_));
        writer.Write("command", static_cast<uint8_t>(command_));

        if (payload_)
        {
            nlohmann::json payloadJson = nlohmann::json::object();
            io::JsonWriter payloadWriter(payloadJson);
            payload_->SerializeJson(payloadWriter);
            writer.Write("payload", payloadJson);
        }
        else
        {
            writer.Write("payload", nlohmann::json::object());
        }
    }

    void Message::DeserializeJson(const io::JsonReader& reader)
    {
        flags_ = static_cast<MessageFlags>(reader.ReadUInt8("flags"));
        command_ = static_cast<MessageCommand>(reader.ReadUInt8("command"));

        // TODO: Deserialize payload based on command
        // For now, just leave payload_ as nullptr
    }

    io::ByteVector Message::ToArray(bool enableCompression) const
    {
        if (enableCompression || !IsCompressed())
        {
            std::ostringstream stream;
            io::BinaryWriter writer(stream);
            Serialize(writer);
            std::string data = stream.str();
            return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        }
        else
        {
            // Avoid compression
            std::ostringstream stream;
            io::BinaryWriter writer(stream);

            writer.Write(static_cast<uint8_t>(ClearFlag(flags_, MessageFlags::Compressed)));
            writer.Write(static_cast<uint8_t>(command_));
            writer.WriteVarBytes(payloadRaw_.AsSpan());

            std::string data = stream.str();
            return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        }
    }

    uint32_t Message::TryDeserialize(const io::ByteSpan& data, Message& message)
    {
        try
        {
            std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
            io::BinaryReader reader(stream);
            message.Deserialize(reader);
            return static_cast<uint32_t>(stream.tellg());
        }
        catch (const std::exception&)
        {
            return 0;
        }
    }

    void Message::DecompressPayload()
    {
        if (payloadCompressed_.IsEmpty())
            return;

        if (IsCompressed())
        {
            // Implement LZ4 decompression
            try
            {
                payloadRaw_ = cryptography::LZ4::Decompress(payloadCompressed_.AsSpan(), PayloadMaxSize);
            }
            catch (const std::exception&)
            {
                // If decompression fails, just use the compressed payload
                payloadRaw_ = payloadCompressed_;
            }
        }
        else
        {
            payloadRaw_ = payloadCompressed_;
        }

        // Create payload based on command
        try
        {
            std::istringstream stream(std::string(reinterpret_cast<const char*>(payloadRaw_.Data()), payloadRaw_.Size()));
            io::BinaryReader reader(stream);
            payload_ = network::PayloadFactory::DeserializePayload(command_, reader);
        }
        catch (const std::exception&)
        {
            // If deserialization fails, just leave payload_ as nullptr
            payload_ = nullptr;
        }
    }

    bool Message::ShouldCompress(MessageCommand command)
    {
        switch (command)
        {
            case MessageCommand::Block:
            case MessageCommand::Transaction:
            case MessageCommand::Extensible:
            case MessageCommand::Headers:
            case MessageCommand::Inv:
            case MessageCommand::Addr:
                return true;
            default:
                return false;
        }
    }
}
