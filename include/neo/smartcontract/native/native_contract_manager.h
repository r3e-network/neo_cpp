#pragma once

#include <neo/io/uint160.h>
#include <neo/smartcontract/native/native_contract.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace neo::smartcontract::native
{
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
     * @brief Gets the contracts.
     * @return The contracts.
     */
    const std::vector<std::shared_ptr<NativeContract>>& GetContracts() const;

    /**
     * @brief Gets a contract by name.
     * @param name The name.
     * @return The contract.
     */
    std::shared_ptr<NativeContract> GetContract(const std::string& name) const;

    /**
     * @brief Gets a contract by script hash.
     * @param scriptHash The script hash.
     * @return The contract.
     */
    std::shared_ptr<NativeContract> GetContract(const io::UInt160& scriptHash) const;

    /**
     * @brief Gets a contract by ID.
     * @param id The ID.
     * @return The contract.
     */
    std::shared_ptr<NativeContract> GetContract(uint32_t id) const;

    /**
     * @brief Registers a contract.
     * @param contract The contract.
     */
    void RegisterContract(std::shared_ptr<NativeContract> contract);

    /**
     * @brief Initializes the contracts.
     */
    void Initialize();

   private:
    NativeContractManager();
    std::vector<std::shared_ptr<NativeContract>> contracts_;
    std::unordered_map<std::string, std::shared_ptr<NativeContract>> contractsByName_;
    std::unordered_map<io::UInt160, std::shared_ptr<NativeContract>> contractsByScriptHash_;
    std::unordered_map<uint32_t, std::shared_ptr<NativeContract>> contractsById_;
};
}  // namespace neo::smartcontract::native
