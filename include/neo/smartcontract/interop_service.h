/**
 * @file interop_service.h
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#ifndef NEO_SMARTCONTRACT_INTEROP_SERVICE_H
#define NEO_SMARTCONTRACT_INTEROP_SERVICE_H

#include <memory>
#include <unordered_map>

#include "neo/smartcontract/interop_descriptor.h"
#include "neo/smartcontract/interop_parameter_descriptor.h"

namespace neo
{
namespace smartcontract
{

class ApplicationEngine;

/**
 * @brief Calculates the interop hash (first 4 bytes of SHA256) for a given syscall name.
 * @param name The syscall name.
 * @return The 32-bit interop hash.
 */
uint32_t calculate_interop_hash(const std::string& name);

/**
 * @brief Provides interoperable services for the Neo virtual machine.
 *
 * The InteropService class manages all system calls and interoperable services
 * that can be invoked from smart contracts running on the Neo VM.
 */
class InteropService
{
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
    const std::unordered_map<uint32_t, InteropDescriptor>& services() const { return services_; }

    /**
     * @brief Gets an interop descriptor by hash.
     */
    const InteropDescriptor* get_descriptor(uint32_t hash) const;

    /**
     * @brief Registers an interop service.
     */
    static InteropDescriptor register_service(const std::string& name, std::function<void(ApplicationEngine&)> handler,
                                              int64_t fixed_price, CallFlags required_call_flags);

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
    static void iterator_key(ApplicationEngine& engine);
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

}  // namespace smartcontract
}  // namespace neo

#endif  // NEO_SMARTCONTRACT_INTEROP_SERVICE_H
