#pragma once

#include <neo/sdk/core/types.h>
#include <string>
#include <vector>
#include <memory>

namespace neo::sdk::wallet {

/**
 * @brief Represents a wallet account
 */
class Account {
public:
    Account();
    ~Account();
    
    // Copy constructor and assignment operator
    Account(const Account& other);
    Account& operator=(const Account& other);
    
    // Move constructor and assignment operator
    Account(Account&& other) noexcept;
    Account& operator=(Account&& other) noexcept;
    
    /**
     * @brief Get the account address
     */
    std::string GetAddress() const;
    
    /**
     * @brief Get the account label
     */
    std::string GetLabel() const;
    
    /**
     * @brief Set the account label
     */
    void SetLabel(const std::string& label);
    
    /**
     * @brief Get the account script hash
     */
    core::UInt160 GetScriptHash() const;
    
    /**
     * @brief Get the account public key
     */
    core::ECPoint GetPublicKey() const;
    
    /**
     * @brief Check if account is default
     */
    bool IsDefault() const;
    
    /**
     * @brief Set account as default
     */
    void SetDefault(bool isDefault);
    
    /**
     * @brief Check if account is locked
     */
    bool IsLocked() const;
    
    /**
     * @brief Get the account contract
     */
    std::vector<uint8_t> GetContract() const;
    
    /**
     * @brief Check if account has private key
     */
    bool HasPrivateKey() const;
    
    /**
     * @brief Get account WIF (if unlocked)
     */
    std::string GetWIF() const;
    
    /**
     * @brief Check if account is watch-only
     */
    bool IsWatchOnly() const;
    
    /**
     * @brief Get account balance for an asset
     */
    uint64_t GetBalance(const std::string& asset) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    friend class Wallet;
};

} // namespace neo::sdk::wallet