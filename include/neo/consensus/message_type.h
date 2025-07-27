#pragma once

#include <cstdint>

namespace neo::consensus
{
/**
 * @brief Represents the type of a consensus message.
 */
enum class MessageType : uint8_t
{
    /**
     * @brief Change view message.
     */
    ChangeView = 0x00,

    /**
     * @brief Prepare request message.
     */
    PrepareRequest = 0x20,

    /**
     * @brief Prepare response message.
     */
    PrepareResponse = 0x21,

    /**
     * @brief Commit message.
     */
    Commit = 0x30,

    /**
     * @brief Recovery message.
     */
    RecoveryMessage = 0x40,

    /**
     * @brief Recovery request message.
     */
    RecoveryRequest = 0x41
};
}  // namespace neo::consensus
