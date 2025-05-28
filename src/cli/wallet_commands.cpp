#include <neo/cli/command_handler.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/wallets/wallet_factory.h>
#include <neo/wallets/nep6/nep6_wallet.h>
#include <iostream>

namespace neo::cli
{
    bool CommandHandler::HandleOpenWallet(const std::vector<std::string>& args)
    {
        if (args.size() < 1)
        {
            std::cout << "Usage: open wallet <path> [password]" << std::endl;
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
            std::cout << "Password: ";
            std::getline(std::cin, password);
        }

        try
        {
            wallet_ = wallets::WalletFactoryManager::GetInstance().OpenWallet(path, password);
            std::cout << "Wallet opened successfully" << std::endl;

            // Show wallet info
            auto neoBalance = wallet_->GetBalance(smartcontract::native::NeoToken::SCRIPT_HASH);
            auto gasBalance = wallet_->GetBalance(smartcontract::native::GasToken::SCRIPT_HASH);

            std::cout << "NEO: " << neoBalance << std::endl;
            std::cout << "GAS: " << gasBalance << std::endl;

            return true;
        }
        catch (const std::exception& ex)
        {
            std::cout << "Failed to open wallet: " << ex.what() << std::endl;
            return false;
        }
    }

    bool CommandHandler::HandleCloseWallet(const std::vector<std::string>& args)
    {
        if (!wallet_)
        {
            std::cout << "No wallet is open" << std::endl;
            return false;
        }

        wallet_ = nullptr;
        std::cout << "Wallet closed" << std::endl;

        return true;
    }

    bool CommandHandler::HandleCreateWallet(const std::vector<std::string>& args)
    {
        if (args.size() < 1)
        {
            std::cout << "Usage: create wallet <path> [password]" << std::endl;
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
            std::cout << "Password: ";
            std::getline(std::cin, password);

            std::string confirmPassword;
            std::cout << "Confirm password: ";
            std::getline(std::cin, confirmPassword);

            if (password != confirmPassword)
            {
                std::cout << "Passwords do not match" << std::endl;
                return false;
            }
        }

        try
        {
            std::string name = "Neo C++ Wallet";
            wallet_ = wallets::WalletFactoryManager::GetInstance().CreateWallet(path, password, name);

            // Create default account
            auto account = wallet_->CreateAccount();
            wallet_->Save();

            std::cout << "Wallet created successfully" << std::endl;
            std::cout << "Default account: " << account->GetAddress() << std::endl;

            return true;
        }
        catch (const std::exception& ex)
        {
            std::cout << "Failed to create wallet: " << ex.what() << std::endl;
            return false;
        }
    }

    bool CommandHandler::HandleImportKey(const std::vector<std::string>& args)
    {
        if (!wallet_)
        {
            std::cout << "No wallet is open" << std::endl;
            return false;
        }

        if (args.size() < 1)
        {
            std::cout << "Usage: import key <wif|hex>" << std::endl;
            return false;
        }

        std::string key = args[0];

        try
        {
            std::shared_ptr<wallets::WalletAccount> account;

            if (key.size() == 52 && key[0] == 'K' || key[0] == 'L')
            {
                // WIF format
                account = wallet_->ImportFromWIF(key);
            }
            else
            {
                // Hex format
                io::ByteVector privateKey = io::ByteVector::FromHex(key);
                account = wallet_->ImportFromPrivateKey(privateKey);
            }

            wallet_->Save();

            std::cout << "Key imported successfully" << std::endl;
            std::cout << "Address: " << account->GetAddress() << std::endl;

            return true;
        }
        catch (const std::exception& ex)
        {
            std::cout << "Failed to import key: " << ex.what() << std::endl;
            return false;
        }
    }

    bool CommandHandler::HandleExportKey(const std::vector<std::string>& args)
    {
        if (!wallet_)
        {
            std::cout << "No wallet is open" << std::endl;
            return false;
        }

        if (args.size() < 1)
        {
            std::cout << "Usage: export key <address> [password]" << std::endl;
            return false;
        }

        std::string address = args[0];
        std::string password;

        if (args.size() >= 2)
        {
            password = args[1];
        }
        else
        {
            std::cout << "Password: ";
            std::getline(std::cin, password);
        }

        try
        {
            auto account = wallet_->GetAccount(address);
            if (!account)
            {
                std::cout << "Account not found" << std::endl;
                return false;
            }

            if (!wallet_->VerifyPassword(password))
            {
                std::cout << "Incorrect password" << std::endl;
                return false;
            }

            auto privateKey = account->GetPrivateKey(password);
            auto wif = account->GetWIF(password);

            std::cout << "Private key (hex): " << privateKey.ToHex() << std::endl;
            std::cout << "Private key (WIF): " << wif << std::endl;

            return true;
        }
        catch (const std::exception& ex)
        {
            std::cout << "Failed to export key: " << ex.what() << std::endl;
            return false;
        }
    }

    bool CommandHandler::HandleListAddress(const std::vector<std::string>& args)
    {
        if (!wallet_)
        {
            std::cout << "No wallet is open" << std::endl;
            return false;
        }

        auto accounts = wallet_->GetAccounts();

        std::cout << "Addresses:" << std::endl;

        for (const auto& account : accounts)
        {
            std::cout << "  " << account->GetAddress() << std::endl;
        }

        return true;
    }

    bool CommandHandler::HandleListAsset(const std::vector<std::string>& args)
    {
        if (!wallet_)
        {
            std::cout << "No wallet is open" << std::endl;
            return false;
        }

        auto neoBalance = wallet_->GetBalance(smartcontract::native::NeoToken::SCRIPT_HASH);
        auto gasBalance = wallet_->GetBalance(smartcontract::native::GasToken::SCRIPT_HASH);

        std::cout << "Assets:" << std::endl;
        std::cout << "  NEO: " << neoBalance << std::endl;
        std::cout << "  GAS: " << gasBalance << std::endl;

        return true;
    }

    bool CommandHandler::HandleImportNEP2(const std::vector<std::string>& args)
    {
        if (!wallet_)
        {
            std::cout << "No wallet is open" << std::endl;
            return false;
        }

        if (args.size() < 1)
        {
            std::cout << "Usage: import nep2 <nep2-key> [password]" << std::endl;
            return false;
        }

        std::string nep2Key = args[0];
        std::string password;

        if (args.size() >= 2)
        {
            password = args[1];
        }
        else
        {
            std::cout << "Password: ";
            std::getline(std::cin, password);
        }

        try
        {
            // Check if the wallet is a NEP6 wallet
            auto nep6Wallet = std::dynamic_pointer_cast<wallets::nep6::NEP6Wallet>(wallet_);
            if (!nep6Wallet)
            {
                std::cout << "The current wallet does not support NEP2 keys" << std::endl;
                return false;
            }

            // Import the NEP2 key
            auto account = nep6Wallet->ImportFromNEP2(nep2Key, password);
            wallet_->Save();

            std::cout << "NEP2 key imported successfully" << std::endl;
            std::cout << "Address: " << account->GetAddress() << std::endl;

            return true;
        }
        catch (const std::exception& ex)
        {
            std::cout << "Failed to import NEP2 key: " << ex.what() << std::endl;
            return false;
        }
    }
}
