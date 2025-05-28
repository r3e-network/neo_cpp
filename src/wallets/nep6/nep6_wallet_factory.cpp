#include <neo/wallets/nep6/nep6_wallet_factory.h>
#include <neo/wallets/nep6/nep6_wallet.h>
#include <filesystem>

namespace neo::wallets::nep6
{
    std::shared_ptr<NEP6WalletFactory> NEP6WalletFactory::GetInstance()
    {
        static std::shared_ptr<NEP6WalletFactory> instance = std::make_shared<NEP6WalletFactory>();
        return instance;
    }

    bool NEP6WalletFactory::CanHandle(const std::string& path) const
    {
        std::filesystem::path fsPath(path);
        return fsPath.extension().string() == ".json";
    }

    std::shared_ptr<Wallet> NEP6WalletFactory::CreateWallet(const std::string& path, const std::string& password, const std::string& name) const
    {
        if (std::filesystem::exists(path))
            throw std::invalid_argument("The wallet file already exists.");
        
        auto wallet = std::make_shared<NEP6Wallet>(path, password, name);
        wallet->Save();
        return wallet;
    }

    std::shared_ptr<Wallet> NEP6WalletFactory::OpenWallet(const std::string& path, const std::string& password) const
    {
        return std::make_shared<NEP6Wallet>(path, password);
    }
}
