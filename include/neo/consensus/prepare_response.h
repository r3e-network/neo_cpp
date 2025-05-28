#pragma once

#include <neo/consensus/consensus_message.h>
#include <neo/io/uint256.h>
#include <cstdint>

namespace neo::consensus
{
    /**
     * @brief Represents a prepare response message.
     */
    class PrepareResponse : public ConsensusMessage
    {
    public:
        /**
         * @brief Constructs a PrepareResponse.
         * @param viewNumber The view number.
         * @param preparationHash The preparation hash.
         */
        PrepareResponse(uint8_t viewNumber, const io::UInt256& preparationHash);
        
        /**
         * @brief Gets the preparation hash.
         * @return The preparation hash.
         */
        const io::UInt256& GetPreparationHash() const;
        
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
        io::UInt256 preparationHash_;
    };
}
