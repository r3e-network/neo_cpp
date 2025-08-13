/**
 * @file rpc_methods.h
 * @brief Rpc Methods
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/json.h>
#include <neo/node/neo_system.h>

#include <memory>
#include <string>

namespace neo::rpc
{
/**
 * @brief Provides RPC methods.
 */
class RPCMethods
{
   public:
    /**
     * @brief Gets the version.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetVersion(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets the block count.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetBlockCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets a block.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets a block hash.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets a block header.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetBlockHeader(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets the raw memory pool.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetRawMemPool(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets a raw transaction.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets a transaction height.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetTransactionHeight(std::shared_ptr<node::NeoSystem> neoSystem,
                                               const nlohmann::json& params);

    /**
     * @brief Sends a raw transaction.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json SendRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Invokes a function.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json InvokeFunction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Invokes a script.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json InvokeScript(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets a contract state.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetContractState(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets unclaimed gas.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetUnclaimedGas(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets the connection count.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetConnectionCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets the peers.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetPeers(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets the committee.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetCommittee(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets the validators.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetValidators(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets the next block validators.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetNextBlockValidators(std::shared_ptr<node::NeoSystem> neoSystem,
                                                 const nlohmann::json& params);

    /**
     * @brief Gets the best block hash.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetBestBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets the block header count.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetBlockHeaderCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets storage items.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Finds storage items.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json FindStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets validator candidates.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetCandidates(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Gets native contracts.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json GetNativeContracts(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Submits a block.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json SubmitBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Validates an address.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json ValidateAddress(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Traverses an iterator.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json TraverseIterator(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Terminates a session.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json TerminateSession(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params);

    /**
     * @brief Invokes contract verification.
     * @param neoSystem The Neo system.
     * @param params The parameters.
     * @return The result.
     */
    static nlohmann::json InvokeContractVerify(std::shared_ptr<node::NeoSystem> neoSystem,
                                               const nlohmann::json& params);

   private:
    /**
     * @brief Converts a block to JSON.
     * @param block The block.
     * @param verbose Whether to include verbose information.
     * @return The JSON.
     */
    static nlohmann::json BlockToJson(std::shared_ptr<ledger::Block> block, bool verbose);

    /**
     * @brief Converts a transaction to JSON.
     * @param tx The transaction.
     * @param verbose Whether to include verbose information.
     * @return The JSON.
     */
    static nlohmann::json TransactionToJson(std::shared_ptr<ledger::Transaction> tx, bool verbose);

    /**
     * @brief Converts a contract to JSON.
     * @param contract The contract.
     * @return The JSON.
     */
    static nlohmann::json ContractToJson(std::shared_ptr<smartcontract::ContractState> contract);
};
}  // namespace neo::rpc
