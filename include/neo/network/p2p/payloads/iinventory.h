#pragma once

#include <neo/io/uint256.h>
#include <neo/network/p2p/inventory_type.h>

#include <cstdint>

namespace neo::network::p2p::payloads
{
/**
 * @brief Inventory types for the Neo network protocol.
 */
// Use the canonical InventoryType from neo::network::p2p
using InventoryType = neo::network::p2p::InventoryType;

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