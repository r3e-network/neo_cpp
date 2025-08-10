#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Buffer-related splice opcode handlers for the JumpTable.
 */
class JumpTableSpliceBuffer
{
   public:
    // Buffer operations
    static void NEWBUFFER(ExecutionEngine& engine, const Instruction& instruction);
    static void MEMCPY(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
