/**
 * @file debugger.cpp
 * @brief Debugger
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/debugger.h>

namespace neo::vm
{
Debugger::Debugger(ExecutionEngine& engine) : engine_(engine)
{
    initialContextCount_ = static_cast<int>(engine_.GetInvocationStack().size());
    // Initialize the engine to Break state for debugging
    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }
}

VMState Debugger::Execute()
{
    // Use the standard Execute method to run until completion
    engine_.SetState(VMState::None);
    return engine_.Execute();
}

VMState Debugger::StepInto()
{
    // If not in Break state, initialize to Break for debugging
    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    if (engine_.GetState() != VMState::Break) return engine_.GetState();

    if (engine_.GetInvocationStack().empty())
    {
        engine_.SetState(VMState::Halt);
        return engine_.GetState();
    }

    // Execute one instruction
    engine_.ExecuteNext();

    // Return to Break state for debugging unless we've halted or faulted
    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    return engine_.GetState();
}

VMState Debugger::StepOver()
{
    // If not in Break state, initialize to Break for debugging
    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    if (engine_.GetState() != VMState::Break) return engine_.GetState();

    if (engine_.GetInvocationStack().empty())
    {
        engine_.SetState(VMState::Halt);
        return engine_.GetState();
    }

    int contextCount = static_cast<int>(engine_.GetInvocationStack().size());

    // Execute one instruction
    engine_.ExecuteNext();

    // If we're in a deeper context, continue until we return to the original context
    while (engine_.GetState() != VMState::Halt && engine_.GetState() != VMState::Fault &&
           static_cast<int>(engine_.GetInvocationStack().size()) > contextCount)
    {
        engine_.ExecuteNext();
    }

    // Return to Break state for debugging unless we've halted or faulted
    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    return engine_.GetState();
}

VMState Debugger::StepOut()
{
    // If not in Break state, initialize to Break for debugging
    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    if (engine_.GetState() != VMState::Break) return engine_.GetState();

    if (engine_.GetInvocationStack().empty())
    {
        engine_.SetState(VMState::Halt);
        return engine_.GetState();
    }

    int contextCount = static_cast<int>(engine_.GetInvocationStack().size());

    // Continue execution until we return to a higher context
    while (engine_.GetState() != VMState::Halt && engine_.GetState() != VMState::Fault &&
           static_cast<int>(engine_.GetInvocationStack().size()) >= contextCount)
    {
        engine_.ExecuteNext();
    }

    // Return to Break state for debugging unless we've halted or faulted
    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    return engine_.GetState();
}

void Debugger::AddBreakPoint(int position) { breakpoints_.insert(position); }

void Debugger::RemoveBreakPoint(int position) { breakpoints_.erase(position); }

void Debugger::ClearBreakPoints() { breakpoints_.clear(); }
}  // namespace neo::vm
