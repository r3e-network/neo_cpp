#pragma once

#include <neo/consensus/message_type.h>
#include <neo/io/iserializable.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>
#include <neo/protocol_settings.h>
#include <cstdint>
#include <memory>

namespace neo::consensus
{
    /**
     * @brief Base class for consensus messages.
     * 
     * In Neo N3, consensus messages are serialized and sent within
     * ExtensiblePayload with category "dBFT".
     */
    class ConsensusMessage : public io::ISerializable
    {
    public:
        /**
         * @brief Constructs a ConsensusMessage with the specified type.
         * @param type The message type.
         */
        explicit ConsensusMessage(MessageType type);

        /**
         * @brief Virtual destructor.
         */
        virtual ~ConsensusMessage() = default;

        /**
         * @brief Gets the message type.
         * @return The message type.
         */
        MessageType GetType() const { return type_; }

        /**
         * @brief Gets the block index.
         * @return The block index.
         */
        uint32_t GetBlockIndex() const { return blockIndex_; }

        /**
         * @brief Sets the block index.
         * @param index The block index.
         */
        void SetBlockIndex(uint32_t index) { blockIndex_ = index; }

        /**
         * @brief Gets the validator index.
         * @return The validator index.
         */
        uint8_t GetValidatorIndex() const { return validatorIndex_; }

        /**
         * @brief Sets the validator index.
         * @param index The validator index.
         */
        void SetValidatorIndex(uint8_t index) { validatorIndex_ = index; }

        /**
         * @brief Gets the view number.
         * @return The view number.
         */
        uint8_t GetViewNumber() const { return viewNumber_; }

        /**
         * @brief Sets the view number.
         * @param viewNumber The view number.
         */
        void SetViewNumber(uint8_t viewNumber) { viewNumber_ = viewNumber; }

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
         * @brief Gets the size of the message.
         * @return The size of the message.
         */
        virtual size_t GetSize() const;

        /**
         * @brief Verifies the message.
         * @param settings The protocol settings.
         * @return True if valid, false otherwise.
         */
        virtual bool Verify(const ProtocolSettings& settings) const;

        /**
         * @brief Deserializes a consensus message from data.
         * @param data The data to deserialize.
         * @return The deserialized message.
         */
        static std::shared_ptr<ConsensusMessage> DeserializeFrom(const io::ByteVector& data);

    protected:
        MessageType type_;
        uint32_t blockIndex_ = 0;
        uint8_t validatorIndex_ = 0;
        uint8_t viewNumber_ = 0;
    };
}