#pragma once

#include <neo/smartcontract/call_flags.h>
#include <neo/vm/execution_engine.h>
#include <string>
#include <functional>

namespace neo::smartcontract
{
    /**
     * @brief Structure to hold information about a system call.
     */
    struct SystemCallDescriptor
    {
        std::string name;
        std::function<bool(vm::ExecutionEngine&)> handler;
        int64_t gasCost;
        CallFlags requiredFlags;
    };
}
