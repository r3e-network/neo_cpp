#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Bitwise arithmetic-related opcode handlers for the JumpTable.
 */
class JumpTableArithmeticBitwise
{
  public:
    // Bitwise operations
    static void INVERT(ExecutionEngine& engine, const Instruction& instruction);
    static void AND(ExecutionEngine& engine, const Instruction& instruction);
    static void OR(ExecutionEngine& engine, const Instruction& instruction);
    static void XOR(ExecutionEngine& engine, const Instruction& instruction);
    static void EQUAL(ExecutionEngine& engine, const Instruction& instruction);
    static void NOTEQUAL(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
