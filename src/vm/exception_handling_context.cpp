#include <neo/vm/exception_handling_context.h>

namespace neo::vm
{
ExceptionHandlingContext::ExceptionHandlingContext(int32_t catchPointer, int32_t finallyPointer)
    : catchPointer_(catchPointer), finallyPointer_(finallyPointer), endPointer_(-1), state_(ExceptionHandlingState::Try)
{
}

int32_t ExceptionHandlingContext::GetCatchPointer() const
{
    return catchPointer_;
}

int32_t ExceptionHandlingContext::GetFinallyPointer() const
{
    return finallyPointer_;
}

int32_t ExceptionHandlingContext::GetEndPointer() const
{
    return endPointer_;
}

void ExceptionHandlingContext::SetEndPointer(int32_t endPointer)
{
    endPointer_ = endPointer;
}

ExceptionHandlingState ExceptionHandlingContext::GetState() const
{
    return state_;
}

void ExceptionHandlingContext::SetState(ExceptionHandlingState state)
{
    state_ = state;
}

bool ExceptionHandlingContext::HasCatch() const
{
    return catchPointer_ >= 0;
}

bool ExceptionHandlingContext::HasFinally() const
{
    return finallyPointer_ >= 0;
}
}  // namespace neo::vm
