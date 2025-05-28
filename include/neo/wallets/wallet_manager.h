#pragma once

#include <neo/wallets/wallet.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace neo::wallets
{
    /**
     * @brief Represents a wallet manager.
     */
    class WalletManager
    {
    public:
        /**
         * @brief Gets the instance.
         * @return The instance.
         */
        static WalletManager& GetInstance();

        /**
         * @brief Gets the wallets.
         * @return The wallets.
         */
        const std::vector<std::shared_ptr<Wallet>>& GetWallets() const;

        /**
         * @brief Gets the current wallet.
         * @return The current wallet.
         */
        std::shared_ptr<Wallet> GetCurrentWallet() const;

        /**
         * @brief Sets the current wallet.
         * @param wallet The current wallet.
         */
        void SetCurrentWallet(std::shared_ptr<Wallet> wallet);

        /**
         * @brief Gets a wallet by path.
         * @param path The path.
         * @return The wallet, or nullptr if not found.
         */
        std::shared_ptr<Wallet> GetWallet(const std::string& path) const;

        /**
         * @brief Gets a wallet by name.
         * @param name The name.
         * @return The wallet, or nullptr if not found.
         */
        std::shared_ptr<Wallet> GetWalletByName(const std::string& name) const;

        /**
         * @brief Creates a new wallet.
         * @param path The path.
         * @return The new wallet.
         */
        std::shared_ptr<Wallet> CreateWallet(const std::string& path);

        /**
         * @brief Opens a wallet.
         * @param path The path.
         * @return The wallet, or nullptr if not found.
         */
        std::shared_ptr<Wallet> OpenWallet(const std::string& path);

        /**
         * @brief Closes a wallet.
         * @param path The path.
         * @return True if the wallet was closed, false otherwise.
         */
        bool CloseWallet(const std::string& path);

        /**
         * @brief Closes a wallet.
         * @param wallet The wallet.
         * @return True if the wallet was closed, false otherwise.
         */
        bool CloseWallet(std::shared_ptr<Wallet> wallet);

        /**
         * @brief Closes all wallets.
         */
        void CloseAllWallets();

    private:
        WalletManager();
        std::vector<std::shared_ptr<Wallet>> wallets_;
        std::shared_ptr<Wallet> currentWallet_;
        mutable std::mutex mutex_;
    };
}
