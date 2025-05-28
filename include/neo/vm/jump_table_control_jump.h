#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
    /**
     * @brief Jump-related control flow opcode handlers for the JumpTable.
     */
    class JumpTableControlJump
    {
    public:
        // Jump operations
        static void JMP(ExecutionEngine& engine, const Instruction& instruction);
        static void JMP_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPIF(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPIF_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPIFNOT(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPIFNOT_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPEQ(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPEQ_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPNE(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPNE_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPGT(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPGT_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPGE(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPGE_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPLT(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPLT_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPLE(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPLE_L(ExecutionEngine& engine, const Instruction& instruction);
        static void CALL(ExecutionEngine& engine, const Instruction& instruction);
        static void CALL_L(ExecutionEngine& engine, const Instruction& instruction);
        static void CALLA(ExecutionEngine& engine, const Instruction& instruction);
        static void RET(ExecutionEngine& engine, const Instruction& instruction);
        static void SYSCALL(ExecutionEngine& engine, const Instruction& instruction);
        
        // Helper methods
        static void ExecuteJumpOffset(ExecutionEngine& engine, int32_t offset);
    };
}
