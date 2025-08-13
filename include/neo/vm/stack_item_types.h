/**
 * @file stack_item_types.h
 * @brief Type definitions
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>

namespace neo::vm
{
/**
 * @brief Enum for stack item type.
 */
enum class StackItemType : uint8_t
{
    Any = 0x00,
    Pointer = 0x10,
    Boolean = 0x20,
    Integer = 0x21,
    ByteString = 0x28,
    Buffer = 0x30,
    Array = 0x40,
    Struct = 0x41,
    Map = 0x48,
    InteropInterface = 0x60,
    Null = 0x70
};

/**
 * @brief Checks if a stack item type is valid.
 * @param type The type to check.
 * @return True if the type is valid, false otherwise.
 */
inline bool IsValidStackItemType(StackItemType type)
{
    switch (type)
    {
        case StackItemType::Any:
        case StackItemType::Pointer:
        case StackItemType::Boolean:
        case StackItemType::Integer:
        case StackItemType::ByteString:
        case StackItemType::Buffer:
        case StackItemType::Array:
        case StackItemType::Struct:
        case StackItemType::Map:
        case StackItemType::InteropInterface:
        case StackItemType::Null:
            return true;
        default:
            return false;
    }
}
}  // namespace neo::vm
