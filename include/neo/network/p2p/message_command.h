#pragma once

#include <cstdint>
#include <string>

namespace neo::network::p2p
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
        NotFound = 0x2a,

        /**
         * @brief Sent to send a transaction.
         */
        Transaction = 0x2b,

        /**
         * @brief Sent to send a block.
         */
        Block = 0x2c,

        /**
         * @brief Sent to send a consensus message.
         */
        Consensus = 0x2d,

        /**
         * @brief Sent to send an extensible payload.
         */
        Extensible = 0xff,

        /**
         * @brief Sent to reject an inventory.
         */
        Reject = 0x2f,

        /**
         * @brief Unknown command.
         */
        Unknown = 0xfe,

        // SPV protocol

        /**
         * @brief Sent to load the bloom filter.
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
        Alert = 0x40
    };

    /**
     * @brief Gets the name of a message command.
     * @param command The message command.
     * @return The name of the message command.
     */
    inline std::string GetCommandName(MessageCommand command)
    {
        switch (command)
        {
            case MessageCommand::Version:
                return "version";
            case MessageCommand::Verack:
                return "verack";
            case MessageCommand::GetAddr:
                return "getaddr";
            case MessageCommand::Addr:
                return "addr";
            case MessageCommand::Ping:
                return "ping";
            case MessageCommand::Pong:
                return "pong";
            case MessageCommand::GetHeaders:
                return "getheaders";
            case MessageCommand::Headers:
                return "headers";
            case MessageCommand::GetBlocks:
                return "getblocks";
            case MessageCommand::Mempool:
                return "mempool";
            case MessageCommand::Inv:
                return "inv";
            case MessageCommand::GetData:
                return "getdata";
            case MessageCommand::GetBlockByIndex:
                return "getblockbyindex";
            case MessageCommand::NotFound:
                return "notfound";
            case MessageCommand::Transaction:
                return "tx";
            case MessageCommand::Block:
                return "block";
            case MessageCommand::Consensus:
                return "consensus";
            case MessageCommand::Reject:
                return "reject";
            case MessageCommand::FilterLoad:
                return "filterload";
            case MessageCommand::FilterAdd:
                return "filteradd";
            case MessageCommand::FilterClear:
                return "filterclear";
            case MessageCommand::MerkleBlock:
                return "merkleblock";
            case MessageCommand::Alert:
                return "alert";
            case MessageCommand::Extensible:
                return "extensible";
            default:
                return "unknown";
        }
    }
}
