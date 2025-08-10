#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/block.h>
#include <neo/network/p2p/ipayload.h>

#include <memory>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a block payload.
 *
 * This message is sent to respond to GetBlocks messages.
 */
class BlockPayload : public IPayload
{
   public:
    /**
     * @brief Constructs an empty BlockPayload.
     */
    BlockPayload();

    /**
     * @brief Virtual destructor.
     */
    virtual ~BlockPayload();

    /**
     * @brief Constructs a BlockPayload with the specified block.
     * @param block The block.
     */
    explicit BlockPayload(std::shared_ptr<ledger::Block> block);

    /**
     * @brief Gets the block.
     * @return The block.
     */
    std::shared_ptr<ledger::Block> GetBlock() const;

    /**
     * @brief Sets the block.
     * @param block The block.
     */
    void SetBlock(std::shared_ptr<ledger::Block> block);

    /**
     * @brief Gets the size of the payload.
     * @return The size of the payload.
     */
    int GetSize() const;

    /**
     * @brief Creates a new BlockPayload with the specified block.
     * @param block The block.
     * @return The created payload.
     */
    static BlockPayload Create(std::shared_ptr<ledger::Block> block);

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
    std::shared_ptr<ledger::Block> block_;
};
}  // namespace neo::network::p2p::payloads
