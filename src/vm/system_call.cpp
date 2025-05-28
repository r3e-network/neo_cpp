#include <neo/vm/execution_engine.h>

namespace neo::vm
{
    // SystemCall implementation
    SystemCall::SystemCall(const std::string& name, std::function<bool(ExecutionEngine&)> handler)
        : name_(name), handler_(handler)
    {
    }

    const std::string& SystemCall::GetName() const
    {
        return name_;
    }

    const std::function<bool(ExecutionEngine&)>& SystemCall::GetHandler() const
    {
        return handler_;
    }
}
