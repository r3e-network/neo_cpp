#pragma once

#include <neo/sdk/core/types.h>
#include <neo/sdk/wallet/wallet.h>
#include <memory>
#include <vector>

namespace neo::sdk::tx {

/**
 * @brief Fluent interface for building Neo transactions
 */
class TransactionBuilder {
public:
    TransactionBuilder();
    ~TransactionBuilder();
    
    /**
     * @brief Set the transaction sender (fee payer)
     */
    TransactionBuilder& SetSender(const core::UInt160& sender);
    
    /**
     * @brief Set the system fee (for execution)
     */
    TransactionBuilder& SetSystemFee(uint64_t fee);
    
    /**
     * @brief Set the network fee (for transaction size)
     */
    TransactionBuilder& SetNetworkFee(uint64_t fee);
    
    /**
     * @brief Set the block until which the transaction is valid
     */
    TransactionBuilder& SetValidUntilBlock(uint32_t block);
    
    /**
     * @brief Add a transaction attribute
     */
    TransactionBuilder& AddAttribute(const core::TransactionAttribute& attr);
    
    /**
     * @brief Add a witness to the transaction
     */
    TransactionBuilder& AddWitness(const core::Witness& witness);
    
    /**
     * @brief Add a signer to the transaction
     */
    TransactionBuilder& AddSigner(const core::Signer& signer);
    
    /**
     * @brief Set the transaction script
     */
    TransactionBuilder& SetScript(const std::vector<uint8_t>& script);
    
    /**
     * @brief Invoke a smart contract method
     */
    TransactionBuilder& InvokeContract(
        const core::UInt160& scriptHash,
        const std::string& method,
        const std::vector<core::ContractParameter>& params
    );
    
    /**
     * @brief Transfer NEP-17 tokens
     */
    TransactionBuilder& Transfer(
        const core::UInt160& from,
        const core::UInt160& to,
        const std::string& asset,
        uint64_t amount
    );
    
    /**
     * @brief Transfer NEP-17 tokens with data
     */
    TransactionBuilder& TransferWithData(
        const core::UInt160& from,
        const core::UInt160& to,
        const std::string& asset,
        uint64_t amount,
        const std::vector<uint8_t>& data
    );
    
    /**
     * @brief Add a cosigner
     */
    TransactionBuilder& AddCosigner(
        const core::UInt160& account,
        uint8_t scopes
    );
    
    /**
     * @brief Set transaction nonce
     */
    TransactionBuilder& SetNonce(uint32_t nonce);
    
    /**
     * @brief Build the transaction (without signing)
     */
    std::shared_ptr<core::Transaction> Build();
    
    /**
     * @brief Build and sign the transaction with a wallet
     */
    std::shared_ptr<core::Transaction> BuildAndSign(wallet::Wallet& wallet);
    
    /**
     * @brief Reset the builder to create a new transaction
     */
    TransactionBuilder& Reset();
    
    /**
     * @brief Calculate the required network fee
     */
    uint64_t CalculateNetworkFee() const;
    
    /**
     * @brief Estimate the system fee (requires RPC connection)
     */
    uint64_t EstimateSystemFee() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace neo::sdk::tx