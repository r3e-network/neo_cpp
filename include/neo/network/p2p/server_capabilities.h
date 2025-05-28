#pragma once

#include <cstdint>

namespace neo::network::p2p
{
    /**
     * @brief Represents the capabilities of a server.
     */
    enum class ServerCapability : uint32_t
    {
        /**
         * @brief No capabilities.
         */
        None = 0,
        
        /**
         * @brief The server can be used as a full node.
         */
        FullNode = 1,
        
        /**
         * @brief The server can be used as a state service.
         */
        StateService = 0b10,
        
        /**
         * @brief The server can be used as a transaction service.
         */
        TransactionService = 0b100
    };
    
    /**
     * @brief Checks if a ServerCapability value has a specific capability.
     * @param capabilities The capabilities to check.
     * @param capability The capability to check for.
     * @return True if the capability is set, false otherwise.
     */
    inline bool HasCapability(ServerCapability capabilities, ServerCapability capability)
    {
        return (static_cast<uint32_t>(capabilities) & static_cast<uint32_t>(capability)) != 0;
    }
    
    /**
     * @brief Sets a specific capability in a ServerCapability value.
     * @param capabilities The capabilities to modify.
     * @param capability The capability to set.
     * @return The modified capabilities.
     */
    inline ServerCapability SetCapability(ServerCapability capabilities, ServerCapability capability)
    {
        return static_cast<ServerCapability>(static_cast<uint32_t>(capabilities) | static_cast<uint32_t>(capability));
    }
    
    /**
     * @brief Clears a specific capability in a ServerCapability value.
     * @param capabilities The capabilities to modify.
     * @param capability The capability to clear.
     * @return The modified capabilities.
     */
    inline ServerCapability ClearCapability(ServerCapability capabilities, ServerCapability capability)
    {
        return static_cast<ServerCapability>(static_cast<uint32_t>(capabilities) & ~static_cast<uint32_t>(capability));
    }
}
