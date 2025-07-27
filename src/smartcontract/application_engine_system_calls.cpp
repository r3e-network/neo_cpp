#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/system_call_exception.h>
#include <neo/smartcontract/system_calls.h>

namespace neo::smartcontract
{
void ApplicationEngine::RegisterSystemCall(const std::string& name, std::function<bool(vm::ExecutionEngine&)> handler,
                                           int64_t gasCost, CallFlags requiredFlags)
{
    SystemCallDescriptor descriptor;
    descriptor.name = name;
    descriptor.handler = handler;
    descriptor.gasCost = gasCost;
    descriptor.requiredFlags = requiredFlags;

    systemCalls_[name] = descriptor;

    // Register the system call with the VM
    vm::ExecutionEngine::RegisterSystemCall(
        name,
        [this, name](vm::ExecutionEngine& engine)
        {
            auto& descriptor = systemCalls_[name];

            // Check if the engine has the required flags
            if (descriptor.requiredFlags != CallFlags::None)
            {
                if (!HasFlag(descriptor.requiredFlags))
                {
                    std::string requiredFlagsStr;
                    if ((static_cast<uint8_t>(descriptor.requiredFlags) &
                         static_cast<uint8_t>(CallFlags::ReadStates)) != 0)
                        requiredFlagsStr += "ReadStates ";
                    if ((static_cast<uint8_t>(descriptor.requiredFlags) &
                         static_cast<uint8_t>(CallFlags::WriteStates)) != 0)
                        requiredFlagsStr += "WriteStates ";
                    if ((static_cast<uint8_t>(descriptor.requiredFlags) & static_cast<uint8_t>(CallFlags::AllowCall)) !=
                        0)
                        requiredFlagsStr += "AllowCall ";
                    if ((static_cast<uint8_t>(descriptor.requiredFlags) &
                         static_cast<uint8_t>(CallFlags::AllowNotify)) != 0)
                        requiredFlagsStr += "AllowNotify ";

                    throw MissingFlagsException(name, requiredFlagsStr);
                }
            }

            // Add gas cost
            if (descriptor.gasCost > 0)
            {
                try
                {
                    AddGas(descriptor.gasCost);
                }
                catch (const std::runtime_error& ex)
                {
                    throw InsufficientGasException(name, descriptor.gasCost, GetGasLeft());
                }
            }

            // Call the handler
            return descriptor.handler(engine);
        });
}

// Registration functions are declared in system_calls.h

void ApplicationEngine::RegisterSystemCalls()
{
    // Register all system calls from separate files
    RegisterRuntimeSystemCalls(*this);
    RegisterStorageSystemCalls(*this);
    RegisterContractSystemCalls(*this);
    RegisterCryptoSystemCalls(*this);
    RegisterJsonSystemCalls(*this);
    RegisterBinarySystemCalls(*this);
}
}  // namespace neo::smartcontract
