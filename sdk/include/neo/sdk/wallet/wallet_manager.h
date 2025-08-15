#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>
#include "../types/common.h"
#include "../crypto/keypair.h"

namespace neo {
namespace sdk {
namespace wallet {

// Forward declarations
class Account;
class Transaction;

// NEP-6 wallet format
struct NEP6Wallet {
    std::string name;
    std::string version = "1.0";
    struct Scrypt {
        int n = 16384;
        int r = 8;
        int p = 8;
    } scrypt;
    std::vector<std::shared_ptr<Account>> accounts;
    std::string extra;
};

// Account structure
class Account {
public:
    // Create new account
    static std::shared_ptr<Account> Create(const std::string& label = "");
    
    // Import account from private key (WIF format)
    static std::shared_ptr<Account> FromWIF(const std::string& wif, const std::string& label = "");
    
    // Import account from private key (hex format)
    static std::shared_ptr<Account> FromPrivateKey(const std::string& privateKeyHex, const std::string& label = "");
    
    // Create multi-signature account
    static std::shared_ptr<Account> CreateMultiSig(
        int m,  // minimum signatures required
        const std::vector<std::string>& publicKeys,
        const std::string& label = ""
    );
    
    // Getters
    std::string GetAddress() const;
    std::string GetPublicKey() const;
    std::string GetPrivateKey() const;  // Returns encrypted key
    std::string GetLabel() const;
    bool IsDefault() const;
    bool IsLocked() const;
    bool IsMultiSig() const;
    
    // Operations
    void SetLabel(const std::string& label);
    void SetDefault(bool isDefault);
    void Lock();
    void Unlock(const std::string& password);
    
    // Export
    std::string ExportWIF() const;
    std::string ExportPrivateKey() const;  // Hex format
    
    // Sign data
    std::vector<uint8_t> Sign(const std::vector<uint8_t>& message) const;
    
    // Verify signature
    bool Verify(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature) const;
    
private:
    std::string address_;
    std::string label_;
    bool isDefault_ = false;
    bool isLocked_ = true;
    std::shared_ptr<crypto::KeyPair> keyPair_;
    std::string encryptedKey_;
    bool isMultiSig_ = false;
    int m_ = 1;  // For multi-sig
    std::vector<std::string> publicKeys_;  // For multi-sig
};

// Wallet manager class
class WalletManager {
public:
    // Create new wallet
    static std::unique_ptr<WalletManager> Create(
        const std::string& name = "MyWallet",
        const std::string& password = ""
    );
    
    // Open existing wallet (NEP-6 format)
    static std::unique_ptr<WalletManager> Open(
        const std::string& path,
        const std::string& password
    );
    
    // Import wallet from JSON
    static std::unique_ptr<WalletManager> FromJSON(
        const std::string& json,
        const std::string& password
    );
    
    // Account management
    std::shared_ptr<Account> CreateAccount(const std::string& label = "");
    std::shared_ptr<Account> ImportAccount(const std::string& wif, const std::string& label = "");
    std::shared_ptr<Account> ImportAccountFromPrivateKey(const std::string& privateKey, const std::string& label = "");
    void RemoveAccount(const std::string& address);
    
    // Get accounts
    std::shared_ptr<Account> GetAccount(const std::string& address) const;
    std::shared_ptr<Account> GetDefaultAccount() const;
    std::vector<std::shared_ptr<Account>> GetAccounts() const;
    
    // Wallet operations
    void SetDefaultAccount(const std::string& address);
    void Lock();
    void Unlock(const std::string& password);
    bool IsLocked() const;
    
    // Save/Export
    void Save(const std::string& path);
    std::string ToJSON() const;
    
    // Transaction signing
    void SignTransaction(Transaction& tx, const std::string& address);
    void SignTransactionWithAllAccounts(Transaction& tx);
    
    // Multi-signature operations
    std::shared_ptr<Account> CreateMultiSigAccount(
        int m,
        const std::vector<std::string>& publicKeys,
        const std::string& label = ""
    );
    
    // Wallet properties
    std::string GetName() const;
    void SetName(const std::string& name);
    std::string GetVersion() const;
    
    // NEP-6 specific
    NEP6Wallet ToNEP6() const;
    static std::unique_ptr<WalletManager> FromNEP6(const NEP6Wallet& nep6, const std::string& password);
    
    // Utility
    static bool IsValidAddress(const std::string& address);
    static std::string GenerateMnemonic(int wordCount = 12);
    static std::shared_ptr<Account> FromMnemonic(const std::string& mnemonic, const std::string& passphrase = "");
    
private:
    std::string name_;
    std::string password_;
    bool locked_ = false;
    std::map<std::string, std::shared_ptr<Account>> accounts_;
    std::string defaultAccount_;
    NEP6Wallet::Scrypt scryptParams_;
};

// Transaction builder for easy transaction creation
class TransactionBuilder {
public:
    TransactionBuilder();
    
    // Set transaction properties
    TransactionBuilder& SetSender(const std::string& address);
    TransactionBuilder& SetSystemFee(uint64_t fee);
    TransactionBuilder& SetNetworkFee(uint64_t fee);
    TransactionBuilder& SetValidUntilBlock(uint32_t block);
    TransactionBuilder& SetNonce(uint32_t nonce);
    
    // Add transfers (NEP-17)
    TransactionBuilder& AddTransfer(
        const std::string& tokenHash,
        const std::string& from,
        const std::string& to,
        const std::string& amount
    );
    
    // Add script
    TransactionBuilder& AddScript(const std::vector<uint8_t>& script);
    
    // Add contract invocation
    TransactionBuilder& InvokeContract(
        const std::string& contractHash,
        const std::string& method,
        const std::vector<std::string>& params
    );
    
    // Add attributes
    TransactionBuilder& AddAttribute(const std::string& type, const std::string& value);
    
    // Add signer
    TransactionBuilder& AddSigner(const std::string& account, const std::string& scopes = "CalledByEntry");
    
    // Build transaction
    std::shared_ptr<Transaction> Build();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Helper functions

// Create wallet with mnemonic
std::unique_ptr<WalletManager> CreateWalletWithMnemonic(
    const std::string& name,
    const std::string& password,
    int wordCount = 12
);

// Import wallet from mnemonic
std::unique_ptr<WalletManager> ImportWalletFromMnemonic(
    const std::string& mnemonic,
    const std::string& password,
    const std::string& passphrase = ""
);

// Create HD wallet (Hierarchical Deterministic)
std::unique_ptr<WalletManager> CreateHDWallet(
    const std::string& name,
    const std::string& password,
    const std::string& seed = ""
);

// Encrypt/Decrypt private key (NEP-2)
std::string EncryptPrivateKey(const std::string& privateKey, const std::string& password);
std::string DecryptPrivateKey(const std::string& encryptedKey, const std::string& password);

} // namespace wallet
} // namespace sdk
} // namespace neo