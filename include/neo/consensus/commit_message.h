#pragma once

#include <neo/consensus/consensus_message.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <cstdint>

namespace neo::consensus
{
    /**
     * @brief Represents a commit message.
     */
    class CommitMessage : public ConsensusMessage
    {
    public:
        /**
         * @brief Constructs a CommitMessage.
         * @param viewNumber The view number.
         * @param commitHash The commit hash.
         * @param signature The signature.
         */
        CommitMessage(uint8_t viewNumber, const io::UInt256& commitHash, const io::ByteVector& signature);
        
        /**
         * @brief Gets the commit hash.
         * @return The commit hash.
         */
        const io::UInt256& GetCommitHash() const;
        
        /**
         * @brief Gets the commit signature.
         * @return The commit signature.
         */
        const io::ByteVector& GetCommitSignature() const;
        
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
        io::ByteVector GetData() const override;
        
    private:
        io::UInt256 commitHash_;
        io::ByteVector commitSignature_;
    };
}
