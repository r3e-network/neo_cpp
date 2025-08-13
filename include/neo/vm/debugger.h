/**
 * @file debugger.h
 * @brief Debugger
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/execution_engine.h>

#include <unordered_set>

namespace neo::vm
{
/**
 * @brief Represents a debugger for the VM.
 */
class Debugger
{
   public:
    /**
     * @brief Initializes a new instance of the Debugger class.
     * @param engine The execution engine to debug.
     */
    Debugger(ExecutionEngine& engine);

    /**
     * @brief Execute the script until completion or breakpoint.
     * @return The state of the VM after execution.
     */
    VMState Execute();

    /**
     * @brief Step into the next instruction.
     * @return The state of the VM after stepping.
     */
    VMState StepInto();

    /**
     * @brief Step over the current instruction.
     * @return The state of the VM after stepping.
     */
    VMState StepOver();

    /**
     * @brief Step out of the current context.
     * @return The state of the VM after stepping.
     */
    VMState StepOut();

    /**
     * @brief Add a breakpoint at the specified position.
     * @param position The position to add the breakpoint.
     */
    void AddBreakPoint(int position);

    /**
     * @brief Remove a breakpoint at the specified position.
     * @param position The position to remove the breakpoint.
     */
    void RemoveBreakPoint(int position);

    /**
     * @brief Clear all breakpoints.
     */
    void ClearBreakPoints();

   private:
    ExecutionEngine& engine_;
    std::unordered_set<int> breakpoints_;
    int initialContextCount_ = 0;
};
}  // namespace neo::vm
