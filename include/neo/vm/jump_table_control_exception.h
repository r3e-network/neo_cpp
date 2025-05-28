#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
    /**
     * @brief Exception handling-related control flow opcode handlers for the JumpTable.
     */
    class JumpTableControlException
    {
    public:
        // Exception handling operations
        static void LEAVE(ExecutionEngine& engine, const Instruction& instruction);
        static void LEAVE_L(ExecutionEngine& engine, const Instruction& instruction);
        static void ABORT(ExecutionEngine& engine, const Instruction& instruction);
        static void ASSERT(ExecutionEngine& engine, const Instruction& instruction);
        static void THROW(ExecutionEngine& engine, const Instruction& instruction);
        static void TRY(ExecutionEngine& engine, const Instruction& instruction);
        static void TRY_L(ExecutionEngine& engine, const Instruction& instruction);
        static void ENDTRY(ExecutionEngine& engine, const Instruction& instruction);
        static void ENDTRY_L(ExecutionEngine& engine, const Instruction& instruction);
        static void ENDFINALLY(ExecutionEngine& engine, const Instruction& instruction);
    };
}
