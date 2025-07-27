#pragma once

#include <cstdint>

namespace neo::vm
{
/**
 * @brief VM execution states.
 */
enum class VMState : uint8_t
{
    None = 0x00,
    Halt = 0x01,
    Fault = 0x02,
    Break = 0x04
};
}  // namespace neo::vm