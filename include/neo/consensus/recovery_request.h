#pragma once

#include <neo/consensus/consensus_message.h>
#include <cstdint>

namespace neo::consensus
{
    /**
     * @brief Represents a recovery request message.
     */
    class RecoveryRequest : public ConsensusMessage
    {
    public:
        /**
         * @brief Constructs a RecoveryRequest.
         * @param viewNumber The view number.
         * @param timestamp The timestamp.
         */
        RecoveryRequest(uint8_t viewNumber, uint64_t timestamp);
        
        /**
         * @brief Gets the timestamp.
         * @return The timestamp.
         */
        uint64_t GetTimestamp() const;
        
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
        uint64_t timestamp_;
    };
}
