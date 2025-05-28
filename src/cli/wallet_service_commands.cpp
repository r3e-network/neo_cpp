#include <neo/cli/wallet_commands.h>
#include <neo/cli/console_helper.h>
#include <neo/io/uint160.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/gas_token.h>
#include <iostream>

namespace neo::cli
{
    WalletCommands::WalletCommands(MainService& service)
        : service_(service)
    {
    }

    void WalletCommands::RegisterCommands()
    {
        service_.RegisterCommand("openwallet", [this](const std::vector<std::string>& args) {
            return HandleOpenWallet(args);
        }, "Wallet");

        service_.RegisterCommand("closewallet", [this](const std::vector<std::string>& args) {
            return HandleCloseWallet(args);
        }, "Wallet");

        service_.RegisterCommand("showbalance", [this](const std::vector<std::string>& args) {
            return HandleShowBalance(args);
        }, "Wallet");

        service_.RegisterCommand("showaddress", [this](const std::vector<std::string>& args) {
            return HandleShowAddress(args);
        }, "Wallet");

        service_.RegisterCommand("transfer", [this](const std::vector<std::string>& args) {
            return HandleTransfer(args);
        }, "Wallet");
    }

    bool WalletCommands::HandleOpenWallet(const std::vector<std::string>& args)
    {
        if (args.empty())
        {
            ConsoleHelper::Error("Missing argument: path");
            return false;
        }

        std::string path = args[0];
        std::string password;

        if (args.size() >= 2)
        {
            password = args[1];
        }
        else
        {
            ConsoleHelper::Info("Enter password:");
            password = ConsoleHelper::ReadPassword();
        }

        service_.OnOpenWallet(path, password);
        return true;
    }

    bool WalletCommands::HandleCloseWallet(const std::vector<std::string>& args)
    {
        service_.OnCloseWallet();
        return true;
    }

    bool WalletCommands::HandleShowBalance(const std::vector<std::string>& args)
    {
        if (!service_.HasWallet())
        {
            ConsoleHelper::Error("No wallet is open");
            return false;
        }

        std::string asset;
        if (!args.empty())
        {
            asset = args[0];
        }

        if (asset.empty())
        {
            // Show all assets
            service_.OnShowBalance();
        }
        else
        {
            // Show specific asset
            io::UInt160 assetId;

            if (asset == "neo" || asset == "NEO")
            {
                assetId = smartcontract::native::NeoToken::SCRIPT_HASH;
            }
            else if (asset == "gas" || asset == "GAS")
            {
                assetId = smartcontract::native::GasToken::SCRIPT_HASH;
            }
            else
            {
                try
                {
                    assetId = io::UInt160::Parse(asset);
                }
                catch (const std::exception&)
                {
                    ConsoleHelper::Error("Invalid asset ID");
                    return false;
                }
            }

            service_.OnShowBalance(assetId);
        }

        return true;
    }

    bool WalletCommands::HandleShowAddress(const std::vector<std::string>& args)
    {
        if (!service_.HasWallet())
        {
            ConsoleHelper::Error("No wallet is open");
            return false;
        }

        service_.OnShowAddress();
        return true;
    }

    bool WalletCommands::HandleTransfer(const std::vector<std::string>& args)
    {
        if (!service_.HasWallet())
        {
            ConsoleHelper::Error("No wallet is open");
            return false;
        }

        if (args.size() < 3)
        {
            ConsoleHelper::Error("Usage: transfer <asset> <address> <amount>");
            return false;
        }

        std::string asset = args[0];
        std::string address = args[1];
        std::string amountStr = args[2];

        // Convert asset to script hash
        io::UInt160 assetId;

        if (asset == "neo" || asset == "NEO")
        {
            assetId = smartcontract::native::NeoToken::SCRIPT_HASH;
        }
        else if (asset == "gas" || asset == "GAS")
        {
            assetId = smartcontract::native::GasToken::SCRIPT_HASH;
        }
        else
        {
            try
            {
                assetId = io::UInt160::Parse(asset);
            }
            catch (const std::exception&)
            {
                ConsoleHelper::Error("Invalid asset ID");
                return false;
            }
        }

        // Parse amount
        double amount;
        try
        {
            amount = std::stod(amountStr);
        }
        catch (const std::exception&)
        {
            ConsoleHelper::Error("Invalid amount");
            return false;
        }

        service_.OnTransfer(assetId, address, amount);
        return true;
    }
}
