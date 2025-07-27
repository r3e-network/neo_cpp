#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Argument-related slot opcode handlers for the JumpTable.
 */
class JumpTableSlotArgument
{
  public:
    // Argument operations
    static void LDARG0(ExecutionEngine& engine, const Instruction& instruction);
    static void LDARG1(ExecutionEngine& engine, const Instruction& instruction);
    static void LDARG2(ExecutionEngine& engine, const Instruction& instruction);
    static void LDARG3(ExecutionEngine& engine, const Instruction& instruction);
    static void LDARG4(ExecutionEngine& engine, const Instruction& instruction);
    static void LDARG5(ExecutionEngine& engine, const Instruction& instruction);
    static void LDARG6(ExecutionEngine& engine, const Instruction& instruction);
    static void LDARG(ExecutionEngine& engine, const Instruction& instruction);
    static void STARG0(ExecutionEngine& engine, const Instruction& instruction);
    static void STARG1(ExecutionEngine& engine, const Instruction& instruction);
    static void STARG2(ExecutionEngine& engine, const Instruction& instruction);
    static void STARG3(ExecutionEngine& engine, const Instruction& instruction);
    static void STARG4(ExecutionEngine& engine, const Instruction& instruction);
    static void STARG5(ExecutionEngine& engine, const Instruction& instruction);
    static void STARG6(ExecutionEngine& engine, const Instruction& instruction);
    static void STARG(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
