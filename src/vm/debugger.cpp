/**
 * @file debugger.cpp
 * @brief Debugger implementation mirroring Neo C# semantics.
 */

#include <neo/vm/debugger.h>

#include <neo/vm/execution_context.h>
#include <neo/vm/script.h>

#include <string>

namespace neo::vm
{
Debugger::Debugger(ExecutionEngine& engine) : engine_(engine)
{
    initialContextCount_ = static_cast<int>(engine_.GetInvocationStack().size());
    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }
}

VMState Debugger::Execute()
{
    if (engine_.GetState() == VMState::Break)
    {
        engine_.SetState(VMState::None);
    }

    while (engine_.GetState() == VMState::None)
    {
        ExecuteAndCheckBreakPoints();
    }

    return engine_.GetState();
}

VMState Debugger::StepInto()
{
    if (engine_.GetState() == VMState::Halt || engine_.GetState() == VMState::Fault) return engine_.GetState();

    ExecuteAndCheckBreakPoints();

    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    return engine_.GetState();
}

VMState Debugger::StepOver()
{
    if (engine_.GetState() == VMState::Halt || engine_.GetState() == VMState::Fault) return engine_.GetState();

    engine_.SetState(VMState::None);
    const auto initialDepth = engine_.GetInvocationStack().size();

    do
    {
        ExecuteAndCheckBreakPoints();
    } while (engine_.GetState() == VMState::None && engine_.GetInvocationStack().size() > initialDepth);

    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    return engine_.GetState();
}

VMState Debugger::StepOut()
{
    if (engine_.GetState() == VMState::Halt || engine_.GetState() == VMState::Fault) return engine_.GetState();

    if (engine_.GetState() == VMState::Break)
    {
        engine_.SetState(VMState::None);
    }

    const auto targetDepth = engine_.GetInvocationStack().empty() ? 0 : engine_.GetInvocationStack().size() - 1;

    while (engine_.GetState() == VMState::None && engine_.GetInvocationStack().size() > targetDepth)
    {
        ExecuteAndCheckBreakPoints();
    }

    if (engine_.GetState() == VMState::None)
    {
        engine_.SetState(VMState::Break);
    }

    return engine_.GetState();
}

void Debugger::AddBreakPoint(const Script& script, uint32_t position)
{
    breakpoints_[MakeScriptKey(script)].insert(position);
}

bool Debugger::RemoveBreakPoint(const Script& script, uint32_t position)
{
    auto key = MakeScriptKey(script);
    auto it = breakpoints_.find(key);
    if (it == breakpoints_.end()) return false;

    const auto removed = it->second.erase(position);
    if (it->second.empty()) breakpoints_.erase(it);
    return removed > 0;
}

void Debugger::ClearBreakPoints() { breakpoints_.clear(); }

void Debugger::ExecuteAndCheckBreakPoints()
{
    engine_.ExecuteNext();

    if (engine_.GetState() != VMState::None) return;
    if (engine_.GetInvocationStack().empty()) return;

    if (ShouldBreakOnCurrentInstruction())
    {
        engine_.SetState(VMState::Break);
    }
}

bool Debugger::ShouldBreakOnCurrentInstruction() const
{
    const auto& invocationStack = engine_.GetInvocationStack();
    if (invocationStack.empty()) return false;

    const ExecutionContext& context = *invocationStack.back();
    auto key = MakeScriptKey(context.GetScript());
    auto it = breakpoints_.find(key);
    if (it == breakpoints_.end()) return false;

    const auto position = static_cast<uint32_t>(context.GetInstructionPointer());
    return it->second.find(position) != it->second.end();
}

std::string Debugger::MakeScriptKey(const Script& script)
{
    const auto& data = script.GetScript();
    return std::string(reinterpret_cast<const char*>(data.Data()), data.Size());
}
}  // namespace neo::vm
