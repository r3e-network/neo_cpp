#include <neo/sdk/wallet/wallet.h>
#include <neo/wallets/nep6_wallet.h>
#include <neo/cryptography/helper.h>
#include <neo/logging/logger.h>
#include <fstream>
#include <memory>

namespace neo::sdk::wallet {

// Private implementation class
class Wallet::Impl {
public:
    std::unique_ptr<neo::wallets::NEP6Wallet> nep6Wallet;
    std::string path;
    std::string password;
    bool isLocked = false;
    
    Impl() : nep6Wallet(std::make_unique<neo::wallets::NEP6Wallet>()) {}
};

// Constructor
Wallet::Wallet() : impl_(std::make_unique<Impl>()) {}

// Static factory methods
std::shared_ptr<Wallet> Wallet::Create(
    const std::string& path,
    const std::string& password,
    const std::string& name) {
    
    try {
        auto wallet = std::shared_ptr<Wallet>(new Wallet());
        wallet->impl_->path = path;
        wallet->impl_->password = password;
        
        // Create new NEP6 wallet
        wallet->impl_->nep6Wallet->SetName(name);
        wallet->impl_->nep6Wallet->SetVersion("1.0");
        
        // Save to file
        if (!wallet->Save()) {
            NEO_LOG_ERROR("Failed to save new wallet to: {}", path);
            return nullptr;
        }
        
        NEO_LOG_INFO("Created new wallet: {}", path);
        return wallet;
        
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to create wallet: {}", e.what());
        return nullptr;
    }
}

std::shared_ptr<Wallet> Wallet::Open(
    const std::string& path,
    const std::string& password) {
    
    try {
        // Check if file exists
        std::ifstream file(path);
        if (!file.good()) {
            throw std::runtime_error("Wallet file not found: " + path);
        }
        file.close();
        
        auto wallet = std::shared_ptr<Wallet>(new Wallet());
        wallet->impl_->path = path;
        wallet->impl_->password = password;
        
        // Load NEP6 wallet from file
        wallet->impl_->nep6Wallet = neo::wallets::NEP6Wallet::FromFile(path, password);
        
        NEO_LOG_INFO("Opened wallet: {}", path);
        return wallet;
        
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to open wallet: {}", e.what());
        throw;
    }
}

// Account management
Account Wallet::CreateAccount(const std::string& label) {
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    
    try {
        // Generate new key pair
        auto keyPair = neo::cryptography::KeyPair::Generate();
        
        // Create NEP6 account
        auto nep6Account = impl_->nep6Wallet->CreateAccount(keyPair, label);
        
        // Convert to SDK account
        Account account;
        // account.SetFromNEP6(nep6Account);
        
        NEO_LOG_INFO("Created new account with label: {}", label);
        return account;
        
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to create account: {}", e.what());
        throw;
    }
}

Account Wallet::ImportAccount(const std::string& wif, const std::string& label) {
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    
    try {
        // Import key pair from WIF
        auto keyPair = neo::cryptography::KeyPair::FromWIF(wif);
        
        // Create NEP6 account
        auto nep6Account = impl_->nep6Wallet->ImportAccount(wif, label);
        
        // Convert to SDK account
        Account account;
        // account.SetFromNEP6(nep6Account);
        
        NEO_LOG_INFO("Imported account with label: {}", label);
        return account;
        
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to import account from WIF: {}", e.what());
        throw;
    }
}

Account Wallet::ImportAccount(const std::vector<uint8_t>& privateKey, const std::string& label) {
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    
    try {
        // Create key pair from private key
        auto keyPair = neo::cryptography::KeyPair::FromPrivateKey(privateKey);
        
        // Convert to WIF and import
        auto wif = keyPair.GetWIF();
        return ImportAccount(wif, label);
        
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to import account from private key: {}", e.what());
        throw;
    }
}

std::vector<Account> Wallet::GetAccounts() const {
    std::vector<Account> accounts;
    
    auto nep6Accounts = impl_->nep6Wallet->GetAccounts();
    accounts.reserve(nep6Accounts.size());
    
    for (const auto& nep6Account : nep6Accounts) {
        Account account;
        // account.SetFromNEP6(nep6Account);
        accounts.push_back(account);
    }
    
    return accounts;
}

std::shared_ptr<Account> Wallet::GetAccount(const std::string& address) const {
    auto nep6Account = impl_->nep6Wallet->GetAccount(address);
    if (!nep6Account) {
        return nullptr;
    }
    
    auto account = std::make_shared<Account>();
    // account->SetFromNEP6(*nep6Account);
    return account;
}

std::shared_ptr<Account> Wallet::GetAccount(const core::UInt160& scriptHash) const {
    // Convert script hash to address
    auto address = neo::wallets::Helper::ToAddress(scriptHash);
    return GetAccount(address);
}

bool Wallet::DeleteAccount(const std::string& address) {
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    
    try {
        bool result = impl_->nep6Wallet->DeleteAccount(address);
        if (result) {
            NEO_LOG_INFO("Deleted account: {}", address);
        }
        return result;
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to delete account: {}", e.what());
        return false;
    }
}

Account Wallet::GetDefaultAccount() const {
    auto accounts = GetAccounts();
    if (accounts.empty()) {
        throw std::runtime_error("Wallet has no accounts");
    }
    
    // Find default account or return first
    auto defaultAccount = impl_->nep6Wallet->GetDefaultAccount();
    if (defaultAccount) {
        Account account;
        // account.SetFromNEP6(*defaultAccount);
        return account;
    }
    
    return accounts[0];
}

bool Wallet::SetDefaultAccount(const std::string& address) {
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    
    return impl_->nep6Wallet->SetDefaultAccount(address);
}

bool Wallet::ContainsAccount(const std::string& address) const {
    return impl_->nep6Wallet->ContainsAccount(address);
}

std::string Wallet::GetName() const {
    return impl_->nep6Wallet->GetName();
}

void Wallet::SetName(const std::string& name) {
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    impl_->nep6Wallet->SetName(name);
}

std::string Wallet::GetVersion() const {
    return impl_->nep6Wallet->GetVersion();
}

bool Wallet::Save() {
    try {
        return impl_->nep6Wallet->Save(impl_->path, impl_->password);
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to save wallet: {}", e.what());
        return false;
    }
}

bool Wallet::SaveAs(const std::string& path) {
    try {
        bool result = impl_->nep6Wallet->Save(path, impl_->password);
        if (result) {
            impl_->path = path;
        }
        return result;
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to save wallet as: {}", e.what());
        return false;
    }
}

void Wallet::Lock() {
    impl_->isLocked = true;
    // Clear sensitive data from memory
    impl_->password.clear();
}

bool Wallet::Unlock(const std::string& password) {
    // Verify password by trying to decrypt an account
    if (!impl_->nep6Wallet->VerifyPassword(password)) {
        return false;
    }
    
    impl_->password = password;
    impl_->isLocked = false;
    return true;
}

bool Wallet::IsLocked() const {
    return impl_->isLocked;
}

bool Wallet::ChangePassword(const std::string& oldPassword, const std::string& newPassword) {
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    
    if (impl_->password != oldPassword) {
        return false;
    }
    
    try {
        bool result = impl_->nep6Wallet->ChangePassword(oldPassword, newPassword);
        if (result) {
            impl_->password = newPassword;
        }
        return result;
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to change password: {}", e.what());
        return false;
    }
}

std::vector<uint8_t> Wallet::Sign(
    const std::vector<uint8_t>& message,
    const Account& account) {
    
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    
    try {
        // Get private key for account
        auto privateKey = impl_->nep6Wallet->GetPrivateKey(account.GetAddress(), impl_->password);
        
        // Sign message
        auto keyPair = neo::cryptography::KeyPair::FromPrivateKey(privateKey);
        return keyPair.Sign(message);
        
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to sign message: {}", e.what());
        throw;
    }
}

bool Wallet::SignTransaction(std::shared_ptr<core::Transaction> transaction) {
    if (impl_->isLocked) {
        throw std::runtime_error("Wallet is locked");
    }
    
    if (!transaction) {
        return false;
    }
    
    try {
        // Sign transaction with all required accounts
        // This would iterate through signers and sign with matching accounts
        // return impl_->nep6Wallet->SignTransaction(*transaction, impl_->password);
        return true;  // Placeholder
        
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to sign transaction: {}", e.what());
        return false;
    }
}

std::string Wallet::GetPath() const {
    return impl_->path;
}

} // namespace neo::sdk::wallet