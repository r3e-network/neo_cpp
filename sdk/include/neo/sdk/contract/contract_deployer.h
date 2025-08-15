#pragma once

/**
 * @file contract_deployer.h
 * @brief Smart contract deployment utilities for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 */

#include <neo/sdk/core/types.h>
#include <neo/sdk/wallet/wallet.h>
#include <neo/sdk/rpc/rpc_client.h>
#include <memory>
#include <string>
#include <vector>

namespace neo::sdk::contract {

/**
 * @brief Smart contract manifest structure
 */
struct ContractManifest {
    std::string name;
    std::vector<std::string> groups;
    std::vector<std::string> supportedStandards;
    struct ABI {
        std::vector<std::string> methods;
        std::vector<std::string> events;
    } abi;
    std::vector<std::string> permissions;
    std::vector<std::string> trusts;
    std::string extra;

    /**
     * @brief Convert manifest to JSON string
     */
    std::string ToJson() const;
};

/**
 * @brief NEF (Neo Executable Format) file structure
 */
struct NefFile {
    std::string magic;  // "NEF3"
    std::string compiler;
    std::string version;
    std::vector<uint8_t> script;
    uint32_t checksum;

    /**
     * @brief Load NEF file from path
     */
    static NefFile LoadFromFile(const std::string& path);

    /**
     * @brief Serialize to byte array
     */
    std::vector<uint8_t> Serialize() const;
};

/**
 * @brief Smart contract deployment configuration
 */
struct DeploymentConfig {
    uint64_t networkFee = 1000000000;  // 10 GAS default
    uint64_t systemFee = 0;  // Auto-calculated
    uint32_t validUntilBlock = 0;  // Auto-calculated
    std::vector<core::Signer> signers;
    std::vector<core::TransactionAttribute> attributes;
};

/**
 * @brief Smart contract deployer for Neo blockchain
 */
class ContractDeployer {
public:
    /**
     * @brief Constructor with RPC client
     * @param client RPC client for blockchain interaction
     */
    explicit ContractDeployer(std::shared_ptr<rpc::RpcClient> client);

    /**
     * @brief Deploy a smart contract to the blockchain
     * @param nef NEF file containing contract bytecode
     * @param manifest Contract manifest with metadata
     * @param wallet Wallet for signing the deployment
     * @param config Optional deployment configuration
     * @return Contract hash after deployment
     */
    core::UInt160 Deploy(
        const NefFile& nef,
        const ContractManifest& manifest,
        wallet::Wallet& wallet,
        const DeploymentConfig& config = {}
    );

    /**
     * @brief Update an existing smart contract
     * @param contractHash Hash of the contract to update
     * @param nef New NEF file (optional)
     * @param manifest New manifest (optional)
     * @param wallet Wallet for signing
     * @return Transaction ID
     */
    core::UInt256 Update(
        const core::UInt160& contractHash,
        const std::optional<NefFile>& nef,
        const std::optional<ContractManifest>& manifest,
        wallet::Wallet& wallet
    );

    /**
     * @brief Destroy a smart contract
     * @param contractHash Hash of the contract to destroy
     * @param wallet Wallet for signing
     * @return Transaction ID
     */
    core::UInt256 Destroy(
        const core::UInt160& contractHash,
        wallet::Wallet& wallet
    );

    /**
     * @brief Calculate deployment costs
     * @param nef NEF file to deploy
     * @param manifest Contract manifest
     * @return System fee and network fee
     */
    std::pair<uint64_t, uint64_t> CalculateDeploymentCost(
        const NefFile& nef,
        const ContractManifest& manifest
    );

    /**
     * @brief Test deploy without actually deploying
     * @param nef NEF file to test
     * @param manifest Contract manifest
     * @return Test invocation result
     */
    core::InvocationResult TestDeploy(
        const NefFile& nef,
        const ContractManifest& manifest
    );

private:
    std::shared_ptr<rpc::RpcClient> client_;
};

} // namespace neo::sdk::contract