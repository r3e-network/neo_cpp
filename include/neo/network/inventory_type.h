#pragma once

#include <cstdint>

namespace neo::network
{
/**
 * @brief Represents an inventory type.
 */
enum class InventoryType : uint8_t
{
    /**
     * @brief Transaction inventory.
     */
    Transaction = 0x2b,  // Neo N3 value

    /**
     * @brief Alias for Transaction (backward compatibility).
     */
    TX = 0x2b,

    /**
     * @brief Block inventory.
     */
    Block = 0x2c,  // Neo N3 value

    /**
     * @brief Extensible payload inventory.
     */
    Extensible = 0x2e,  // Neo N3 value

    /**
     * @brief Consensus inventory (legacy).
     */
    Consensus = 0xE0
};
}  // namespace neo::network
