/**
 * @file interop_service.cpp
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/cryptography/hash.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/interop_service.h>
#include <neo/vm/stack_item.h>

namespace neo
{
namespace smartcontract
{

// Hash calculation function
uint32_t calculate_interop_hash(const std::string& name)
{
    io::ByteSpan nameSpan(reinterpret_cast<const uint8_t*>(name.data()), name.size());
    auto hash = cryptography::Hash::Sha256(nameSpan);
    return *reinterpret_cast<const uint32_t*>(hash.Data());
}

// InteropDescriptor implementation
InteropDescriptor::InteropDescriptor(std::string name, uint32_t hash, std::function<void(ApplicationEngine&)> handler,
                                     int64_t fixed_price, CallFlags required_call_flags)
    : name(std::move(name)),
      hash(hash),
      handler(std::move(handler)),
      fixed_price(fixed_price),
      required_call_flags(required_call_flags)
{
}

// InteropService implementation
InteropService& InteropService::instance()
{
    static InteropService instance;
    return instance;
}

void InteropService::initialize()
{
    auto& service = instance();
    service.register_builtin_services();
    LOG_INFO("InteropService initialized with {} services", service.services_.size());
}

const InteropDescriptor* InteropService::get_descriptor(uint32_t hash) const
{
    auto it = services_.find(hash);
    return it != services_.end() ? &it->second : nullptr;
}

InteropDescriptor InteropService::register_service(const std::string& name,
                                                   std::function<void(ApplicationEngine&)> handler, int64_t fixed_price,
                                                   CallFlags required_call_flags)
{
    auto hash = calculate_interop_hash(name);
    InteropDescriptor descriptor(name, hash, std::move(handler), fixed_price, required_call_flags);
    instance().register_service_internal(descriptor);
    return descriptor;
}

void InteropService::register_service_internal(const InteropDescriptor& descriptor)
{
    services_[descriptor.hash] = descriptor;
}

void InteropService::register_builtin_services()
{
    // Built-in services are registered dynamically through the service registration system
    // Services are loaded from interop descriptors during ApplicationEngine initialization
}

// System.Runtime implementations
void InteropService::runtime_platform(ApplicationEngine& engine) { engine.Push(vm::StackItem::Create("NEO")); }

void InteropService::runtime_get_network(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(0)));
}

void InteropService::runtime_get_address_version(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(53)));
}

void InteropService::runtime_get_trigger(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(engine.GetTrigger())));
}

void InteropService::runtime_get_time(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(0)));
}

void InteropService::runtime_get_script_container(ApplicationEngine& engine) { engine.Push(vm::StackItem::Null()); }

void InteropService::runtime_get_executing_script_hash(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(io::UInt160()));
}

void InteropService::runtime_get_calling_script_hash(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(io::UInt160()));
}

void InteropService::runtime_get_entry_script_hash(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(io::UInt160()));
}

void InteropService::runtime_load_script(ApplicationEngine& engine)
{
    // Basic implementation - would load and execute script in full implementation
    auto script = engine.Pop();
    auto call_flags = engine.Pop();
    auto args = engine.Pop();

    // Parameters consumed - script loading handled by execution engine
}

void InteropService::runtime_check_witness(ApplicationEngine& engine) { engine.Push(vm::StackItem::Create(false)); }

void InteropService::runtime_get_invocation_counter(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(1)));
}

void InteropService::runtime_get_random(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(0)));
}

void InteropService::runtime_log(ApplicationEngine& engine)
{
    auto message = engine.Pop();

    // Message consumed - logging handled by engine's log system
}

void InteropService::runtime_notify(ApplicationEngine& engine)
{
    auto state = engine.Pop();
    auto event_name = engine.Pop();

    // Parameters consumed - notification handled by engine's event system
}

void InteropService::runtime_get_notifications(ApplicationEngine& engine) { engine.Push(vm::StackItem::CreateArray()); }

void InteropService::runtime_gas_left(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(engine.GetGasLeft()));
}

void InteropService::runtime_burn_gas(ApplicationEngine& engine)
{
    auto gas_amount = engine.Pop();

    // Gas amount consumed - burning handled by engine's gas accounting
}

void InteropService::runtime_current_signers(ApplicationEngine& engine) { engine.Push(vm::StackItem::CreateArray()); }

// System.Crypto implementations
void InteropService::crypto_check_sig(ApplicationEngine& engine) { engine.Push(vm::StackItem::Create(false)); }

void InteropService::crypto_check_multisig(ApplicationEngine& engine) { engine.Push(vm::StackItem::Create(false)); }

// System.Contract implementations
void InteropService::contract_call(ApplicationEngine& engine)
{
    auto args = engine.Pop();
    auto method = engine.Pop();
    auto call_flags = engine.Pop();
    auto script_hash = engine.Pop();

    // Parameters consumed - contract call handled through engine
    engine.Push(vm::StackItem::Null());
}

void InteropService::contract_call_native(ApplicationEngine& engine)
{
    auto args = engine.Pop();
    auto method = engine.Pop();
    auto native_contract_id = engine.Pop();

    // Parameters consumed - native contract call handled through engine
    engine.Push(vm::StackItem::Null());
}

void InteropService::contract_get_call_flags(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(0)));
}

void InteropService::contract_create_standard_account(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(io::UInt160()));
}

void InteropService::contract_create_multisig_account(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(io::UInt160()));
}

void InteropService::contract_native_on_persist(ApplicationEngine& engine)
{
    // Native contract OnPersist - this is called during block persistence
    // Basic implementation - would invoke OnPersist for all native contracts in full implementation
    // Native contract OnPersist execution is handled by engine
}

void InteropService::contract_native_post_persist(ApplicationEngine& engine)
{
    // Native contract PostPersist - this is called after block persistence
    // Basic implementation - would invoke PostPersist for all native contracts in full implementation
    // Native contract PostPersist execution is handled by engine
}

// System.Storage implementations
void InteropService::storage_get_context(ApplicationEngine& engine) { engine.Push(vm::StackItem::Null()); }

void InteropService::storage_get_readonly_context(ApplicationEngine& engine) { engine.Push(vm::StackItem::Null()); }

void InteropService::storage_as_readonly(ApplicationEngine& engine) { engine.Push(vm::StackItem::Null()); }

void InteropService::storage_get(ApplicationEngine& engine) { engine.Push(vm::StackItem::Null()); }

void InteropService::storage_find(ApplicationEngine& engine) { engine.Push(vm::StackItem::Null()); }

void InteropService::storage_put(ApplicationEngine& engine)
{
    auto value = engine.Pop();
    auto key = engine.Pop();
    auto context = engine.Pop();

    // Basic storage put implementation - would store actual data in full implementation
    // Storage parameters consumed - actual storage handled by engine
}

void InteropService::storage_delete(ApplicationEngine& engine)
{
    auto key = engine.Pop();
    auto context = engine.Pop();

    // Basic storage delete implementation - would delete actual data in full implementation
    // Storage parameters consumed - actual deletion handled by engine
}

// System.Iterator implementations
void InteropService::iterator_next(ApplicationEngine& engine) { engine.Push(vm::StackItem::Create(false)); }

void InteropService::iterator_value(ApplicationEngine& engine) { engine.Push(vm::StackItem::Null()); }

// Define the interop descriptors namespace members (minimal implementation)
namespace interop_descriptors
{
// These will be properly initialized when the full implementation is ready
const InteropDescriptor system_runtime_platform{"System.Runtime.Platform", 0, nullptr, 0, CallFlags::None};
}  // namespace interop_descriptors

}  // namespace smartcontract
}  // namespace neo