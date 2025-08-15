#pragma once

/**
 * @file contract_invoker.h
 * @brief Smart contract invocation utilities for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 */

#include <neo/sdk/core/types.h>
#include <neo/sdk/wallet/wallet.h>
#include <neo/sdk/rpc/rpc_client.h>
#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace neo::sdk::contract {

/**
 * @brief Contract parameter types
 */
enum class ContractParameterType {
    Signature = 0x00,
    Boolean = 0x01,
    Integer = 0x02,
    Hash160 = 0x03,
    Hash256 = 0x04,
    ByteArray = 0x05,
    PublicKey = 0x06,
    String = 0x07,
    Array = 0x10,
    Map = 0x12,
    InteropInterface = 0x30,
    Any = 0xfe,
    Void = 0xff
};

/**
 * @brief Contract parameter for invocation
 */
class ContractParameter {
public:
    using Value = std::variant<
        bool,
        int64_t,
        std::string,
        std::vector<uint8_t>,
        core::UInt160,
        core::UInt256,
        std::vector<ContractParameter>
    >;

    ContractParameterType type;
    Value value;

    /**
     * @brief Create parameter from boolean
     */
    static ContractParameter FromBoolean(bool value);

    /**
     * @brief Create parameter from integer
     */
    static ContractParameter FromInteger(int64_t value);

    /**
     * @brief Create parameter from string
     */
    static ContractParameter FromString(const std::string& value);

    /**
     * @brief Create parameter from byte array
     */
    static ContractParameter FromByteArray(const std::vector<uint8_t>& value);

    /**
     * @brief Create parameter from address
     */
    static ContractParameter FromAddress(const std::string& address);

    /**
     * @brief Create parameter from script hash
     */
    static ContractParameter FromScriptHash(const core::UInt160& hash);

    /**
     * @brief Create array parameter
     */
    static ContractParameter FromArray(const std::vector<ContractParameter>& values);

    /**
     * @brief Convert to JSON representation
     */
    std::string ToJson() const;
};

/**
 * @brief Smart contract invocation configuration
 */
struct InvocationConfig {
    uint64_t networkFee = 1000000;  // 0.01 GAS default
    uint64_t systemFee = 0;  // Auto-calculated
    uint32_t validUntilBlock = 0;  // Auto-calculated
    std::vector<core::Signer> signers;
    std::vector<core::TransactionAttribute> attributes;
};

/**
 * @brief Smart contract invoker for Neo blockchain
 */
class ContractInvoker {
public:
    /**
     * @brief Constructor with RPC client
     * @param client RPC client for blockchain interaction
     */
    explicit ContractInvoker(std::shared_ptr<rpc::RpcClient> client);

    /**
     * @brief Test invoke a contract method (read-only, no blockchain state change)
     * @param contractHash Contract script hash
     * @param method Method name to invoke
     * @param params Method parameters
     * @return Invocation result with stack and gas consumed
     */
    core::InvocationResult TestInvoke(
        const core::UInt160& contractHash,
        const std::string& method,
        const std::vector<ContractParameter>& params = {}
    );

    /**
     * @brief Invoke a contract method (writes to blockchain)
     * @param contractHash Contract script hash
     * @param method Method name to invoke
     * @param params Method parameters
     * @param wallet Wallet for signing transaction
     * @param config Optional invocation configuration
     * @return Transaction ID
     */
    core::UInt256 Invoke(
        const core::UInt160& contractHash,
        const std::string& method,
        const std::vector<ContractParameter>& params,
        wallet::Wallet& wallet,
        const InvocationConfig& config = {}
    );

    /**
     * @brief Invoke multiple contract methods in a single transaction
     * @param invocations Vector of contract invocations
     * @param wallet Wallet for signing
     * @param config Optional configuration
     * @return Transaction ID
     */
    core::UInt256 MultiInvoke(
        const std::vector<std::tuple<core::UInt160, std::string, std::vector<ContractParameter>>>& invocations,
        wallet::Wallet& wallet,
        const InvocationConfig& config = {}
    );

    /**
     * @brief Test invoke with custom script
     * @param script NEO VM script to execute
     * @param signers Transaction signers
     * @return Invocation result
     */
    core::InvocationResult TestInvokeScript(
        const std::vector<uint8_t>& script,
        const std::vector<core::Signer>& signers = {}
    );

    /**
     * @brief Calculate invocation costs
     * @param contractHash Contract to invoke
     * @param method Method name
     * @param params Parameters
     * @return System fee and network fee
     */
    std::pair<uint64_t, uint64_t> CalculateInvocationCost(
        const core::UInt160& contractHash,
        const std::string& method,
        const std::vector<ContractParameter>& params
    );

    /**
     * @brief Get contract state
     * @param contractHash Contract script hash
     * @return Contract state information
     */
    core::ContractState GetContractState(const core::UInt160& contractHash);

    /**
     * @brief Build invocation script
     * @param contractHash Contract hash
     * @param method Method name
     * @param params Parameters
     * @return NEO VM script bytes
     */
    static std::vector<uint8_t> BuildInvocationScript(
        const core::UInt160& contractHash,
        const std::string& method,
        const std::vector<ContractParameter>& params
    );

private:
    std::shared_ptr<rpc::RpcClient> client_;
};

} // namespace neo::sdk::contract