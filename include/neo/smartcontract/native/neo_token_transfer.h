/**
 * @file neo_token_transfer.h
 * @brief NEO governance token contract
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/smartcontract/native/neo_token.h>

// Forward declaration
namespace neo::smartcontract
{
class ApplicationEngine;
}

namespace neo::smartcontract::native
{
/**
 * @brief Transfer-related methods for the NEO token.
 */
class NeoTokenTransfer
{
   public:
    /**
     * @brief Gets the total supply.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @return The total supply.
     */
    static io::Fixed8 GetTotalSupply(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Transfers NEO from one account to another.
     * @param token The NEO token.
     * @param engine The engine.
     * @param snapshot The snapshot.
     * @param from The from account.
     * @param to The to account.
     * @param amount The amount.
     * @return True if the transfer was successful, false otherwise.
     */
    static bool Transfer(const NeoToken& token, neo::smartcontract::ApplicationEngine& engine,
                         std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& from,
                         const io::UInt160& to, const io::Fixed8& amount);

    /**
     * @brief Handles the totalSupply method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnTotalSupply(const NeoToken& token,
                                                        neo::smartcontract::ApplicationEngine& engine,
                                                        const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the transfer method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnTransfer(const NeoToken& token,
                                                     neo::smartcontract::ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);
};
}  // namespace neo::smartcontract::native
