/**
 * @file system_call_descriptor.h
 * @brief System Call Descriptor
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/smartcontract/call_flags.h>
#include <neo/vm/execution_engine.h>

#include <functional>
#include <string>

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
}  // namespace neo::smartcontract
