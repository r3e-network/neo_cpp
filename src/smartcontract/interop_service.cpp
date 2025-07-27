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
    : name(std::move(name)), hash(hash), handler(std::move(handler)), fixed_price(fixed_price),
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
    // Register only essential services for now
    // Services will be registered when interop_descriptors are available
}

// System.Runtime implementations
void InteropService::runtime_platform(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create("NEO"));
}

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

void InteropService::runtime_get_script_container(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Null());
}

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
    // TODO: Implement when available
}

void InteropService::runtime_check_witness(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(false));
}

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
    // TODO: Implement logging
}

void InteropService::runtime_notify(ApplicationEngine& engine)
{
    // TODO: Implement notification
}

void InteropService::runtime_get_notifications(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::CreateArray());
}

void InteropService::runtime_gas_left(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(engine.GetGasLeft()));
}

void InteropService::runtime_burn_gas(ApplicationEngine& engine)
{
    // TODO: Implement gas burning
}

void InteropService::runtime_current_signers(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::CreateArray());
}

// System.Crypto implementations
void InteropService::crypto_check_sig(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(false));
}

void InteropService::crypto_check_multisig(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(false));
}

// System.Contract implementations
void InteropService::contract_call(ApplicationEngine& engine)
{
    // TODO: Implement contract call
}

void InteropService::contract_call_native(ApplicationEngine& engine)
{
    // TODO: Implement native contract call
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
    // TODO: Implement native contract persist
}

void InteropService::contract_native_post_persist(ApplicationEngine& engine)
{
    // TODO: Implement native contract post persist
}

// System.Storage implementations
void InteropService::storage_get_context(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Null());
}

void InteropService::storage_get_readonly_context(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Null());
}

void InteropService::storage_as_readonly(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Null());
}

void InteropService::storage_get(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Null());
}

void InteropService::storage_find(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Null());
}

void InteropService::storage_put(ApplicationEngine& engine)
{
    // TODO: Implement storage put
}

void InteropService::storage_delete(ApplicationEngine& engine)
{
    // TODO: Implement storage delete
}

// System.Iterator implementations
void InteropService::iterator_next(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(false));
}

void InteropService::iterator_value(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Null());
}

// Define the interop descriptors namespace members (minimal implementation)
namespace interop_descriptors
{
// These will be properly initialized when the full implementation is ready
const InteropDescriptor system_runtime_platform{"System.Runtime.Platform", 0, nullptr, 0, CallFlags::None};
}  // namespace interop_descriptors

}  // namespace smartcontract
}  // namespace neo