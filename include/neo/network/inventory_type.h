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
        TX = 0x01,
        
        /**
         * @brief Block inventory.
         */
        Block = 0x02,
        
        /**
         * @brief Consensus inventory.
         */
        Consensus = 0xE0
    };
}
