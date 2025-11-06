/**
 * @file debugger.h
 * @brief Debugger
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/execution_engine.h>

#include <cstdint>
#include <unordered_set>
#include <unordered_map>

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
     * @param script The script that owns the breakpoint.
     * @param position The position to add the breakpoint.
     */
    void AddBreakPoint(const Script& script, uint32_t position);

    /**
     * @brief Remove a breakpoint at the specified position.
     * @param script The script that owns the breakpoint.
     * @param position The position to remove the breakpoint.
     * @return True if the breakpoint was removed; otherwise false.
     */
    bool RemoveBreakPoint(const Script& script, uint32_t position);

    /**
     * @brief Clear all breakpoints.
     */
    void ClearBreakPoints();

   private:
    ExecutionEngine& engine_;
    using BreakpointSet = std::unordered_set<uint32_t>;
    std::unordered_map<const Script*, BreakpointSet> breakpoints_;
    int initialContextCount_ = 0;

    void ExecuteAndCheckBreakPoints();
    bool ShouldBreakOnCurrentInstruction() const;
};
}  // namespace neo::vm
