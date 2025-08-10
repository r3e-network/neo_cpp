#pragma once

#include <cstdint>

namespace neo::vm
{
/**
 * @brief Constants for the Neo Virtual Machine
 */
class VMConstants
{
   public:
    /**
     * @brief Maximum value that can be pushed using direct PUSH opcodes (PUSH0-PUSH16)
     */
    static constexpr int32_t MAX_DIRECT_PUSH_VALUE = 16;

    /**
     * @brief Minimum value that can be pushed using direct PUSH opcodes (PUSHM1-PUSH16)
     */
    static constexpr int32_t MIN_DIRECT_PUSH_VALUE = -1;

    /**
     * @brief Base opcode value for PUSH0 (0x10)
     */
    static constexpr uint8_t PUSH0_OPCODE = 0x10;

    /**
     * @brief Maximum number of contract parameters (from C# implementation)
     */
    static constexpr int32_t MAX_CONTRACT_PARAMETERS_COUNT = 255;

    /**
     * @brief Maximum number of transaction witnesses (from C# implementation)
     */
    static constexpr int32_t MAX_TRANSACTION_WITNESSES = 16;
};
}  // namespace neo::vm