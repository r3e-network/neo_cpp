/**
 * @file neo_token_gas.h
 * @brief NEO governance token contract
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/smartcontract/native/neo_token.h>

namespace neo::smartcontract::native
{
/**
 * @brief Gas-related methods for the NEO token.
 */
class NeoTokenGas
{
   public:
    /**
     * @brief Gets the gas per block.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @return The gas per block.
     */
    static int64_t GetGasPerBlock(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Sets the gas per block.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param gasPerBlock The gas per block.
     */
    static void SetGasPerBlock(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                               int64_t gasPerBlock);

    /**
     * @brief Gets the unclaimed gas for an account.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param account The account.
     * @param end The block index used when calculating GAS.
     * @return The unclaimed gas.
     */
    static int64_t GetUnclaimedGas(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                   const io::UInt160& account, uint32_t end);

    /**
     * @brief Distributes gas to an account.
     * @param token The NEO token.
     * @param engine The engine.
     * @param account The account.
     * @param state The account state.
     * @return The gas distribution.
     */
    static NeoToken::GasDistribution DistributeGas(const NeoToken& token, ApplicationEngine& engine,
                                                   const io::UInt160& account, const NeoToken::AccountState& state);

    /**
     * @brief Calculates the bonus for an account.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param state The account state.
     * @param end The block index.
     * @return The bonus.
     */
    static int64_t CalculateBonus(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                  const NeoToken::AccountState& state, uint32_t end);

    /**
     * @brief Calculates the NEO holder reward.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param value The value.
     * @param start The start block index.
     * @param end The end block index.
     * @return The reward.
     */
    static int64_t CalculateNeoHolderReward(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                            int64_t value, uint32_t start, uint32_t end);

    /**
     * @brief Handles the getGasPerBlock method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnGetGasPerBlock(const NeoToken& token, ApplicationEngine& engine,
                                                           const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setGasPerBlock method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnSetGasPerBlock(const NeoToken& token, ApplicationEngine& engine,
                                                           const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getUnclaimedGas method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnGetUnclaimedGas(const NeoToken& token, ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args);
};
}  // namespace neo::smartcontract::native
