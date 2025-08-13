/**
 * @file jump_table_splice_string.h
 * @brief Jump Table Splice String
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief String-related splice opcode handlers for the JumpTable.
 */
class JumpTableSpliceString
{
   public:
    // String operations
    static void CAT(ExecutionEngine& engine, const Instruction& instruction);
    static void SUBSTR(ExecutionEngine& engine, const Instruction& instruction);
    static void LEFT(ExecutionEngine& engine, const Instruction& instruction);
    static void RIGHT(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
