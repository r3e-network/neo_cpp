/**
 * @file native_contract.h
 * @brief Native contract implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/persistence/data_cache.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/contract.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::smartcontract
{
/**
 * @brief Represents a native contract.
 */
class NativeContract
{
   public:
    /**
     * @brief Constructs a NativeContract.
     * @param name The name.
     * @param id The id.
     */
    NativeContract(const std::string& name, int32_t id);

    /**
     * @brief Gets the name.
     * @return The name.
     */
    const std::string& GetName() const;

    /**
     * @brief Gets the id.
     * @return The id.
     */
    int32_t GetId() const;

    /**
     * @brief Gets the script hash.
     * @return The script hash.
     */
    const io::UInt160& GetScriptHash() const;

    /**
     * @brief Gets the contract state.
     * @return The contract state.
     */
    const ContractState& GetContractState() const;

    /**
     * @brief Registers a method.
     * @param name The name.
     * @param handler The handler.
     * @param flags The flags.
     */
    void RegisterMethod(const std::string& name, std::function<bool(ApplicationEngine&)> handler, CallFlags flags);

    /**
     * @brief Invokes a method.
     * @param engine The engine.
     * @param method The method.
     * @return True if the method was invoked, false otherwise.
     */
    bool Invoke(ApplicationEngine& engine, const std::string& method);

    /**
     * @brief Initializes the contract.
     * @param snapshot The snapshot.
     */
    virtual void Initialize(std::shared_ptr<persistence::DataCache> snapshot) = 0;

    /**
     * @brief Gets the storage prefix.
     * @return The storage prefix.
     */
    virtual uint8_t GetStoragePrefix() const = 0;

    /**
     * @brief Creates a storage key.
     * @param prefix The prefix.
     * @param key The key.
     * @return The storage key.
     */
    persistence::StorageKey CreateStorageKey(uint8_t prefix, const io::ByteVector& key = io::ByteVector()) const;

   protected:
    std::string name_;
    int32_t id_;
    io::UInt160 scriptHash_;
    ContractState contractState_;
    std::unordered_map<std::string, std::pair<std::function<bool(ApplicationEngine&)>, CallFlags>> methods_;

    /**
     * @brief Creates the manifest.
     * @return The manifest.
     */
    virtual std::string CreateManifest() const = 0;
};

/**
 * @brief Represents a native contract manager.
 */
class NativeContractManager
{
   public:
    /**
     * @brief Gets the instance.
     * @return The instance.
     */
    static NativeContractManager& GetInstance();

    /**
     * @brief Registers a native contract.
     * @param contract The contract.
     */
    void RegisterContract(std::shared_ptr<NativeContract> contract);

    /**
     * @brief Gets a native contract by script hash.
     * @param scriptHash The script hash.
     * @return The native contract.
     */
    std::shared_ptr<NativeContract> GetContract(const io::UInt160& scriptHash) const;

    /**
     * @brief Gets a native contract by name.
     * @param name The name.
     * @return The native contract.
     */
    std::shared_ptr<NativeContract> GetContract(const std::string& name) const;

    /**
     * @brief Gets all native contracts.
     * @return The native contracts.
     */
    const std::vector<std::shared_ptr<NativeContract>>& GetContracts() const;

    /**
     * @brief Initializes all native contracts.
     * @param snapshot The snapshot.
     */
    void Initialize(std::shared_ptr<persistence::DataCache> snapshot);

   private:
    NativeContractManager();
    std::vector<std::shared_ptr<NativeContract>> contracts_;
    std::unordered_map<io::UInt160, std::shared_ptr<NativeContract>> contractsByHash_;
    std::unordered_map<std::string, std::shared_ptr<NativeContract>> contractsByName_;
};
}  // namespace neo::smartcontract
