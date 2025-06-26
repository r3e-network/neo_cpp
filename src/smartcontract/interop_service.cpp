// Copyright (C) 2015-2025 The Neo Project.
//
// interop_service.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#include "neo/smartcontract/interop_service.h"
#include "neo/smartcontract/application_engine.h"
#include "neo/vm/stack_item.h"
#include "neo/io/uint160.h"
#include "neo/io/uint256.h"
#include "neo/cryptography/hash.h"
#include "neo/logging/logger.h"
#include <algorithm>
#include <stdexcept>

namespace neo {
namespace smartcontract {

// Hash calculation function
uint32_t calculate_interop_hash(const std::string& name) {
    auto hash = cryptography::Hash::sha256(reinterpret_cast<const uint8_t*>(name.data()), name.size());
    return *reinterpret_cast<const uint32_t*>(hash.data());
}

// InteropDescriptor implementation
InteropDescriptor::InteropDescriptor(std::string name, uint32_t hash,
                                   std::function<void(ApplicationEngine&)> handler,
                                   int64_t fixed_price, CallFlags required_call_flags)
    : name(std::move(name))
    , hash(hash)
    , handler(std::move(handler))
    , fixed_price(fixed_price)
    , required_call_flags(required_call_flags) {
}

// InteropService implementation
InteropService& InteropService::instance() {
    static InteropService instance;
    return instance;
}

void InteropService::initialize() {
    auto& service = instance();
    service.register_builtin_services();
    logging::Logger::info("InteropService initialized with {} services", service.services_.size());
}

const InteropDescriptor* InteropService::get_descriptor(uint32_t hash) const {
    auto it = services_.find(hash);
    return it != services_.end() ? &it->second : nullptr;
}

InteropDescriptor InteropService::register_service(const std::string& name,
                                                  std::function<void(ApplicationEngine&)> handler,
                                                  int64_t fixed_price,
                                                  CallFlags required_call_flags) {
    uint32_t hash = calculate_interop_hash(name);
    InteropDescriptor descriptor(name, hash, std::move(handler), fixed_price, required_call_flags);
    
    auto& service = instance();
    service.register_service_internal(descriptor);
    
    return descriptor;
}

void InteropService::register_service_internal(const InteropDescriptor& descriptor) {
    services_[descriptor.hash] = descriptor;
}

void InteropService::register_builtin_services() {
    // System.Runtime services
    register_service_internal(interop_descriptors::system_runtime_platform);
    register_service_internal(interop_descriptors::system_runtime_get_network);
    register_service_internal(interop_descriptors::system_runtime_get_address_version);
    register_service_internal(interop_descriptors::system_runtime_get_trigger);
    register_service_internal(interop_descriptors::system_runtime_get_time);
    register_service_internal(interop_descriptors::system_runtime_get_script_container);
    register_service_internal(interop_descriptors::system_runtime_get_executing_script_hash);
    register_service_internal(interop_descriptors::system_runtime_get_calling_script_hash);
    register_service_internal(interop_descriptors::system_runtime_get_entry_script_hash);
    register_service_internal(interop_descriptors::system_runtime_load_script);
    register_service_internal(interop_descriptors::system_runtime_check_witness);
    register_service_internal(interop_descriptors::system_runtime_get_invocation_counter);
    register_service_internal(interop_descriptors::system_runtime_get_random);
    register_service_internal(interop_descriptors::system_runtime_log);
    register_service_internal(interop_descriptors::system_runtime_notify);
    register_service_internal(interop_descriptors::system_runtime_get_notifications);
    register_service_internal(interop_descriptors::system_runtime_gas_left);
    register_service_internal(interop_descriptors::system_runtime_burn_gas);
    register_service_internal(interop_descriptors::system_runtime_current_signers);
    
    // System.Crypto services
    register_service_internal(interop_descriptors::system_crypto_check_sig);
    register_service_internal(interop_descriptors::system_crypto_check_multisig);
    
    // System.Contract services
    register_service_internal(interop_descriptors::system_contract_call);
    register_service_internal(interop_descriptors::system_contract_call_native);
    register_service_internal(interop_descriptors::system_contract_get_call_flags);
    register_service_internal(interop_descriptors::system_contract_create_standard_account);
    register_service_internal(interop_descriptors::system_contract_create_multisig_account);
    register_service_internal(interop_descriptors::system_contract_native_on_persist);
    register_service_internal(interop_descriptors::system_contract_native_post_persist);
    
    // System.Storage services
    register_service_internal(interop_descriptors::system_storage_get_context);
    register_service_internal(interop_descriptors::system_storage_get_readonly_context);
    register_service_internal(interop_descriptors::system_storage_as_readonly);
    register_service_internal(interop_descriptors::system_storage_get);
    register_service_internal(interop_descriptors::system_storage_find);
    register_service_internal(interop_descriptors::system_storage_put);
    register_service_internal(interop_descriptors::system_storage_delete);
    
    // System.Iterator services
    register_service_internal(interop_descriptors::system_iterator_next);
    register_service_internal(interop_descriptors::system_iterator_value);
}

// System.Runtime implementations
void InteropService::runtime_platform(ApplicationEngine& engine) {
    engine.push(vm::StackItem::create_string("NEO"));
}

void InteropService::runtime_get_network(ApplicationEngine& engine) {
    engine.push(vm::StackItem::create_integer(engine.protocol_settings().network()));
}

void InteropService::runtime_get_address_version(ApplicationEngine& engine) {
    engine.push(vm::StackItem::create_integer(engine.protocol_settings().address_version()));
}

void InteropService::runtime_get_trigger(ApplicationEngine& engine) {
    engine.push(vm::StackItem::create_integer(static_cast<int>(engine.trigger())));
}

void InteropService::runtime_get_time(ApplicationEngine& engine) {
    auto timestamp = engine.persisting_block() ? 
        engine.persisting_block()->header().timestamp : 0;
    engine.push(vm::StackItem::create_integer(timestamp));
}

void InteropService::runtime_get_script_container(ApplicationEngine& engine) {
    if (engine.script_container()) {
        // Convert script container to stack item
        engine.push(engine.script_container()->to_stack_item());
    } else {
        engine.push(vm::StackItem::create_null());
    }
}

void InteropService::runtime_get_executing_script_hash(ApplicationEngine& engine) {
    auto hash = engine.current_script_hash();
    engine.push(vm::StackItem::create_byte_string(hash.to_array()));
}

void InteropService::runtime_get_calling_script_hash(ApplicationEngine& engine) {
    auto hash = engine.calling_script_hash();
    if (hash.has_value()) {
        engine.push(vm::StackItem::create_byte_string(hash->to_array()));
    } else {
        engine.push(vm::StackItem::create_null());
    }
}

void InteropService::runtime_get_entry_script_hash(ApplicationEngine& engine) {
    auto hash = engine.entry_script_hash();
    engine.push(vm::StackItem::create_byte_string(hash.to_array()));
}

void InteropService::runtime_load_script(ApplicationEngine& engine) {
    auto script = engine.pop()->get_span();
    auto call_flags = static_cast<CallFlags>(engine.pop()->get_integer());
    auto args = engine.pop();
    
    engine.load_script(script, call_flags, args);
}

void InteropService::runtime_check_witness(ApplicationEngine& engine) {
    auto hash_or_pubkey = engine.pop()->get_span();
    bool result = false;
    
    if (hash_or_pubkey.size() == 20) {
        // Script hash
        UInt160 script_hash(hash_or_pubkey);
        result = engine.check_witness(script_hash);
    } else if (hash_or_pubkey.size() == 33) {
        // Public key
        // TODO: Convert public key to script hash and check
        result = false; // Placeholder
    }
    
    engine.push(vm::StackItem::create_boolean(result));
}

void InteropService::runtime_get_invocation_counter(ApplicationEngine& engine) {
    auto count = engine.get_invocation_counter();
    engine.push(vm::StackItem::create_integer(count));
}

void InteropService::runtime_get_random(ApplicationEngine& engine) {
    auto random = engine.get_random();
    engine.push(vm::StackItem::create_byte_string(random));
}

void InteropService::runtime_log(ApplicationEngine& engine) {
    auto message = engine.pop()->get_string();
    engine.send_log(message);
}

void InteropService::runtime_notify(ApplicationEngine& engine) {
    auto event_name = engine.pop()->get_string();
    auto state = engine.pop();
    engine.send_notification(event_name, state);
}

void InteropService::runtime_get_notifications(ApplicationEngine& engine) {
    auto script_hash_item = engine.pop();
    std::optional<UInt160> script_hash;
    
    if (!script_hash_item->is_null()) {
        auto hash_bytes = script_hash_item->get_span();
        if (hash_bytes.size() == 20) {
            script_hash = UInt160(hash_bytes);
        }
    }
    
    auto notifications = engine.get_notifications(script_hash);
    engine.push(notifications);
}

void InteropService::runtime_gas_left(ApplicationEngine& engine) {
    auto gas_left = engine.gas_left();
    engine.push(vm::StackItem::create_integer(gas_left));
}

void InteropService::runtime_burn_gas(ApplicationEngine& engine) {
    auto gas = engine.pop()->get_integer();
    engine.add_gas(gas);
}

void InteropService::runtime_current_signers(ApplicationEngine& engine) {
    auto signers = engine.current_signers();
    engine.push(signers);
}

// System.Crypto implementations
void InteropService::crypto_check_sig(ApplicationEngine& engine) {
    auto pubkey = engine.pop()->get_span();
    auto signature = engine.pop()->get_span();
    
    bool result = engine.check_sig(pubkey, signature);
    engine.push(vm::StackItem::create_boolean(result));
}

void InteropService::crypto_check_multisig(ApplicationEngine& engine) {
    auto pubkeys = engine.pop();
    auto signatures = engine.pop();
    
    bool result = engine.check_multisig(pubkeys, signatures);
    engine.push(vm::StackItem::create_boolean(result));
}

// System.Contract implementations
void InteropService::contract_call(ApplicationEngine& engine) {
    auto script_hash = UInt160(engine.pop()->get_span());
    auto method = engine.pop()->get_string();
    auto call_flags = static_cast<CallFlags>(engine.pop()->get_integer());
    auto args = engine.pop();
    
    engine.call_contract(script_hash, method, call_flags, args);
}

void InteropService::contract_call_native(ApplicationEngine& engine) {
    auto version = engine.pop()->get_integer();
    auto name = engine.pop()->get_string();
    auto method = engine.pop()->get_string();
    auto args = engine.pop();
    
    engine.call_native_contract(version, name, method, args);
}

void InteropService::contract_get_call_flags(ApplicationEngine& engine) {
    auto flags = engine.get_call_flags();
    engine.push(vm::StackItem::create_integer(static_cast<int>(flags)));
}

void InteropService::contract_create_standard_account(ApplicationEngine& engine) {
    auto pubkey = engine.pop()->get_span();
    auto script_hash = engine.create_standard_account(pubkey);
    engine.push(vm::StackItem::create_byte_string(script_hash.to_array()));
}

void InteropService::contract_create_multisig_account(ApplicationEngine& engine) {
    auto m = engine.pop()->get_integer();
    auto pubkeys = engine.pop();
    auto script_hash = engine.create_multisig_account(m, pubkeys);
    engine.push(vm::StackItem::create_byte_string(script_hash.to_array()));
}

void InteropService::contract_native_on_persist(ApplicationEngine& engine) {
    engine.native_on_persist();
}

void InteropService::contract_native_post_persist(ApplicationEngine& engine) {
    engine.native_post_persist();
}

// System.Storage implementations
void InteropService::storage_get_context(ApplicationEngine& engine) {
    auto context = engine.get_storage_context();
    engine.push(context);
}

void InteropService::storage_get_readonly_context(ApplicationEngine& engine) {
    auto context = engine.get_readonly_storage_context();
    engine.push(context);
}

void InteropService::storage_as_readonly(ApplicationEngine& engine) {
    auto context = engine.pop();
    auto readonly_context = engine.as_readonly_storage_context(context);
    engine.push(readonly_context);
}

void InteropService::storage_get(ApplicationEngine& engine) {
    auto context = engine.pop();
    auto key = engine.pop()->get_span();
    auto value = engine.storage_get(context, key);
    engine.push(value);
}

void InteropService::storage_find(ApplicationEngine& engine) {
    auto context = engine.pop();
    auto prefix = engine.pop()->get_span();
    auto options = engine.pop()->get_integer();
    auto iterator = engine.storage_find(context, prefix, options);
    engine.push(iterator);
}

void InteropService::storage_put(ApplicationEngine& engine) {
    auto context = engine.pop();
    auto key = engine.pop()->get_span();
    auto value = engine.pop()->get_span();
    engine.storage_put(context, key, value);
}

void InteropService::storage_delete(ApplicationEngine& engine) {
    auto context = engine.pop();
    auto key = engine.pop()->get_span();
    engine.storage_delete(context, key);
}

// System.Iterator implementations
void InteropService::iterator_next(ApplicationEngine& engine) {
    auto iterator = engine.pop();
    bool result = engine.iterator_next(iterator);
    engine.push(vm::StackItem::create_boolean(result));
}

void InteropService::iterator_value(ApplicationEngine& engine) {
    auto iterator = engine.pop();
    auto value = engine.iterator_value(iterator);
    engine.push(value);
}

// Global interop descriptors
namespace interop_descriptors {
    const InteropDescriptor system_runtime_platform = 
        InteropService::register_service("System.Runtime.Platform", 
                                        InteropService::runtime_platform, 
                                        1 << 3, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_network = 
        InteropService::register_service("System.Runtime.GetNetwork", 
                                        InteropService::runtime_get_network, 
                                        1 << 3, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_address_version = 
        InteropService::register_service("System.Runtime.GetAddressVersion", 
                                        InteropService::runtime_get_address_version, 
                                        1 << 3, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_trigger = 
        InteropService::register_service("System.Runtime.GetTrigger", 
                                        InteropService::runtime_get_trigger, 
                                        1 << 3, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_time = 
        InteropService::register_service("System.Runtime.GetTime", 
                                        InteropService::runtime_get_time, 
                                        1 << 3, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_script_container = 
        InteropService::register_service("System.Runtime.GetScriptContainer", 
                                        InteropService::runtime_get_script_container, 
                                        1 << 3, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_executing_script_hash = 
        InteropService::register_service("System.Runtime.GetExecutingScriptHash", 
                                        InteropService::runtime_get_executing_script_hash, 
                                        1 << 4, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_calling_script_hash = 
        InteropService::register_service("System.Runtime.GetCallingScriptHash", 
                                        InteropService::runtime_get_calling_script_hash, 
                                        1 << 4, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_entry_script_hash = 
        InteropService::register_service("System.Runtime.GetEntryScriptHash", 
                                        InteropService::runtime_get_entry_script_hash, 
                                        1 << 4, CallFlags::None);
    
    const InteropDescriptor system_runtime_load_script = 
        InteropService::register_service("System.Runtime.LoadScript", 
                                        InteropService::runtime_load_script, 
                                        1 << 15, CallFlags::AllowCall);
    
    const InteropDescriptor system_runtime_check_witness = 
        InteropService::register_service("System.Runtime.CheckWitness", 
                                        InteropService::runtime_check_witness, 
                                        1 << 10, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_invocation_counter = 
        InteropService::register_service("System.Runtime.GetInvocationCounter", 
                                        InteropService::runtime_get_invocation_counter, 
                                        1 << 4, CallFlags::None);
    
    const InteropDescriptor system_runtime_get_random = 
        InteropService::register_service("System.Runtime.GetRandom", 
                                        InteropService::runtime_get_random, 
                                        0, CallFlags::None);
    
    const InteropDescriptor system_runtime_log = 
        InteropService::register_service("System.Runtime.Log", 
                                        InteropService::runtime_log, 
                                        1 << 15, CallFlags::AllowNotify);
    
    const InteropDescriptor system_runtime_notify = 
        InteropService::register_service("System.Runtime.Notify", 
                                        InteropService::runtime_notify, 
                                        1 << 15, CallFlags::AllowNotify);
    
    const InteropDescriptor system_runtime_get_notifications = 
        InteropService::register_service("System.Runtime.GetNotifications", 
                                        InteropService::runtime_get_notifications, 
                                        1 << 12, CallFlags::None);
    
    const InteropDescriptor system_runtime_gas_left = 
        InteropService::register_service("System.Runtime.GasLeft", 
                                        InteropService::runtime_gas_left, 
                                        1 << 4, CallFlags::None);
    
    const InteropDescriptor system_runtime_burn_gas = 
        InteropService::register_service("System.Runtime.BurnGas", 
                                        InteropService::runtime_burn_gas, 
                                        1 << 4, CallFlags::None);
    
    const InteropDescriptor system_runtime_current_signers = 
        InteropService::register_service("System.Runtime.CurrentSigners", 
                                        InteropService::runtime_current_signers, 
                                        1 << 4, CallFlags::None);
    
    // System.Crypto
    const InteropDescriptor system_crypto_check_sig = 
        InteropService::register_service("System.Crypto.CheckSig", 
                                        InteropService::crypto_check_sig, 
                                        1 << 15, CallFlags::None);
    
    const InteropDescriptor system_crypto_check_multisig = 
        InteropService::register_service("System.Crypto.CheckMultisig", 
                                        InteropService::crypto_check_multisig, 
                                        0, CallFlags::None);
    
    // System.Contract
    const InteropDescriptor system_contract_call = 
        InteropService::register_service("System.Contract.Call", 
                                        InteropService::contract_call, 
                                        1 << 15, CallFlags::ReadStates | CallFlags::AllowCall);
    
    const InteropDescriptor system_contract_call_native = 
        InteropService::register_service("System.Contract.CallNative", 
                                        InteropService::contract_call_native, 
                                        0, CallFlags::None);
    
    const InteropDescriptor system_contract_get_call_flags = 
        InteropService::register_service("System.Contract.GetCallFlags", 
                                        InteropService::contract_get_call_flags, 
                                        1 << 10, CallFlags::None);
    
    const InteropDescriptor system_contract_create_standard_account = 
        InteropService::register_service("System.Contract.CreateStandardAccount", 
                                        InteropService::contract_create_standard_account, 
                                        0, CallFlags::None);
    
    const InteropDescriptor system_contract_create_multisig_account = 
        InteropService::register_service("System.Contract.CreateMultisigAccount", 
                                        InteropService::contract_create_multisig_account, 
                                        0, CallFlags::None);
    
    const InteropDescriptor system_contract_native_on_persist = 
        InteropService::register_service("System.Contract.NativeOnPersist", 
                                        InteropService::contract_native_on_persist, 
                                        0, CallFlags::States);
    
    const InteropDescriptor system_contract_native_post_persist = 
        InteropService::register_service("System.Contract.NativePostPersist", 
                                        InteropService::contract_native_post_persist, 
                                        0, CallFlags::States);
    
    // System.Storage
    const InteropDescriptor system_storage_get_context = 
        InteropService::register_service("System.Storage.GetContext", 
                                        InteropService::storage_get_context, 
                                        1 << 4, CallFlags::ReadStates);
    
    const InteropDescriptor system_storage_get_readonly_context = 
        InteropService::register_service("System.Storage.GetReadOnlyContext", 
                                        InteropService::storage_get_readonly_context, 
                                        1 << 4, CallFlags::ReadStates);
    
    const InteropDescriptor system_storage_as_readonly = 
        InteropService::register_service("System.Storage.AsReadOnly", 
                                        InteropService::storage_as_readonly, 
                                        1 << 4, CallFlags::ReadStates);
    
    const InteropDescriptor system_storage_get = 
        InteropService::register_service("System.Storage.Get", 
                                        InteropService::storage_get, 
                                        1 << 15, CallFlags::ReadStates);
    
    const InteropDescriptor system_storage_find = 
        InteropService::register_service("System.Storage.Find", 
                                        InteropService::storage_find, 
                                        1 << 15, CallFlags::ReadStates);
    
    const InteropDescriptor system_storage_put = 
        InteropService::register_service("System.Storage.Put", 
                                        InteropService::storage_put, 
                                        1 << 15, CallFlags::WriteStates);
    
    const InteropDescriptor system_storage_delete = 
        InteropService::register_service("System.Storage.Delete", 
                                        InteropService::storage_delete, 
                                        1 << 15, CallFlags::WriteStates);
    
    // System.Iterator
    const InteropDescriptor system_iterator_next = 
        InteropService::register_service("System.Iterator.Next", 
                                        InteropService::iterator_next, 
                                        1 << 15, CallFlags::None);
    
    const InteropDescriptor system_iterator_value = 
        InteropService::register_service("System.Iterator.Value", 
                                        InteropService::iterator_value, 
                                        1 << 4, CallFlags::None);
}

} // namespace smartcontract
} // namespace neo