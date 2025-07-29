#include <functional>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/interop_service.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/syscalls.h>
#include <unordered_map>

namespace neo::vm
{

class SyscallHandler
{
  private:
    std::unordered_map<uint32_t, std::function<void(ExecutionEngine&)>> handlers_;

  public:
    SyscallHandler()
    {
        RegisterBuiltinSyscalls();
    }

    void RegisterSyscall(uint32_t id, std::function<void(ExecutionEngine&)> handler)
    {
        handlers_[id] = std::move(handler);
    }

    bool HandleSyscall(ExecutionEngine& engine, uint32_t id)
    {
        auto it = handlers_.find(id);
        if (it != handlers_.end())
        {
            try
            {
                it->second(engine);
                return true;
            }
            catch (const std::exception&)
            {
                // Syscall failed
                return false;
            }
        }
        return false;
    }

  private:
    void RegisterBuiltinSyscalls()
    {
        // System.Runtime syscalls
        RegisterSyscall(CalculateHash("System.Runtime.Platform"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_platform(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetNetwork"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_network(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetAddressVersion"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_address_version(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetTrigger"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_trigger(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetTime"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_time(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetScriptContainer"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_script_container(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetExecutingScriptHash"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_executing_script_hash(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetCallingScriptHash"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_calling_script_hash(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetEntryScriptHash"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_entry_script_hash(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.CheckWitness"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_check_witness(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetInvocationCounter"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_invocation_counter(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetRandom"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_random(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.Log"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_log(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.Notify"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_notify(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GetNotifications"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_get_notifications(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.GasLeft"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_gas_left(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Runtime.BurnGas"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::runtime_burn_gas(appEngine);
                        });

        // System.Crypto syscalls
        RegisterSyscall(CalculateHash("System.Crypto.CheckSig"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::crypto_check_sig(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Crypto.CheckMultisig"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::crypto_check_multisig(appEngine);
                        });

        // System.Contract syscalls
        RegisterSyscall(CalculateHash("System.Contract.Call"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::contract_call(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Contract.CallNative"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::contract_call_native(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Contract.GetCallFlags"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::contract_get_call_flags(appEngine);
                        });

        // System.Storage syscalls
        RegisterSyscall(CalculateHash("System.Storage.GetContext"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::storage_get_context(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Storage.GetReadOnlyContext"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::storage_get_readonly_context(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Storage.AsReadOnly"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::storage_as_readonly(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Storage.Get"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::storage_get(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Storage.Find"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::storage_find(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Storage.Put"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::storage_put(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Storage.Delete"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::storage_delete(appEngine);
                        });

        // System.Iterator syscalls
        RegisterSyscall(CalculateHash("System.Iterator.Next"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::iterator_next(appEngine);
                        });

        RegisterSyscall(CalculateHash("System.Iterator.Value"),
                        [](ExecutionEngine& engine)
                        {
                            auto& appEngine = static_cast<smartcontract::ApplicationEngine&>(engine);
                            smartcontract::InteropService::iterator_value(appEngine);
                        });
    }

    uint32_t CalculateHash(const std::string& name)
    {
        return smartcontract::calculate_interop_hash(name);
    }
};

static SyscallHandler& GetSyscallHandler()
{
    static SyscallHandler handler;
    return handler;
}

void RegisterSyscall(uint32_t id, std::function<void(ExecutionEngine&)> handler)
{
    GetSyscallHandler().RegisterSyscall(id, std::move(handler));
}

bool HandleSyscall(ExecutionEngine& engine, uint32_t id)
{
    return GetSyscallHandler().HandleSyscall(engine, id);
}

}  // namespace neo::vm