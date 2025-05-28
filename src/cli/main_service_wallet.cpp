#include <neo/cli/main_service.h>
#include <neo/cli/console_helper.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/wallets/wallet.h>

namespace neo::cli
{
    void MainService::InitializeWalletCommands()
    {
        // Wallet Commands
        RegisterCommand("openwallet", [this](const std::vector<std::string>& args) {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: path");
                return false;
            }

            std::string password;
            if (args.size() > 1)
            {
                password = args[1];
            }
            else
            {
                password = ConsoleHelper::ReadPassword("Password: ");
            }

            OnOpenWallet(args[0], password);
            return true;
        }, "Wallet");

        RegisterCommand("closewallet", [this](const std::vector<std::string>& args) {
            OnCloseWallet();
            return true;
        }, "Wallet");

        RegisterCommand("showbalance", [this](const std::vector<std::string>& args) {
            OnShowBalance();
            return true;
        }, "Wallet");
    }

    void MainService::OnOpenWallet(const std::string& path, const std::string& password)
    {
        try
        {
            // Close current wallet if open
            if (currentWallet_)
            {
                currentWallet_.reset();
            }

            // Open wallet
            currentWallet_ = wallets::Wallet::Open(path, password);

            ConsoleHelper::Info("Wallet opened: " + path);
        }
        catch (const std::exception& ex)
        {
            ConsoleHelper::Error(ex.what());
        }
    }

    void MainService::OnCloseWallet()
    {
        if (!currentWallet_)
        {
            ConsoleHelper::Error("No wallet is open");
            return;
        }

        currentWallet_.reset();
        ConsoleHelper::Info("Wallet closed");
    }

    void MainService::OnShowBalance()
    {
        if (!currentWallet_)
        {
            ConsoleHelper::Error("No wallet is open");
            return;
        }

        if (!neoSystem_)
        {
            ConsoleHelper::Error("Neo system not initialized");
            return;
        }

        try
        {
            auto accounts = currentWallet_->GetAccounts();

            for (const auto& account : accounts)
            {
                ConsoleHelper::Info("Account: " + account->GetScriptHash().ToString());

                // Get NEO balance
                auto neoToken = smartcontract::native::NeoToken::GetInstance();
                auto neoBalance = neoToken->BalanceOf(neoSystem_->GetSnapshot(), account->GetScriptHash());

                // Get GAS balance
                auto gasToken = smartcontract::native::GasToken::GetInstance();
                auto gasBalance = gasToken->BalanceOf(neoSystem_->GetSnapshot(), account->GetScriptHash());

                ConsoleHelper::Info("  NEO: " + std::to_string(neoBalance));
                ConsoleHelper::Info("  GAS: " + std::to_string(gasBalance));
            }
        }
        catch (const std::exception& ex)
        {
            ConsoleHelper::Error(ex.what());
        }
    }
}
