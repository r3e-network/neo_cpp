#pragma once

#include <cstdint>

namespace neo::network::p2p
{
/**
 * @brief Represents the type of an inventory item.
 */
enum class InventoryType : uint8_t
{
    // Primary names (match C# Neo.Network.P2P)
    Transaction = 0x01,
    Block = 0x02,
    Consensus = 0xe0,
    Extensible = 0xf0,

    // Backward-compatible aliases used elsewhere in this C++ port
    TX = Transaction
};
}  // namespace neo::network::p2p
