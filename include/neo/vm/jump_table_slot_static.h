#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Static field-related slot opcode handlers for the JumpTable.
 */
class JumpTableSlotStatic
{
  public:
    // Static field operations
    static void INITSSLOT(ExecutionEngine& engine, const Instruction& instruction);
    static void LDSFLD0(ExecutionEngine& engine, const Instruction& instruction);
    static void LDSFLD1(ExecutionEngine& engine, const Instruction& instruction);
    static void LDSFLD2(ExecutionEngine& engine, const Instruction& instruction);
    static void LDSFLD3(ExecutionEngine& engine, const Instruction& instruction);
    static void LDSFLD4(ExecutionEngine& engine, const Instruction& instruction);
    static void LDSFLD5(ExecutionEngine& engine, const Instruction& instruction);
    static void LDSFLD6(ExecutionEngine& engine, const Instruction& instruction);
    static void LDSFLD(ExecutionEngine& engine, const Instruction& instruction);
    static void STSFLD0(ExecutionEngine& engine, const Instruction& instruction);
    static void STSFLD1(ExecutionEngine& engine, const Instruction& instruction);
    static void STSFLD2(ExecutionEngine& engine, const Instruction& instruction);
    static void STSFLD3(ExecutionEngine& engine, const Instruction& instruction);
    static void STSFLD4(ExecutionEngine& engine, const Instruction& instruction);
    static void STSFLD5(ExecutionEngine& engine, const Instruction& instruction);
    static void STSFLD6(ExecutionEngine& engine, const Instruction& instruction);
    static void STSFLD(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
