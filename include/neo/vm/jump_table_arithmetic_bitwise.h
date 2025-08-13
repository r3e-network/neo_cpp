/**
 * @file jump_table_arithmetic_bitwise.h
 * @brief Jump Table Arithmetic Bitwise
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

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
