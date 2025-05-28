#pragma once

#include <neo/consensus/consensus_message.h>
#include <cstdint>

namespace neo::consensus
{
    /**
     * @brief Represents a change view message.
     */
    class ChangeViewMessage : public ConsensusMessage
    {
    public:
        /**
         * @brief Constructs a ChangeViewMessage.
         * @param viewNumber The view number.
         * @param newViewNumber The new view number.
         * @param timestamp The timestamp.
         */
        ChangeViewMessage(uint8_t viewNumber, uint8_t newViewNumber, uint64_t timestamp);
        
        /**
         * @brief Gets the new view number.
         * @return The new view number.
         */
        uint8_t GetNewViewNumber() const;
        
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
        uint8_t newViewNumber_;
        uint64_t timestamp_;
    };
}
