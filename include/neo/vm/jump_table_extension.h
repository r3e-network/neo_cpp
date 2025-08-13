/**
 * @file jump_table_extension.h
 * @brief Jump Table Extension
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Extension-related opcode handlers for the JumpTable.
 */
class JumpTableExtension
{
   public:
    // Extension operations
    static void ABORTMSG(ExecutionEngine& engine, const Instruction& instruction);
    static void ASSERTMSG(ExecutionEngine& engine, const Instruction& instruction);
    static void PUSHT(ExecutionEngine& engine, const Instruction& instruction);
    static void PUSHF(ExecutionEngine& engine, const Instruction& instruction);
    static void CALLT(ExecutionEngine& engine, const Instruction& instruction);

    // Type checking and conversion operations
    static void ISNULL(ExecutionEngine& engine, const Instruction& instruction);
    static void ISTYPE(ExecutionEngine& engine, const Instruction& instruction);
    static void CONVERT(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
