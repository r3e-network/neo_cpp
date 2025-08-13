/**
 * @file jump_table_compound_map.h
 * @brief Jump Table Compound Map
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Map-related compound opcode handlers for the JumpTable.
 */
class JumpTableCompoundMap
{
   public:
    // Map operations
    static void NEWMAP(ExecutionEngine& engine, const Instruction& instruction);
    static void PACKMAP(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
