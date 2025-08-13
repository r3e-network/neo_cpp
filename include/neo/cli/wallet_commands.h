/**
 * @file wallet_commands.h
 * @brief Wallet Commands
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cli/main_service.h>

#include <string>
#include <vector>

namespace neo::cli
{
/**
 * @brief Wallet commands for the CLI.
 */
class WalletCommands
{
   public:
    /**
     * @brief Constructs a WalletCommands.
     * @param service The main service.
     */
    explicit WalletCommands(MainService& service);

    /**
     * @brief Registers the commands.
     */
    void RegisterCommands();

    /**
     * @brief Handles the openwallet command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleOpenWallet(const std::vector<std::string>& args);

    /**
     * @brief Handles the closewallet command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleCloseWallet(const std::vector<std::string>& args);

    /**
     * @brief Handles the showbalance command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleShowBalance(const std::vector<std::string>& args);

    /**
     * @brief Handles the showaddress command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleShowAddress(const std::vector<std::string>& args);

    /**
     * @brief Handles the transfer command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleTransfer(const std::vector<std::string>& args);

   private:
    MainService& service_;
};
}  // namespace neo::cli
