/**
 * @file vm_state.h
 * @brief Virtual machine implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

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
    NONE = None,
    Halt = 0x01,
    HALT = Halt,
    Fault = 0x02,
    FAULT = Fault,
    Break = 0x04,
    BREAK = Break
};
}  // namespace neo::vm
