#pragma once

#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_slot_argument.h>
#include <neo/vm/jump_table_slot_local.h>
#include <neo/vm/jump_table_slot_static.h>

namespace neo::vm
{
/**
 * @brief Slot-related opcode handlers for the JumpTable.
 *
 * This class is a facade that delegates to the specialized slot operation classes:
 * - JumpTableSlotStatic: Static field operations
 * - JumpTableSlotLocal: Local variable operations
 * - JumpTableSlotArgument: Argument operations
 */
class JumpTableSlot : public JumpTableSlotStatic, public JumpTableSlotLocal, public JumpTableSlotArgument
{
};
}  // namespace neo::vm
