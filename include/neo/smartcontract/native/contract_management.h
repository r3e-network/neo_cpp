#pragma once

#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/io/uint160.h>
#include <neo/io/byte_vector.h>
#include <memory>
#include <string>

namespace neo::smartcontract::native
{
    /**
     * @brief Represents the contract management native contract.
     */
    class ContractManagement : public NativeContract
    {
        // Friend classes for testing
        friend class NativeContractTest;
        friend class ContractManagementTest;
        
    public:
        /**
         * @brief The contract ID.
         */
        static constexpr int32_t ID = -1;

        /**
         * @brief The contract name.
         */
        static constexpr const char* NAME = "ContractManagement";

        /**
         * @brief The storage prefix for contracts.
         */
        static constexpr uint8_t PREFIX_CONTRACT = 8;

        /**
         * @brief The storage prefix for contract hash.
         */
        static constexpr uint8_t PREFIX_CONTRACT_HASH = 12;

        /**
         * @brief The storage prefix for next available ID.
         */
        static constexpr uint8_t PREFIX_NEXT_AVAILABLE_ID = 15;

        /**
         * @brief The storage prefix for minimum deployment fee.
         */
        static constexpr uint8_t PREFIX_MINIMUM_DEPLOYMENT_FEE = 20;

        /**
         * @brief The event ID for Deploy.
         */
        static constexpr uint32_t EVENT_DEPLOY = 0;

        /**
         * @brief The event ID for Update.
         */
        static constexpr uint32_t EVENT_UPDATE = 1;

        /**
         * @brief The event ID for Destroy.
         */
        static constexpr uint32_t EVENT_DESTROY = 2;

        /**
         * @brief Constructs a ContractManagement.
         */
        ContractManagement();

        /**
         * @brief Gets a contract.
         * @param snapshot The snapshot.
         * @param hash The hash.
         * @return The contract.
         */
        std::shared_ptr<ContractState> GetContract(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& hash) const;

        /**
         * @brief Gets a contract (overload for DataCache).
         * @param snapshot The snapshot.
         * @param hash The hash.
         * @return The contract.
         */
        static std::shared_ptr<ContractState> GetContract(const persistence::DataCache& snapshot, const io::UInt160& hash);

        /**
         * @brief Checks if a method exists in a contract.
         * @param snapshot The snapshot.
         * @param hash The hash.
         * @param method The method.
         * @param parameterCount The parameter count.
         * @return True if the method exists, false otherwise.
         */
        bool HasMethod(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& hash, const std::string& method, int parameterCount) const;

        /**
         * @brief Lists all contracts.
         * @param snapshot The snapshot.
         * @return The contracts.
         */
        std::vector<std::shared_ptr<ContractState>> ListContracts(std::shared_ptr<persistence::StoreView> snapshot) const;

        /**
         * @brief Creates a contract.
         * @param snapshot The snapshot.
         * @param script The script.
         * @param manifest The manifest.
         * @param hash The hash.
         * @return The contract.
         */
        std::shared_ptr<ContractState> CreateContract(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& script, const std::string& manifest, const io::UInt160& hash) const;

        /**
         * @brief Updates a contract.
         * @param snapshot The snapshot.
         * @param hash The hash.
         * @param script The script.
         * @param manifest The manifest.
         * @return The contract.
         */
        std::shared_ptr<ContractState> UpdateContract(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& hash, const io::ByteVector& script, const std::string& manifest) const;

        /**
         * @brief Destroys a contract.
         * @param snapshot The snapshot.
         * @param hash The hash.
         */
        void DestroyContract(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& hash) const;

        /**
         * @brief Gets the minimum deployment fee.
         * @param snapshot The snapshot.
         * @return The minimum deployment fee.
         */
        int64_t GetMinimumDeploymentFee(std::shared_ptr<persistence::StoreView> snapshot) const;

        /**
         * @brief Sets the minimum deployment fee.
         * @param snapshot The snapshot.
         * @param fee The fee.
         */
        void SetMinimumDeploymentFee(std::shared_ptr<persistence::StoreView> snapshot, int64_t fee) const;

        /**
         * @brief Gets the instance.
         * @return The instance.
         */
        static std::shared_ptr<ContractManagement> GetInstance();

        /**
         * @brief Initializes the contract when it's first deployed.
         * @param engine The application engine.
         * @param hardfork The hardfork version.
         * @return True if the initialization was successful, false otherwise.
         */
        bool InitializeContract(ApplicationEngine& engine, uint32_t hardfork);

        /**
         * @brief Called after a contract is deployed.
         * @param engine The application engine.
         * @param contract The contract.
         * @param data Additional data to pass to the contract.
         * @param update Whether this is an update or a new deployment.
         * @return True if the post-deploy was successful, false otherwise.
         */
        bool OnDeploy(ApplicationEngine& engine, std::shared_ptr<ContractState> contract, std::shared_ptr<vm::StackItem> data, bool update);

    protected:
        /**
         * @brief Initializes the contract.
         */
        void Initialize() override;

    private:
        /**
         * @brief Gets the next available ID.
         * @param snapshot The snapshot.
         * @return The ID.
         */
        uint32_t GetNextId(std::shared_ptr<persistence::StoreView> snapshot) const;

        /**
         * @brief Handles the OnPersist event.
         * @param engine The engine.
         * @return True if successful, false otherwise.
         */
        bool OnPersist(ApplicationEngine& engine);

        /**
         * @brief Handles the getMinimumDeploymentFee method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnGetMinimumDeploymentFee(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the setMinimumDeploymentFee method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnSetMinimumDeploymentFee(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the deploy method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnDeploy(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the update method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnUpdate(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the destroy method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnDestroy(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the getContract method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnGetContract(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the hasMethod method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnHasMethod(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the listContracts method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnListContracts(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);
        
        /**
         * @brief Handles the getContractById method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnGetContractById(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);
        
        /**
         * @brief Handles the getContractHashes method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnGetContractHashes(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);
        
        /**
         * @brief Gets committee members from NEO token contract
         * @param snapshot The blockchain snapshot
         * @return Vector of committee member public keys
         */
        std::vector<cryptography::ecc::ECPoint> GetCommitteeFromNeoContract(const std::shared_ptr<persistence::DataCache>& snapshot);
        
        /**
         * @brief Calculates committee multi-signature address
         * @param committee Vector of committee member public keys
         * @return The committee script hash address
         */
        io::UInt160 CalculateCommitteeAddress(const std::vector<cryptography::ecc::ECPoint>& committee);
        
        /**
         * @brief Gets script hash from public key
         * @param publicKey The public key
         * @return The script hash for the public key
         */
        io::UInt160 GetScriptHashFromPublicKey(const cryptography::ecc::ECPoint& publicKey);
    };
}
