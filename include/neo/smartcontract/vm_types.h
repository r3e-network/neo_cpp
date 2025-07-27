#pragma once

#include <memory>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/vm/vm_state.h>
#include <string>
#include <vector>

// Forward declarations
namespace neo::vm
{
class StackItem;
}

namespace neo::smartcontract
{
// Use the unified VMState from the vm namespace
using VMState = neo::vm::VMState;

/**
 * @brief Represents a log entry from smart contract execution.
 */
struct LogEntry
{
    io::UInt160 script_hash;
    std::string message;
    uint64_t timestamp;

    LogEntry() = default;
    LogEntry(const io::UInt160& hash, const std::string& msg, uint64_t time = 0)
        : script_hash(hash), message(msg), timestamp(time)
    {
    }
};

/**
 * @brief Represents a notification from smart contract execution.
 */
struct NotifyEntry
{
    io::UInt160 script_hash;
    std::string event_name;
    std::vector<std::shared_ptr<neo::vm::StackItem>> state;
    uint64_t timestamp;

    NotifyEntry() = default;
    NotifyEntry(const io::UInt160& hash, const std::string& event,
                const std::vector<std::shared_ptr<neo::vm::StackItem>>& data, uint64_t time = 0)
        : script_hash(hash), event_name(event), state(data), timestamp(time)
    {
    }
};

}  // namespace neo::smartcontract