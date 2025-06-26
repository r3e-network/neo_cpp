// Copyright (C) 2015-2025 The Neo Project.
//
// interop_service.h file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef NEO_SMARTCONTRACT_INTEROP_SERVICE_H
#define NEO_SMARTCONTRACT_INTEROP_SERVICE_H

#include "neo/smartcontract/interop_descriptor.h"
#include "neo/smartcontract/interop_parameter_descriptor.h"
#include <unordered_map>
#include <memory>

namespace neo {
namespace smartcontract {

class ApplicationEngine;

/**
 * @brief Provides interoperable services for the Neo virtual machine.
 * 
 * The InteropService class manages all system calls and interoperable services
 * that can be invoked from smart contracts running on the Neo VM.
 */
class InteropService {
public:
    /**
     * @brief Gets the singleton instance of InteropService.
     */
    static InteropService& instance();
    
    /**
     * @brief Initializes all interop services.
     */
    static void initialize();
    
    /**
     * @brief Gets all registered interop services.
     */
    const std::unordered_map<uint32_t, InteropDescriptor>& services() const {
        return services_;
    }
    
    /**
     * @brief Gets an interop descriptor by hash.
     */
    const InteropDescriptor* get_descriptor(uint32_t hash) const;
    
    /**
     * @brief Registers an interop service.
     */
    static InteropDescriptor register_service(const std::string& name,
                                            std::function<void(ApplicationEngine&)> handler,
                                            int64_t fixed_price,
                                            CallFlags required_call_flags);
    
    // System.Runtime services
    static void runtime_platform(ApplicationEngine& engine);
    static void runtime_get_network(ApplicationEngine& engine);
    static void runtime_get_address_version(ApplicationEngine& engine);
    static void runtime_get_trigger(ApplicationEngine& engine);
    static void runtime_get_time(ApplicationEngine& engine);
    static void runtime_get_script_container(ApplicationEngine& engine);
    static void runtime_get_executing_script_hash(ApplicationEngine& engine);
    static void runtime_get_calling_script_hash(ApplicationEngine& engine);
    static void runtime_get_entry_script_hash(ApplicationEngine& engine);
    static void runtime_load_script(ApplicationEngine& engine);
    static void runtime_check_witness(ApplicationEngine& engine);
    static void runtime_get_invocation_counter(ApplicationEngine& engine);
    static void runtime_get_random(ApplicationEngine& engine);
    static void runtime_log(ApplicationEngine& engine);
    static void runtime_notify(ApplicationEngine& engine);
    static void runtime_get_notifications(ApplicationEngine& engine);
    static void runtime_gas_left(ApplicationEngine& engine);
    static void runtime_burn_gas(ApplicationEngine& engine);
    static void runtime_current_signers(ApplicationEngine& engine);
    
    // System.Crypto services
    static void crypto_check_sig(ApplicationEngine& engine);
    static void crypto_check_multisig(ApplicationEngine& engine);
    
    // System.Contract services
    static void contract_call(ApplicationEngine& engine);
    static void contract_call_native(ApplicationEngine& engine);
    static void contract_get_call_flags(ApplicationEngine& engine);
    static void contract_create_standard_account(ApplicationEngine& engine);
    static void contract_create_multisig_account(ApplicationEngine& engine);
    static void contract_native_on_persist(ApplicationEngine& engine);
    static void contract_native_post_persist(ApplicationEngine& engine);
    
    // System.Storage services
    static void storage_get_context(ApplicationEngine& engine);
    static void storage_get_readonly_context(ApplicationEngine& engine);
    static void storage_as_readonly(ApplicationEngine& engine);
    static void storage_get(ApplicationEngine& engine);
    static void storage_find(ApplicationEngine& engine);
    static void storage_put(ApplicationEngine& engine);
    static void storage_delete(ApplicationEngine& engine);
    
    // System.Iterator services
    static void iterator_next(ApplicationEngine& engine);
    static void iterator_value(ApplicationEngine& engine);

private:
    InteropService() = default;
    
    std::unordered_map<uint32_t, InteropDescriptor> services_;
    
    /**
     * @brief Registers all built-in interop services.
     */
    void register_builtin_services();
    
    /**
     * @brief Helper function to register a service.
     */
    void register_service_internal(const InteropDescriptor& descriptor);
};

// Global interop descriptors (matching C# static readonly fields)
namespace interop_descriptors {
    // System.Runtime
    extern const InteropDescriptor system_runtime_platform;
    extern const InteropDescriptor system_runtime_get_network;
    extern const InteropDescriptor system_runtime_get_address_version;
    extern const InteropDescriptor system_runtime_get_trigger;
    extern const InteropDescriptor system_runtime_get_time;
    extern const InteropDescriptor system_runtime_get_script_container;
    extern const InteropDescriptor system_runtime_get_executing_script_hash;
    extern const InteropDescriptor system_runtime_get_calling_script_hash;
    extern const InteropDescriptor system_runtime_get_entry_script_hash;
    extern const InteropDescriptor system_runtime_load_script;
    extern const InteropDescriptor system_runtime_check_witness;
    extern const InteropDescriptor system_runtime_get_invocation_counter;
    extern const InteropDescriptor system_runtime_get_random;
    extern const InteropDescriptor system_runtime_log;
    extern const InteropDescriptor system_runtime_notify;
    extern const InteropDescriptor system_runtime_get_notifications;
    extern const InteropDescriptor system_runtime_gas_left;
    extern const InteropDescriptor system_runtime_burn_gas;
    extern const InteropDescriptor system_runtime_current_signers;
    
    // System.Crypto
    extern const InteropDescriptor system_crypto_check_sig;
    extern const InteropDescriptor system_crypto_check_multisig;
    
    // System.Contract
    extern const InteropDescriptor system_contract_call;
    extern const InteropDescriptor system_contract_call_native;
    extern const InteropDescriptor system_contract_get_call_flags;
    extern const InteropDescriptor system_contract_create_standard_account;
    extern const InteropDescriptor system_contract_create_multisig_account;
    extern const InteropDescriptor system_contract_native_on_persist;
    extern const InteropDescriptor system_contract_native_post_persist;
    
    // System.Storage
    extern const InteropDescriptor system_storage_get_context;
    extern const InteropDescriptor system_storage_get_readonly_context;
    extern const InteropDescriptor system_storage_as_readonly;
    extern const InteropDescriptor system_storage_get;
    extern const InteropDescriptor system_storage_find;
    extern const InteropDescriptor system_storage_put;
    extern const InteropDescriptor system_storage_delete;
    
    // System.Iterator
    extern const InteropDescriptor system_iterator_next;
    extern const InteropDescriptor system_iterator_value;
}

} // namespace smartcontract
} // namespace neo

#endif // NEO_SMARTCONTRACT_INTEROP_SERVICE_H