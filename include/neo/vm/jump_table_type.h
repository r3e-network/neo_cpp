/**
 * @file jump_table_type.h
 * @brief Jump Table Type
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/instruction.h>
#include <neo/vm/opcode.h>

#include <functional>
#include <memory>

// Forward declarations
namespace neo::vm
{
class ExecutionEngine;
class StackItem;
}  // namespace neo::vm

namespace neo::vm
{
/**
 * @brief Type operations for the JumpTable.
 */
class JumpTableType
{
   public:
    /**
     * @brief Checks if the top item on the stack is null.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void ISNULL(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Checks if the top item on the stack is of the specified type.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void ISTYPE(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Converts the top item on the stack to the specified type.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void CONVERT(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
