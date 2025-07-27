#pragma once

#include <functional>
#include <memory>
#include <neo/node/neo_system.h>
#include <neo/rpc/rpc_server.h>
#include <neo/wallets/wallet.h>
#include <string>
#include <vector>

namespace neo::cli
{
/**
 * @brief Handles CLI commands.
 */
class CommandHandler
{
  public:
    /**
     * @brief Constructs a CommandHandler.
     * @param neoSystem The Neo system.
     * @param rpcServer The RPC server.
     */
    CommandHandler(std::shared_ptr<node::NeoSystem> neoSystem, std::shared_ptr<rpc::RPCServer> rpcServer);

    /**
     * @brief Sets the wallet.
     * @param wallet The wallet.
     */
    void SetWallet(std::shared_ptr<wallets::Wallet> wallet);

    /**
     * @brief Gets the wallet.
     * @return The wallet.
     */
    std::shared_ptr<wallets::Wallet> GetWallet() const;

    /**
     * @brief Handles the help command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleHelp(const std::vector<std::string>& args);

    /**
     * @brief Handles the exit command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleExit(const std::vector<std::string>& args);

    /**
     * @brief Handles the clear command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleClear(const std::vector<std::string>& args);

    /**
     * @brief Handles the version command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleVersion(const std::vector<std::string>& args);

    /**
     * @brief Handles the show state command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleShowState(const std::vector<std::string>& args);

    /**
     * @brief Handles the show node command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleShowNode(const std::vector<std::string>& args);

    /**
     * @brief Handles the show pool command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleShowPool(const std::vector<std::string>& args);

    /**
     * @brief Handles the open wallet command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleOpenWallet(const std::vector<std::string>& args);

    /**
     * @brief Handles the close wallet command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleCloseWallet(const std::vector<std::string>& args);

    /**
     * @brief Handles the create wallet command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleCreateWallet(const std::vector<std::string>& args);

    /**
     * @brief Handles the import key command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleImportKey(const std::vector<std::string>& args);

    /**
     * @brief Handles the export key command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleExportKey(const std::vector<std::string>& args);

    /**
     * @brief Handles the list address command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleListAddress(const std::vector<std::string>& args);

    /**
     * @brief Handles the list asset command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleListAsset(const std::vector<std::string>& args);

    /**
     * @brief Handles the transfer command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleTransfer(const std::vector<std::string>& args);

    /**
     * @brief Handles the claim gas command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleClaimGas(const std::vector<std::string>& args);

    /**
     * @brief Handles the send command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleSend(const std::vector<std::string>& args);

    /**
     * @brief Handles the deploy command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleDeploy(const std::vector<std::string>& args);

    /**
     * @brief Handles the invoke command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleInvoke(const std::vector<std::string>& args);

    /**
     * @brief Handles the import NEP2 command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleImportNEP2(const std::vector<std::string>& args);

  private:
    std::shared_ptr<node::NeoSystem> neoSystem_;
    std::shared_ptr<rpc::RPCServer> rpcServer_;
    std::shared_ptr<wallets::Wallet> wallet_;
    std::unordered_map<std::string, std::string> commandHelp_;

    /**
     * @brief Initializes the command help.
     */
    void InitializeCommandHelp();
};
}  // namespace neo::cli
