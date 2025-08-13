/**
 * @file jump_table_arithmetic_numeric.h
 * @brief Jump Table Arithmetic Numeric
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Numeric arithmetic-related opcode handlers for the JumpTable.
 */
class JumpTableArithmeticNumeric
{
   public:
    // Arithmetic operations
    static void ADD(ExecutionEngine& engine, const Instruction& instruction);
    static void SUB(ExecutionEngine& engine, const Instruction& instruction);
    static void MUL(ExecutionEngine& engine, const Instruction& instruction);
    static void DIV(ExecutionEngine& engine, const Instruction& instruction);
    static void MOD(ExecutionEngine& engine, const Instruction& instruction);
    static void POW(ExecutionEngine& engine, const Instruction& instruction);
    static void SQRT(ExecutionEngine& engine, const Instruction& instruction);
    static void SHL(ExecutionEngine& engine, const Instruction& instruction);
    static void SHR(ExecutionEngine& engine, const Instruction& instruction);
    static void NOT(ExecutionEngine& engine, const Instruction& instruction);
    static void BOOLAND(ExecutionEngine& engine, const Instruction& instruction);
    static void BOOLOR(ExecutionEngine& engine, const Instruction& instruction);
    static void NUMEQUAL(ExecutionEngine& engine, const Instruction& instruction);
    static void NUMNOTEQUAL(ExecutionEngine& engine, const Instruction& instruction);
    static void LT(ExecutionEngine& engine, const Instruction& instruction);
    static void GT(ExecutionEngine& engine, const Instruction& instruction);
    static void LE(ExecutionEngine& engine, const Instruction& instruction);
    static void GE(ExecutionEngine& engine, const Instruction& instruction);
    static void MIN(ExecutionEngine& engine, const Instruction& instruction);
    static void MAX(ExecutionEngine& engine, const Instruction& instruction);
    static void WITHIN(ExecutionEngine& engine, const Instruction& instruction);
    static void SIGN(ExecutionEngine& engine, const Instruction& instruction);
    static void ABS(ExecutionEngine& engine, const Instruction& instruction);
    static void NEGATE(ExecutionEngine& engine, const Instruction& instruction);
    static void INC(ExecutionEngine& engine, const Instruction& instruction);
    static void DEC(ExecutionEngine& engine, const Instruction& instruction);
    static void MODMUL(ExecutionEngine& engine, const Instruction& instruction);
    static void MODPOW(ExecutionEngine& engine, const Instruction& instruction);
    static void NZ(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
