#pragma once

#include <functional>
#include <memory>
#include <neo/vm/instruction.h>
#include <neo/vm/opcode.h>
#include <string>

// Forward declarations
namespace neo::vm
{
class ExecutionEngine;
class StackItem;
}  // namespace neo::vm

namespace neo::vm
{
/**
 * @brief Exception handling operations for the JumpTable.
 */
class JumpTableException
{
  public:
    /**
     * @brief Executes a throw operation.
     * @param engine The execution engine.
     * @param message The exception message.
     */
    static void ExecuteThrow(ExecutionEngine& engine, const std::string& message);

    /**
     * @brief Executes a throw operation.
     * @param engine The execution engine.
     * @param exception The exception to throw.
     */
    static void ExecuteThrow(ExecutionEngine& engine, std::shared_ptr<StackItem> exception);
};
}  // namespace neo::vm
