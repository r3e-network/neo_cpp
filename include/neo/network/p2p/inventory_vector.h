#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/uint256.h>
#include <neo/network/p2p/inventory_type.h>

#include <cstdint>

namespace neo::network::p2p
{
/**
 * @brief Represents an inventory vector.
 */
class InventoryVector : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs an empty InventoryVector.
     */
    InventoryVector();

    /**
     * @brief Constructs an InventoryVector with the specified type and hash.
     * @param type The type.
     * @param hash The hash.
     */
    InventoryVector(InventoryType type, const io::UInt256& hash);

    /**
     * @brief Gets the type of the inventory.
     * @return The type.
     */
    InventoryType GetType() const;

    /**
     * @brief Sets the type of the inventory.
     * @param type The type.
     */
    void SetType(InventoryType type);

    /**
     * @brief Gets the hash of the inventory.
     * @return The hash.
     */
    const io::UInt256& GetHash() const;

    /**
     * @brief Sets the hash of the inventory.
     * @param hash The hash.
     */
    void SetHash(const io::UInt256& hash);

    /**
     * @brief Serializes the InventoryVector to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the InventoryVector from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the InventoryVector to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the InventoryVector from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Checks if this InventoryVector is equal to another InventoryVector.
     * @param other The other InventoryVector.
     * @return True if the InventoryVectors are equal, false otherwise.
     */
    bool operator==(const InventoryVector& other) const;

    /**
     * @brief Checks if this InventoryVector is not equal to another InventoryVector.
     * @param other The other InventoryVector.
     * @return True if the InventoryVectors are not equal, false otherwise.
     */
    bool operator!=(const InventoryVector& other) const;

   private:
    InventoryType type_;
    io::UInt256 hash_;
};
}  // namespace neo::network::p2p
