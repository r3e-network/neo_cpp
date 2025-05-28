#pragma once

#include <neo/consensus/message_type.h>
#include <neo/io/serializable.h>
#include <neo/io/byte_vector.h>
#include <neo/cryptography/ecc/keypair.h>
#include <cstdint>
#include <memory>

namespace neo::consensus
{
    /**
     * @brief Represents a consensus message.
     */
    class ConsensusMessage : public io::ISerializable
    {
    public:
        /**
         * @brief Constructs a ConsensusMessage.
         * @param type The message type.
         * @param viewNumber The view number.
         */
        ConsensusMessage(MessageType type, uint8_t viewNumber);
        
        /**
         * @brief Destructor.
         */
        virtual ~ConsensusMessage() = default;
        
        /**
         * @brief Gets the message type.
         * @return The message type.
         */
        MessageType GetType() const;
        
        /**
         * @brief Gets the view number.
         * @return The view number.
         */
        uint8_t GetViewNumber() const;
        
        /**
         * @brief Gets the validator index.
         * @return The validator index.
         */
        uint16_t GetValidatorIndex() const;
        
        /**
         * @brief Sets the validator index.
         * @param validatorIndex The validator index.
         */
        void SetValidatorIndex(uint16_t validatorIndex);
        
        /**
         * @brief Gets the signature.
         * @return The signature.
         */
        const io::ByteVector& GetSignature() const;
        
        /**
         * @brief Sets the signature.
         * @param signature The signature.
         */
        void SetSignature(const io::ByteVector& signature);
        
        /**
         * @brief Serializes the object.
         * @param writer The writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;
        
        /**
         * @brief Deserializes the object.
         * @param reader The reader.
         */
        void Deserialize(io::BinaryReader& reader) override;
        
        /**
         * @brief Gets the message data.
         * @return The message data.
         */
        virtual io::ByteVector GetData() const;
        
        /**
         * @brief Verifies the signature.
         * @param publicKey The public key.
         * @return True if the signature is valid, false otherwise.
         */
        bool VerifySignature(const cryptography::ecc::ECPoint& publicKey) const;
        
        /**
         * @brief Signs the message.
         * @param keyPair The key pair.
         */
        void Sign(const cryptography::ecc::KeyPair& keyPair);
        
    private:
        MessageType type_;
        uint8_t viewNumber_;
        uint16_t validatorIndex_;
        io::ByteVector signature_;
    };
}
