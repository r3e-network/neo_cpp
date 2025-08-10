#pragma once

/**
 * @file iinventory.h
 * @brief Interface for inventory items that can be relayed on the NEO network
 *
 * This header defines the IInventory interface which represents objects
 * that can be relayed and verified on the NEO network.
 */

#include <neo/io/uint256.h>
#include <neo/network/p2p/inventory_type.h>

namespace neo::network::p2p
{
/**
 * @brief Interface for inventory items that can be relayed on the NEO network
 *
 * Represents a message that can be relayed on the NEO network.
 * This interface extends the concept of verifiable objects to include
 * inventory type information.
 */
class IInventory
{
   public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IInventory() = default;

    /**
     * @brief Gets the hash of the inventory item
     * @return The hash of the inventory item
     */
    virtual const io::UInt256& GetHash() const = 0;

    /**
     * @brief Gets the inventory type
     * @return The type of the inventory
     */
    virtual InventoryType GetInventoryType() const = 0;
};
}  // namespace neo::network::p2p
