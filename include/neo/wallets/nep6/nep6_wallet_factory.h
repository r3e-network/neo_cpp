#pragma once

#include <neo/wallets/wallet_factory.h>
#include <string>
#include <memory>

namespace neo::wallets::nep6
{
    /**
     * @brief Factory for creating NEP6 wallets.
     */
    class NEP6WalletFactory : public WalletFactory
    {
    public:
        /**
         * @brief Gets the singleton instance of the NEP6WalletFactory.
         * @return The singleton instance.
         */
        static std::shared_ptr<NEP6WalletFactory> GetInstance();

        /**
         * @brief Checks if the factory can handle the specified path.
         * @param path The path.
         * @return True if the factory can handle the path, false otherwise.
         */
        bool CanHandle(const std::string& path) const override;

        /**
         * @brief Creates a new wallet.
         * @param path The path.
         * @param password The password.
         * @param name The name.
         * @return The new wallet.
         */
        std::shared_ptr<Wallet> CreateWallet(const std::string& path, const std::string& password, const std::string& name) const override;

        /**
         * @brief Opens an existing wallet.
         * @param path The path.
         * @param password The password.
         * @return The opened wallet.
         */
        std::shared_ptr<Wallet> OpenWallet(const std::string& path, const std::string& password) const override;
    };
}
