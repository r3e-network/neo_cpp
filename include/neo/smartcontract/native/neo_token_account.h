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
 * @brief Account-related methods for the NEO token.
 */
class NeoTokenAccount
{
  public:
    /**
     * @brief Gets the balance of an account.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The balance.
     */
    static io::Fixed8 GetBalance(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                 const io::UInt160& account);

    /**
     * @brief Gets the account state.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The account state.
     */
    static NeoToken::AccountState GetAccountState(const NeoToken& token,
                                                  std::shared_ptr<persistence::DataCache> snapshot,
                                                  const io::UInt160& account);

    /**
     * @brief Handles the balanceOf method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnBalanceOf(const NeoToken& token,
                                                      neo::smartcontract::ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getAccountState method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnGetAccountState(const NeoToken& token,
                                                            neo::smartcontract::ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args);
};
}  // namespace neo::smartcontract::native
