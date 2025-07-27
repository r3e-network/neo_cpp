#pragma once

#include <cstdint>
#include <neo/io/uint256.h>

namespace neo::network::p2p::payloads
{
/**
 * @brief Inventory types for the Neo network protocol.
 */
enum class InventoryType : uint8_t
{
    TX = 0x2b,         // Transaction
    Block = 0x2c,      // Block
    Extensible = 0x2e  // Extensible payload
};

/**
 * @brief Interface for inventory items in the P2P network.
 */
class IInventory
{
  public:
    virtual ~IInventory() = default;

    /**
     * @brief Gets the inventory type.
     * @return The inventory type.
     */
    virtual InventoryType GetInventoryType() const = 0;

    /**
     * @brief Gets the hash of the inventory item.
     * @return The hash.
     */
    virtual io::UInt256 GetHash() const = 0;

    /**
     * @brief Gets the size of the inventory item.
     * @return The size in bytes.
     */
    virtual int GetSize() const = 0;
};
}  // namespace neo::network::p2p::payloads