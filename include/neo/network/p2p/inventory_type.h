#pragma once

#include <cstdint>

namespace neo::network::p2p
{
/**
 * @brief Represents the type of an inventory item.
 */
enum class InventoryType : uint8_t
{
    /**
     * @brief Transaction inventory type.
     */
    Transaction = 0x01,

    /**
     * @brief Block inventory type.
     */
    Block = 0x02,

    /**
     * @brief Consensus data inventory type.
     */
    Consensus = 0xe0,

    /**
     * @brief Extensible payload inventory type.
     */
    Extensible = 0xf0
};
}  // namespace neo::network::p2p
