#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
    /**
     * @brief Compound type-related opcode handlers for the JumpTable.
     */
    class JumpTableCompound
    {
    public:
        // Compound type operations
        static void PACK(ExecutionEngine& engine, const Instruction& instruction);
        static void UNPACK(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWARRAY0(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWARRAY(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWARRAY_T(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWSTRUCT0(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWSTRUCT(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWMAP(ExecutionEngine& engine, const Instruction& instruction);
        static void SIZE(ExecutionEngine& engine, const Instruction& instruction);
        static void HASKEY(ExecutionEngine& engine, const Instruction& instruction);
        static void KEYS(ExecutionEngine& engine, const Instruction& instruction);
        static void VALUES(ExecutionEngine& engine, const Instruction& instruction);
        static void PICKITEM(ExecutionEngine& engine, const Instruction& instruction);
        static void APPEND(ExecutionEngine& engine, const Instruction& instruction);
        static void SETITEM(ExecutionEngine& engine, const Instruction& instruction);
        static void REMOVE(ExecutionEngine& engine, const Instruction& instruction);
        static void CLEARITEMS(ExecutionEngine& engine, const Instruction& instruction);
        static void REVERSEITEMS(ExecutionEngine& engine, const Instruction& instruction);
        static void POPITEM(ExecutionEngine& engine, const Instruction& instruction);
        static void PACKMAP(ExecutionEngine& engine, const Instruction& instruction);
        static void PACKSTRUCT(ExecutionEngine& engine, const Instruction& instruction);
        static void ISNULL(ExecutionEngine& engine, const Instruction& instruction);
        static void ISTYPE(ExecutionEngine& engine, const Instruction& instruction);
        static void CONVERT(ExecutionEngine& engine, const Instruction& instruction);
    };
}
