/**
 * @file node_capability_types.h
 * @brief Type definitions
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace neo::network::p2p
{
/**
 * @brief Enumeration of node capability types
 *
 * These capabilities indicate what services a Neo node provides
 * to the network.
 */
enum class NodeCapabilityType : uint8_t
{
    // Servers
    /**
     * @brief TCP server capability - node accepts TCP connections
     */
    TcpServer = 0x01,

    /**
     * @brief WebSocket server capability - node accepts WS connections
     * @deprecated This capability is obsolete in Neo N3
     */
    WsServer = 0x02,

    /**
     * @brief Disable compression capability - node doesn't support compression
     */
    DisableCompression = 0x03,

    // Data availability
    /**
     * @brief Full node capability - node has complete current state
     */
    FullNode = 0x10,

    /**
     * @brief Archival node capability - stores full block history
     * These nodes can be used for P2P synchronization from genesis
     */
    ArchivalNode = 0x11,

    // Private extensions
    /**
     * @brief The first extension ID. Any subsequent can be used in an
     * implementation-specific way.
     */
    Extension0 = 0xf0,

    /**
     * @brief Unknown capability type - for unrecognized capabilities
     */
    Unknown = 0xFF
};

/**
 * @brief Convert NodeCapabilityType to string representation
 * @param capability The capability to convert
 * @return String representation of the capability
 */
std::string NodeCapabilityTypeToString(NodeCapabilityType capability);

/**
 * @brief Parse string to NodeCapabilityType
 * @param str The string to parse
 * @return The corresponding NodeCapabilityType
 * @throws std::invalid_argument if string is not a valid capability
 */
NodeCapabilityType StringToNodeCapabilityType(const std::string& str);

/**
 * @brief Check if a capability is set in a capabilities vector
 * @param capabilities Vector of capabilities to check
 * @param capability The capability to look for
 * @return True if the capability is present
 */
bool HasCapability(const std::vector<NodeCapabilityType>& capabilities, NodeCapabilityType capability);

/**
 * @brief Combine multiple capabilities into a bitmask
 * @param capabilities Vector of capabilities
 * @return Combined bitmask value
 */
uint32_t CapabilitiesToBitmask(const std::vector<NodeCapabilityType>& capabilities);

/**
 * @brief Extract capabilities from a bitmask
 * @param bitmask The bitmask to parse
 * @return Vector of capabilities
 */
std::vector<NodeCapabilityType> BitmaskToCapabilities(uint32_t bitmask);
}  // namespace neo::network::p2p