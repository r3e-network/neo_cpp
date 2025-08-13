/**
 * @file node_type.h
 * @brief Node Type
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>

namespace neo::cryptography::mpttrie
{
/**
 * @brief Node types in the Merkle Patricia Trie.
 */
enum class NodeType : uint8_t
{
    BranchNode = 0x00,
    ExtensionNode = 0x01,
    LeafNode = 0x02,
    HashNode = 0x03,
    Empty = 0x04
};
}  // namespace neo::cryptography::mpttrie
