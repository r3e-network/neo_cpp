#pragma once

#include <cstdint>

namespace neo::network
{
/**
 * @brief Represents the type of a payload.
 */
enum class PayloadType : uint8_t
{
    /**
     * @brief Version payload.
     */
    Version = 0x00,

    /**
     * @brief Verack payload.
     */
    Verack = 0x01,

    /**
     * @brief GetAddr payload.
     */
    GetAddr = 0x10,

    /**
     * @brief Addr payload.
     */
    Addr = 0x11,

    /**
     * @brief Ping payload.
     */
    Ping = 0x18,

    /**
     * @brief Pong payload.
     */
    Pong = 0x19,

    /**
     * @brief GetHeaders payload.
     */
    GetHeaders = 0x20,

    /**
     * @brief Headers payload.
     */
    Headers = 0x21,

    /**
     * @brief GetBlocks payload.
     */
    GetBlocks = 0x24,

    /**
     * @brief Mempool payload.
     */
    Mempool = 0x25,

    /**
     * @brief Inventory payload.
     */
    Inventory = 0x27,

    /**
     * @brief GetData payload.
     */
    GetData = 0x28,

    /**
     * @brief GetBlockByIndex payload.
     */
    GetBlockByIndex = 0x29,

    /**
     * @brief NotFound payload.
     */
    NotFound = 0x2a,

    /**
     * @brief Transaction payload.
     */
    Transaction = 0x2b,

    /**
     * @brief Block payload.
     */
    Block = 0x2c,

    /**
     * @brief Consensus payload.
     */
    Consensus = 0x2d,

    /**
     * @brief Extensible payload.
     */
    Extensible = 0x2e,

    /**
     * @brief Reject payload.
     */
    Reject = 0x2f,

    /**
     * @brief FilterLoad payload.
     */
    FilterLoad = 0x30,

    /**
     * @brief FilterAdd payload.
     */
    FilterAdd = 0x31,

    /**
     * @brief FilterClear payload.
     */
    FilterClear = 0x32,

    /**
     * @brief MerkleBlock payload.
     */
    MerkleBlock = 0x38,

    /**
     * @brief Alert payload.
     */
    Alert = 0x40
};
}  // namespace neo::network
