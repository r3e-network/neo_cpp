/**
 * @file inventory_vector.h
 * @brief Inventory Vector
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/uint256.h>
#include <neo/network/inventory_type.h>

namespace neo::network
{

/**
 * @brief Represents an inventory vector used in network messages.
 */
class InventoryVector : public io::ISerializable
{
   public:
    /**
     * @brief Default constructor.
     */
    InventoryVector() : type_(InventoryType::Transaction) {}

    /**
     * @brief Constructs an inventory vector with specified type and hash.
     * @param type The inventory type.
     * @param hash The inventory hash.
     */
    InventoryVector(InventoryType type, const io::UInt256& hash) : type_(type), hash_(hash) {}

    /**
     * @brief Gets the inventory type.
     * @return The inventory type.
     */
    InventoryType GetType() const { return type_; }

    /**
     * @brief Sets the inventory type.
     * @param type The inventory type.
     */
    void SetType(InventoryType type) { type_ = type; }

    /**
     * @brief Gets the inventory hash.
     * @return The inventory hash.
     */
    const io::UInt256& GetHash() const { return hash_; }

    /**
     * @brief Sets the inventory hash.
     * @param hash The inventory hash.
     */
    void SetHash(const io::UInt256& hash) { hash_ = hash; }

    /**
     * @brief Serializes the inventory vector.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override
    {
        writer.Write(static_cast<uint8_t>(type_));
        writer.Write(hash_);
    }

    /**
     * @brief Deserializes the inventory vector.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override
    {
        type_ = static_cast<InventoryType>(reader.ReadUInt8());
        hash_ = reader.ReadSerializable<io::UInt256>();
    }

    /**
     * @brief Checks equality.
     * @param other The other inventory vector.
     * @return True if equal, false otherwise.
     */
    bool operator==(const InventoryVector& other) const { return type_ == other.type_ && hash_ == other.hash_; }

    /**
     * @brief Checks inequality.
     * @param other The other inventory vector.
     * @return True if not equal, false otherwise.
     */
    bool operator!=(const InventoryVector& other) const { return !(*this == other); }

   private:
    InventoryType type_;
    io::UInt256 hash_;
};

}  // namespace neo::network