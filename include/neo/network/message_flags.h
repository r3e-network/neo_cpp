/**
 * @file message_flags.h
 * @brief Network message handling
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>

namespace neo::network
{
/**
 * @brief Represents the flags of a message.
 */
enum class MessageFlags : uint8_t
{
    /**
     * @brief No flags.
     */
    None = 0x00,

    /**
     * @brief The message is compressed.
     */
    Compressed = 0x01
};

/**
 * @brief Checks if a flags value has a specific flag.
 * @param flags The flags value.
 * @param flag The flag to check.
 * @return True if the flags value has the flag, false otherwise.
 */
inline bool HasFlag(MessageFlags flags, MessageFlags flag)
{
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0;
}

/**
 * @brief Sets a flag in a flags value.
 * @param flags The flags value.
 * @param flag The flag to set.
 * @return The new flags value.
 */
inline MessageFlags SetFlag(MessageFlags flags, MessageFlags flag)
{
    return static_cast<MessageFlags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(flag));
}

/**
 * @brief Clears a flag in a flags value.
 * @param flags The flags value.
 * @param flag The flag to clear.
 * @return The new flags value.
 */
inline MessageFlags ClearFlag(MessageFlags flags, MessageFlags flag)
{
    return static_cast<MessageFlags>(static_cast<uint8_t>(flags) & ~static_cast<uint8_t>(flag));
}
}  // namespace neo::network
