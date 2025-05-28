#pragma once

#include <neo/network/ipayload.h>
#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/blockchain/block.h>
#include <memory>

namespace neo::network::payloads
{
    /**
     * @brief Represents a block payload.
     */
    class BlockPayload : public IPayload, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty BlockPayload.
         */
        BlockPayload();

        /**
         * @brief Constructs a BlockPayload with the specified block.
         * @param block The block.
         */
        explicit BlockPayload(std::shared_ptr<blockchain::Block> block);

        /**
         * @brief Gets the block.
         * @return The block.
         */
        std::shared_ptr<blockchain::Block> GetBlock() const;

        /**
         * @brief Sets the block.
         * @param block The block.
         */
        void SetBlock(std::shared_ptr<blockchain::Block> block);

        /**
         * @brief Serializes the BlockPayload to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the BlockPayload from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the BlockPayload to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the BlockPayload from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

    private:
        std::shared_ptr<blockchain::Block> block_;
    };
}
