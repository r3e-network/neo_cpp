#pragma once

#include <neo/io/uint256.h>
#include <neo/network/inventory_type.h>
#include <neo/network/ipayload.h>

#include <cstdint>
#include <vector>

namespace neo::network::payloads
{
/**
 * @brief Represents an inventory payload.
 */
class InventoryPayload : public IPayload
{
   public:
    /**
     * @brief Constructs an empty InventoryPayload.
     */
    InventoryPayload();

    /**
     * @brief Constructs an InventoryPayload with the specified parameters.
     * @param type The inventory type.
     * @param hashes The hashes.
     */
    InventoryPayload(InventoryType type, const std::vector<io::UInt256>& hashes);

    /**
     * @brief Gets the inventory type.
     * @return The inventory type.
     */
    InventoryType GetType() const;

    /**
     * @brief Gets the hashes.
     * @return The hashes.
     */
    const std::vector<io::UInt256>& GetHashes() const;

    /**
     * @brief Serializes the InventoryPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the InventoryPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the InventoryPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the InventoryPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   private:
    InventoryType type_;
    std::vector<io::UInt256> hashes_;
};
}  // namespace neo::network::payloads
