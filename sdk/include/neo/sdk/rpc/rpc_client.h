#pragma once

#include <neo/sdk/core/types.h>
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace neo::sdk::rpc {

using json = nlohmann::json;

/**
 * @brief RPC client for communicating with Neo nodes
 */
class RpcClient {
public:
    /**
     * @brief Construct RPC client with endpoint
     * @param endpoint The RPC endpoint URL (e.g., "http://localhost:30332")
     */
    explicit RpcClient(const std::string& endpoint);
    ~RpcClient();
    
    // Node information
    
    /**
     * @brief Get node version information
     */
    std::string GetVersion();
    
    /**
     * @brief Get current block count
     */
    uint32_t GetBlockCount();
    
    /**
     * @brief Get best block hash
     */
    std::string GetBestBlockHash();
    
    /**
     * @brief Get connection count
     */
    uint32_t GetConnectionCount();
    
    // Block queries
    
    /**
     * @brief Get block by hash
     */
    json GetBlock(const std::string& hash, bool verbose = true);
    
    /**
     * @brief Get block by index
     */
    json GetBlock(uint32_t index, bool verbose = true);
    
    /**
     * @brief Get block header by hash
     */
    json GetBlockHeader(const std::string& hash, bool verbose = true);
    
    /**
     * @brief Get block header by index
     */
    json GetBlockHeader(uint32_t index, bool verbose = true);
    
    // Transaction operations
    
    /**
     * @brief Get raw transaction
     */
    json GetRawTransaction(const std::string& txid, bool verbose = true);
    
    /**
     * @brief Send raw transaction
     */
    std::string SendRawTransaction(const std::string& hex);
    
    /**
     * @brief Get transaction height
     */
    uint32_t GetTransactionHeight(const std::string& txid);
    
    // Contract operations
    
    /**
     * @brief Invoke contract function (test invocation)
     */
    json InvokeFunction(
        const std::string& scriptHash,
        const std::string& method,
        const std::vector<json>& params = {}
    );
    
    /**
     * @brief Invoke script (test invocation)
     */
    json InvokeScript(const std::string& script);
    
    /**
     * @brief Get contract state
     */
    json GetContractState(const std::string& scriptHash);
    
    // State queries
    
    /**
     * @brief Get NEP-17 token balances for address
     */
    json GetNep17Balances(const std::string& address);
    
    /**
     * @brief Get NEP-17 transfers for address
     */
    json GetNep17Transfers(const std::string& address, uint64_t startTime = 0, uint64_t endTime = 0);
    
    /**
     * @brief Get storage value
     */
    json GetStorage(const std::string& contractHash, const std::string& key);
    
    /**
     * @brief Find storage values
     */
    json FindStorage(const std::string& contractHash, const std::string& prefix);
    
    // Account operations
    
    /**
     * @brief Get account state
     */
    json GetAccountState(const std::string& address);
    
    /**
     * @brief Validate address
     */
    bool ValidateAddress(const std::string& address);
    
    /**
     * @brief Get unclaimed GAS
     */
    uint64_t GetUnclaimedGas(const std::string& address);
    
    // Utility methods
    
    /**
     * @brief List available RPC methods
     */
    std::vector<std::string> ListMethods();
    
    /**
     * @brief Calculate network fee for transaction
     */
    uint64_t CalculateNetworkFee(const std::string& tx);
    
    /**
     * @brief Get application log for transaction
     */
    json GetApplicationLog(const std::string& txid);
    
    /**
     * @brief Get state root by height
     */
    json GetStateRoot(uint32_t height);
    
    /**
     * @brief Get proof for key
     */
    json GetProof(const std::string& rootHash, const std::string& contractHash, const std::string& key);
    
    // Custom RPC call
    
    /**
     * @brief Make custom RPC call
     */
    json Call(const std::string& method, const std::vector<json>& params = {});
    
    // Configuration
    
    /**
     * @brief Set request timeout (milliseconds)
     */
    void SetTimeout(uint32_t timeoutMs);
    
    /**
     * @brief Get current endpoint
     */
    std::string GetEndpoint() const;
    
    /**
     * @brief Test connection to RPC endpoint
     */
    bool TestConnection();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace neo::sdk::rpc