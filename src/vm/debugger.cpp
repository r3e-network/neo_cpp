#include <neo/vm/debugger.h>

namespace neo::vm
{
    Debugger::Debugger(ExecutionEngine& engine)
        : engine_(engine)
    {
        initialContextCount_ = engine_.GetInvocationStack().size();
    }

    VMState Debugger::Execute()
    {
        while (engine_.GetState() == VMState::Break)
        {
            if (engine_.GetInvocationStack().size() <= initialContextCount_)
                break;

            if (engine_.GetInvocationStack().empty())
                break;

            auto& context = engine_.GetCurrentContext();
            int position = context.GetInstructionPointer();

            if (breakpoints_.find(position) != breakpoints_.end())
                break;

            engine_.Execute();
        }

        return engine_.GetState();
    }

    VMState Debugger::StepInto()
    {
        if (engine_.GetState() != VMState::Break)
            return engine_.GetState();

        if (engine_.GetInvocationStack().empty())
            return engine_.GetState();

        auto& context = engine_.GetCurrentContext();
        int currentPosition = context.GetInstructionPointer();

        // Execute one instruction
        engine_.Execute();

        // If the state is still Break and the position has changed, we're done
        if (engine_.GetState() == VMState::Break && !engine_.GetInvocationStack().empty())
        {
            auto& newContext = engine_.GetCurrentContext();
            int newPosition = newContext.GetInstructionPointer();

            if (newContext.GetScript().GetScript() != context.GetScript().GetScript() || newPosition != currentPosition)
                return engine_.GetState();
        }

        // Otherwise, continue execution
        return Execute();
    }

    VMState Debugger::StepOver()
    {
        if (engine_.GetState() != VMState::Break)
            return engine_.GetState();

        if (engine_.GetInvocationStack().empty())
            return engine_.GetState();

        auto& context = engine_.GetCurrentContext();
        int currentPosition = context.GetInstructionPointer();
        int contextCount = engine_.GetInvocationStack().size();

        // Execute one instruction
        engine_.Execute();

        // If the state is still Break, check if we need to continue
        if (engine_.GetState() == VMState::Break)
        {
            // If we're in a different context or at a different position, we're done
            if (engine_.GetInvocationStack().size() < contextCount)
                return engine_.GetState();

            if (engine_.GetInvocationStack().size() == contextCount && !engine_.GetInvocationStack().empty())
            {
                auto& newContext = engine_.GetCurrentContext();
                int newPosition = newContext.GetInstructionPointer();

                if (newContext.GetScript().GetScript() != context.GetScript().GetScript() || newPosition != currentPosition)
                    return engine_.GetState();
            }

            // If we're in a deeper context, continue until we return to the original context
            while (engine_.GetState() == VMState::Break && engine_.GetInvocationStack().size() > contextCount)
            {
                engine_.Execute();
            }
        }

        return engine_.GetState();
    }

    VMState Debugger::StepOut()
    {
        if (engine_.GetState() != VMState::Break)
            return engine_.GetState();

        if (engine_.GetInvocationStack().empty())
            return engine_.GetState();

        int contextCount = engine_.GetInvocationStack().size();

        // Continue execution until we return to a higher context
        while (engine_.GetState() == VMState::Break && engine_.GetInvocationStack().size() >= contextCount)
        {
            engine_.Execute();
        }

        return engine_.GetState();
    }

    void Debugger::AddBreakPoint(int position)
    {
        breakpoints_.insert(position);
    }

    void Debugger::RemoveBreakPoint(int position)
    {
        breakpoints_.erase(position);
    }

    void Debugger::ClearBreakPoints()
    {
        breakpoints_.clear();
    }
}
