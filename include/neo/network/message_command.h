#pragma once

#include <cstdint>

namespace neo::network
{
    /**
     * @brief Represents the command of a message.
     */
    enum class MessageCommand : uint8_t
    {
        // Handshaking
        
        /**
         * @brief Sent when a connection is established.
         */
        Version = 0x00,
        
        /**
         * @brief Sent to respond to Version messages.
         */
        Verack = 0x01,
        
        // Connectivity
        
        /**
         * @brief Sent to request for remote nodes.
         */
        GetAddr = 0x10,
        
        /**
         * @brief Sent to respond to GetAddr messages.
         */
        Addr = 0x11,
        
        /**
         * @brief Sent to detect whether the connection has been disconnected.
         */
        Ping = 0x18,
        
        /**
         * @brief Sent to respond to Ping messages.
         */
        Pong = 0x19,
        
        // Synchronization
        
        /**
         * @brief Sent to request for headers.
         */
        GetHeaders = 0x20,
        
        /**
         * @brief Sent to respond to GetHeaders messages.
         */
        Headers = 0x21,
        
        /**
         * @brief Sent to request for blocks.
         */
        GetBlocks = 0x24,
        
        /**
         * @brief Sent to request for memory pool.
         */
        Mempool = 0x25,
        
        /**
         * @brief Sent to relay inventories.
         */
        Inv = 0x27,
        
        /**
         * @brief Sent to request for inventories.
         */
        GetData = 0x28,
        
        /**
         * @brief Sent to request for blocks.
         */
        GetBlockByIndex = 0x29,
        
        /**
         * @brief Sent to respond to GetData messages when the inventories are not found.
         */
        NotFound = 0x2A,
        
        /**
         * @brief Sent to send a transaction.
         */
        Transaction = 0x2B,
        
        /**
         * @brief Sent to send a block.
         */
        Block = 0x2C,
        
        /**
         * @brief Sent to send a consensus message.
         */
        Consensus = 0x2D,
        
        /**
         * @brief Sent to send a reject message.
         */
        Reject = 0x2F,
        
        // SPV Protocol
        
        /**
         * @brief Sent to set the bloom filter.
         */
        FilterLoad = 0x30,
        
        /**
         * @brief Sent to update the items for the bloom filter.
         */
        FilterAdd = 0x31,
        
        /**
         * @brief Sent to clear the bloom filter.
         */
        FilterClear = 0x32,
        
        /**
         * @brief Sent to send a filtered block.
         */
        MerkleBlock = 0x38,
        
        // Others
        
        /**
         * @brief Sent to send an alert.
         */
        Alert = 0x40,
        
        /**
         * @brief Sent to send an extensible message.
         */
        Extensible = 0xFF
    };
}
