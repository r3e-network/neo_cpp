#pragma once

#include <neo/io/serializable.h>
#include <neo/io/uint160.h>
#include <neo/vm/execution_engine_limits.h>
#include <string>
#include <vector>

namespace neo::smartcontract::manifest
{
    // Forward declarations
    class ContractAbi;
    class ContractPermission;
    class ContractPermissionDescriptor;

    /**
     * @brief Represents a contract manifest.
     * 
     * When a smart contract is deployed, it must explicitly declare the features and permissions it will use.
     * When it is running, it will be limited by its declared list of features and permissions, and cannot make any behavior beyond the scope of the list.
     */
    class ContractManifest : public io::ISerializable
    {
    public:
        /**
         * @brief The maximum length of a manifest.
         */
        static constexpr uint32_t MaxLength = 65535;

        /**
         * @brief Constructs a ContractManifest.
         */
        ContractManifest();

        /**
         * @brief Destructor.
         */
        ~ContractManifest();

        /**
         * @brief Gets the name of the contract.
         * @return The name.
         */
        const std::string& GetName() const;

        /**
         * @brief Sets the name of the contract.
         * @param name The name.
         */
        void SetName(const std::string& name);

        /**
         * @brief Gets the supported standards.
         * @return The supported standards.
         */
        const std::vector<std::string>& GetSupportedStandards() const;

        /**
         * @brief Sets the supported standards.
         * @param supportedStandards The supported standards.
         */
        void SetSupportedStandards(const std::vector<std::string>& supportedStandards);

        /**
         * @brief Gets the ABI.
         * @return The ABI.
         */
        const ContractAbi& GetAbi() const;

        /**
         * @brief Sets the ABI.
         * @param abi The ABI.
         */
        void SetAbi(const ContractAbi& abi);

        /**
         * @brief Gets the permissions.
         * @return The permissions.
         */
        const std::vector<ContractPermission>& GetPermissions() const;

        /**
         * @brief Sets the permissions.
         * @param permissions The permissions.
         */
        void SetPermissions(const std::vector<ContractPermission>& permissions);

        /**
         * @brief Gets the trusts.
         * @return The trusts.
         */
        const std::vector<ContractPermissionDescriptor>& GetTrusts() const;

        /**
         * @brief Sets the trusts.
         * @param trusts The trusts.
         */
        void SetTrusts(const std::vector<ContractPermissionDescriptor>& trusts);

        /**
         * @brief Gets the extra data.
         * @return The extra data.
         */
        const std::string& GetExtra() const;

        /**
         * @brief Sets the extra data.
         * @param extra The extra data.
         */
        void SetExtra(const std::string& extra);

        /**
         * @brief Determines whether the manifest is valid.
         * @param limits The execution engine limits.
         * @param hash The hash of the contract.
         * @return True if the manifest is valid, false otherwise.
         */
        bool IsValid(const vm::ExecutionEngineLimits& limits, const io::UInt160& hash) const;

        /**
         * @brief Parses a manifest from a JSON string.
         * @param json The JSON string.
         * @return The manifest.
         */
        static ContractManifest Parse(const std::string& json);

        /**
         * @brief Converts the manifest to a JSON string.
         * @return The JSON string.
         */
        std::string ToJson() const;

        /**
         * @brief Serializes the object.
         * @param writer The writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the object.
         * @param reader The reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

    private:
        std::string name_;
        std::vector<std::string> supportedStandards_;
        ContractAbi* abi_;
        std::vector<ContractPermission> permissions_;
        std::vector<ContractPermissionDescriptor> trusts_;
        std::string extra_;
    };
}
