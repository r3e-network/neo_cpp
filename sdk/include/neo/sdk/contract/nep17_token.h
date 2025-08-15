#pragma once

/**
 * @file nep17_token.h
 * @brief NEP-17 token standard implementation for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 */

#include <neo/sdk/core/types.h>
#include <neo/sdk/contract/contract_invoker.h>
#include <neo/sdk/wallet/wallet.h>
#include <memory>
#include <string>

namespace neo::sdk::contract {

/**
 * @brief NEP-17 token information
 */
struct TokenInfo {
    std::string symbol;
    uint8_t decimals;
    uint64_t totalSupply;
    std::string name;
};

/**
 * @brief NEP-17 token transfer event
 */
struct TransferEvent {
    core::UInt160 from;
    core::UInt160 to;
    uint64_t amount;
    core::UInt256 txId;
    uint32_t blockIndex;
    uint64_t timestamp;
};

/**
 * @brief NEP-17 token standard interface
 * 
 * Implements the NEP-17 fungible token standard for Neo blockchain.
 * NEP-17 is the standard for fungible tokens on Neo N3.
 */
class NEP17Token {
public:
    /**
     * @brief Constructor with contract hash
     * @param contractHash Token contract script hash
     * @param client RPC client for blockchain interaction
     */
    NEP17Token(const core::UInt160& contractHash, std::shared_ptr<rpc::RpcClient> client);

    /**
     * @brief Get token symbol
     * @return Token symbol (e.g., "NEO", "GAS")
     */
    std::string Symbol();

    /**
     * @brief Get token decimals
     * @return Number of decimal places
     */
    uint8_t Decimals();

    /**
     * @brief Get total supply
     * @return Total token supply
     */
    uint64_t TotalSupply();

    /**
     * @brief Get token name
     * @return Full token name
     */
    std::string Name();

    /**
     * @brief Get token information
     * @return Complete token information
     */
    TokenInfo GetTokenInfo();

    /**
     * @brief Get balance of an account
     * @param account Account script hash or address
     * @return Token balance
     */
    uint64_t BalanceOf(const core::UInt160& account);

    /**
     * @brief Get balance of an address
     * @param address Neo address
     * @return Token balance
     */
    uint64_t BalanceOf(const std::string& address);

    /**
     * @brief Transfer tokens
     * @param from Sender account
     * @param to Receiver account
     * @param amount Amount to transfer (in token units)
     * @param wallet Wallet for signing
     * @param data Optional data
     * @return Transaction ID
     */
    core::UInt256 Transfer(
        const core::UInt160& from,
        const core::UInt160& to,
        uint64_t amount,
        wallet::Wallet& wallet,
        const std::string& data = ""
    );

    /**
     * @brief Transfer tokens using addresses
     * @param fromAddress Sender address
     * @param toAddress Receiver address
     * @param amount Amount to transfer
     * @param wallet Wallet for signing
     * @param data Optional data
     * @return Transaction ID
     */
    core::UInt256 Transfer(
        const std::string& fromAddress,
        const std::string& toAddress,
        uint64_t amount,
        wallet::Wallet& wallet,
        const std::string& data = ""
    );

    /**
     * @brief Multi-transfer to multiple recipients
     * @param from Sender account
     * @param recipients Vector of (address, amount) pairs
     * @param wallet Wallet for signing
     * @return Transaction ID
     */
    core::UInt256 MultiTransfer(
        const core::UInt160& from,
        const std::vector<std::pair<core::UInt160, uint64_t>>& recipients,
        wallet::Wallet& wallet
    );

    /**
     * @brief Get transfer history for an account
     * @param account Account to query
     * @param limit Maximum number of transfers to return
     * @return Vector of transfer events
     */
    std::vector<TransferEvent> GetTransferHistory(
        const core::UInt160& account,
        uint32_t limit = 100
    );

    /**
     * @brief Calculate transfer fee
     * @param from Sender
     * @param to Receiver
     * @param amount Amount
     * @return System fee and network fee
     */
    std::pair<uint64_t, uint64_t> CalculateTransferFee(
        const core::UInt160& from,
        const core::UInt160& to,
        uint64_t amount
    );

    /**
     * @brief Get well-known NEP-17 tokens
     */
    static struct WellKnownTokens {
        static core::UInt160 NEO();  // Native NEO token
        static core::UInt160 GAS();  // Native GAS token
        static core::UInt160 GetBySymbol(const std::string& symbol);
    } Tokens;

    /**
     * @brief Convert amount with decimals
     * @param amount Raw amount
     * @param decimals Number of decimals
     * @return Human-readable amount
     */
    static double ToDecimalAmount(uint64_t amount, uint8_t decimals);

    /**
     * @brief Convert from decimal amount
     * @param decimalAmount Human-readable amount
     * @param decimals Number of decimals
     * @return Raw amount for blockchain
     */
    static uint64_t FromDecimalAmount(double decimalAmount, uint8_t decimals);

private:
    core::UInt160 contractHash_;
    std::shared_ptr<ContractInvoker> invoker_;
    std::shared_ptr<rpc::RpcClient> client_;
    
    // Cache token info
    mutable std::optional<TokenInfo> cachedInfo_;
    void RefreshTokenInfo() const;
};

} // namespace neo::sdk::contract