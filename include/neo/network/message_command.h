#pragma once

#include <string>
#include <cstdint>

namespace neo::network
{
    /**
     * @brief Enumeration of P2P message commands
     * 
     * These commands define the types of messages that can be sent
     * between Neo nodes in the P2P network.
     * 
     * Values match Neo N3 protocol specification.
     */
    enum class MessageCommand : uint8_t
    {
        // Handshaking
        /**
         * @brief Version message - initial handshake
         */
        Version = 0x00,

        /**
         * @brief Version acknowledgment
         */
        Verack = 0x01,

        // Connectivity
        /**
         * @brief Request for peer addresses
         */
        GetAddr = 0x10,

        /**
         * @brief Response with peer addresses
         */
        Addr = 0x11,

        /**
         * @brief Ping message for keepalive
         */
        Ping = 0x18,

        /**
         * @brief Pong response to ping
         */
        Pong = 0x19,

        // Synchronization
        /**
         * @brief Get block headers request
         */
        GetHeaders = 0x20,

        /**
         * @brief Block headers response
         */
        Headers = 0x21,

        /**
         * @brief Get blocks request
         */
        GetBlocks = 0x24,

        /**
         * @brief Request for memory pool
         */
        Mempool = 0x25,

        /**
         * @brief Inventory message (blocks/transactions)
         */
        Inv = 0x27,

        /**
         * @brief Get data request
         */
        GetData = 0x28,

        /**
         * @brief Get block by index request
         */
        GetBlockByIndex = 0x29,

        /**
         * @brief Not found response
         */
        NotFound = 0x2a,

        /**
         * @brief Transaction data
         */
        Transaction = 0x2b,

        /**
         * @brief Block data
         */
        Block = 0x2c,

        /**
         * @brief Extensible payload (consensus, etc.)
         */
        Extensible = 0x2e,

        /**
         * @brief Reject message
         */
        Reject = 0x2f,

        // SPV Protocol
        /**
         * @brief Filter load message
         */
        FilterLoad = 0x30,

        /**
         * @brief Filter add message
         */
        FilterAdd = 0x31,

        /**
         * @brief Filter clear message
         */
        FilterClear = 0x32,

        /**
         * @brief Merkle block message
         */
        MerkleBlock = 0x38,

        // Others
        /**
         * @brief Alert message
         */
        Alert = 0x40
    };

    /**
     * @brief Convert MessageCommand to string representation
     * @param command The command to convert
     * @return String representation of the command
     */
    std::string MessageCommandToString(MessageCommand command);

    /**
     * @brief Parse string to MessageCommand
     * @param str The string to parse
     * @return The corresponding MessageCommand
     * @throws std::invalid_argument if string is not a valid command
     */
    MessageCommand StringToMessageCommand(const std::string& str);

    /**
     * @brief Get the command name as a fixed-length string (12 bytes)
     * @param command The command to get name for
     * @return Fixed-length command name for network protocol
     */
    std::string GetCommandName(MessageCommand command);
}