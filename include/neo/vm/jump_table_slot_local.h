#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Local variable-related slot opcode handlers for the JumpTable.
 */
class JumpTableSlotLocal
{
   public:
    // Local variable operations
    static void INITSLOT(ExecutionEngine& engine, const Instruction& instruction);
    static void LDLOC0(ExecutionEngine& engine, const Instruction& instruction);
    static void LDLOC1(ExecutionEngine& engine, const Instruction& instruction);
    static void LDLOC2(ExecutionEngine& engine, const Instruction& instruction);
    static void LDLOC3(ExecutionEngine& engine, const Instruction& instruction);
    static void LDLOC4(ExecutionEngine& engine, const Instruction& instruction);
    static void LDLOC5(ExecutionEngine& engine, const Instruction& instruction);
    static void LDLOC6(ExecutionEngine& engine, const Instruction& instruction);
    static void LDLOC(ExecutionEngine& engine, const Instruction& instruction);
    static void STLOC0(ExecutionEngine& engine, const Instruction& instruction);
    static void STLOC1(ExecutionEngine& engine, const Instruction& instruction);
    static void STLOC2(ExecutionEngine& engine, const Instruction& instruction);
    static void STLOC3(ExecutionEngine& engine, const Instruction& instruction);
    static void STLOC4(ExecutionEngine& engine, const Instruction& instruction);
    static void STLOC5(ExecutionEngine& engine, const Instruction& instruction);
    static void STLOC6(ExecutionEngine& engine, const Instruction& instruction);
    static void STLOC(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
