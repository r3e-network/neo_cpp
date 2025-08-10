#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Stack-related opcode handlers for the JumpTable.
 */
class JumpTableStack
{
   public:
    // Stack operations
    static void DEPTH(ExecutionEngine& engine, const Instruction& instruction);
    static void DROP(ExecutionEngine& engine, const Instruction& instruction);
    static void NIP(ExecutionEngine& engine, const Instruction& instruction);
    static void XDROP(ExecutionEngine& engine, const Instruction& instruction);
    static void CLEAR(ExecutionEngine& engine, const Instruction& instruction);
    static void DUP(ExecutionEngine& engine, const Instruction& instruction);
    static void OVER(ExecutionEngine& engine, const Instruction& instruction);
    static void PICK(ExecutionEngine& engine, const Instruction& instruction);
    static void TUCK(ExecutionEngine& engine, const Instruction& instruction);
    static void SWAP(ExecutionEngine& engine, const Instruction& instruction);
    static void ROT(ExecutionEngine& engine, const Instruction& instruction);
    static void ROLL(ExecutionEngine& engine, const Instruction& instruction);
    static void REVERSE3(ExecutionEngine& engine, const Instruction& instruction);
    static void REVERSE4(ExecutionEngine& engine, const Instruction& instruction);
    static void REVERSEN(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
