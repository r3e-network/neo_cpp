/**
 * @file system_call.cpp
 * @brief System Call
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/execution_engine.h>

namespace neo::vm
{
// SystemCall implementation
SystemCall::SystemCall(const std::string& name, std::function<bool(ExecutionEngine&)> handler)
    : name_(name), handler_(handler)
{
}

const std::string& SystemCall::GetName() const { return name_; }

const std::function<bool(ExecutionEngine&)>& SystemCall::GetHandler() const { return handler_; }
}  // namespace neo::vm
