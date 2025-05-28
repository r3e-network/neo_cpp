#pragma once

#include <cstdint>

namespace neo::network::p2p
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
     * @brief Checks if a MessageFlags value has a specific flag set.
     * @param flags The flags to check.
     * @param flag The flag to check for.
     * @return True if the flag is set, false otherwise.
     */
    inline bool HasFlag(MessageFlags flags, MessageFlags flag)
    {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0;
    }

    /**
     * @brief Sets a specific flag in a MessageFlags value.
     * @param flags The flags to modify.
     * @param flag The flag to set.
     * @return The modified flags.
     */
    inline MessageFlags SetFlag(MessageFlags flags, MessageFlags flag)
    {
        return static_cast<MessageFlags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(flag));
    }

    /**
     * @brief Clears a specific flag in a MessageFlags value.
     * @param flags The flags to modify.
     * @param flag The flag to clear.
     * @return The modified flags.
     */
    inline MessageFlags ClearFlag(MessageFlags flags, MessageFlags flag)
    {
        return static_cast<MessageFlags>(static_cast<uint8_t>(flags) & ~static_cast<uint8_t>(flag));
    }
}
