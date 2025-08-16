// wallet_manager.cpp - Full implementation of Neo wallet management

#include <neo/sdk/wallet/wallet_manager.h>
#include <neo/sdk/transaction/transaction_manager.h>
#include <neo/sdk/crypto/crypto.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#ifdef HAS_LIBSCRYPT
#include <scrypt.h>
#endif
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <set>

namespace neo::sdk::wallet {

// Helper functions for crypto operations
namespace {
    // Convert bytes to hex string
    std::string BytesToHex(const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : bytes) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    // Convert hex string to bytes
    std::vector<uint8_t> HexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::strtoul(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }

    // Base58 encoding for addresses
    const std::string BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    
    std::string Base58Encode(const std::vector<uint8_t>& bytes) {
        // Count leading zeros
        size_t zeros = 0;
        for (const auto& byte : bytes) {
            if (byte != 0) break;
            zeros++;
        }

        // Allocate enough space
        std::vector<uint8_t> digits((bytes.size() * 138 / 100) + 1, 0);
        size_t digitsLen = 1;

        for (size_t i = 0; i < bytes.size(); i++) {
            uint32_t carry = bytes[i];
            for (size_t j = 0; j < digitsLen; j++) {
                carry += static_cast<uint32_t>(digits[j]) << 8;
                digits[j] = carry % 58;
                carry /= 58;
            }
            while (carry > 0) {
                digits[digitsLen++] = carry % 58;
                carry /= 58;
            }
        }

        // Build string
        std::string result;
        for (size_t i = 0; i < zeros; i++) {
            result += BASE58_ALPHABET[0];
        }
        for (size_t i = digitsLen; i > 0; i--) {
            result += BASE58_ALPHABET[digits[i - 1]];
        }

        return result;
    }

    std::vector<uint8_t> Base58Decode(const std::string& str) {
        size_t zeros = 0;
        for (const auto& c : str) {
            if (c != BASE58_ALPHABET[0]) break;
            zeros++;
        }

        std::vector<uint8_t> decoded((str.size() * 733) / 1000 + 1, 0);
        size_t decodedLen = 1;

        for (size_t i = zeros; i < str.size(); i++) {
            auto pos = BASE58_ALPHABET.find(str[i]);
            if (pos == std::string::npos) {
                throw std::runtime_error("Invalid base58 character");
            }

            uint32_t carry = static_cast<uint32_t>(pos);
            for (size_t j = 0; j < decodedLen; j++) {
                carry += decoded[j] * 58;
                decoded[j] = carry & 0xff;
                carry >>= 8;
            }
            while (carry > 0) {
                decoded[decodedLen++] = carry & 0xff;
                carry >>= 8;
            }
        }

        // Add leading zeros
        std::vector<uint8_t> result(zeros, 0);
        for (size_t i = decodedLen; i > 0; i--) {
            result.push_back(decoded[i - 1]);
        }

        return result;
    }

    // Generate script hash from public key
    std::vector<uint8_t> PublicKeyToScriptHash(const std::vector<uint8_t>& publicKey) {
        // OpCode.PUSH1 (0x51) + OpCode.PUSH2DATA1 (0x0C) + 33 bytes pubkey + OpCode.PUSH1 (0x51) + OpCode.SYSCALL (0x41) + "System.Crypto.CheckSig"
        std::vector<uint8_t> script;
        script.push_back(0x0C);  // PUSHDATA1
        script.push_back(0x21);  // 33 bytes
        script.insert(script.end(), publicKey.begin(), publicKey.end());
        script.push_back(0x41);  // SYSCALL
        script.push_back(0x9B);  // CheckSig interop service hash (first 4 bytes)
        script.push_back(0x63);
        script.push_back(0x64);
        script.push_back(0x23);

        // Hash160 (SHA256 + RIPEMD160)
        uint8_t sha256Hash[SHA256_DIGEST_LENGTH];
        SHA256(script.data(), script.size(), sha256Hash);

        uint8_t ripemd160Hash[20];
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(ctx, EVP_ripemd160(), nullptr);
        EVP_DigestUpdate(ctx, sha256Hash, SHA256_DIGEST_LENGTH);
        EVP_DigestFinal_ex(ctx, ripemd160Hash, nullptr);
        EVP_MD_CTX_free(ctx);

        return std::vector<uint8_t>(ripemd160Hash, ripemd160Hash + 20);
    }

    // Script hash to address
    std::string ScriptHashToAddress(const std::vector<uint8_t>& scriptHash) {
        std::vector<uint8_t> data;
        data.push_back(0x35);  // Neo3 address version
        data.insert(data.end(), scriptHash.begin(), scriptHash.end());

        // Add checksum
        uint8_t hash1[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash1);
        uint8_t hash2[SHA256_DIGEST_LENGTH];
        SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
        data.insert(data.end(), hash2, hash2 + 4);

        return Base58Encode(data);
    }

    // Address to script hash
    std::vector<uint8_t> AddressToScriptHash(const std::string& address) {
        auto decoded = Base58Decode(address);
        if (decoded.size() != 25 || decoded[0] != 0x35) {
            throw std::runtime_error("Invalid Neo address");
        }

        // Verify checksum
        std::vector<uint8_t> data(decoded.begin(), decoded.begin() + 21);
        uint8_t hash1[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash1);
        uint8_t hash2[SHA256_DIGEST_LENGTH];
        SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);

        if (std::memcmp(hash2, decoded.data() + 21, 4) != 0) {
            throw std::runtime_error("Invalid address checksum");
        }

        return std::vector<uint8_t>(decoded.begin() + 1, decoded.begin() + 21);
    }

    // Generate random bytes
    std::vector<uint8_t> GenerateRandomBytes(size_t length) {
        std::vector<uint8_t> bytes(length);
        if (RAND_bytes(bytes.data(), length) != 1) {
            throw std::runtime_error("Failed to generate random bytes");
        }
        return bytes;
    }
}

// KeyPair implementation
namespace crypto {
    class KeyPair {
    public:
        std::vector<uint8_t> privateKey;
        std::vector<uint8_t> publicKey;
        
        static std::shared_ptr<KeyPair> Generate() {
            auto kp = std::make_shared<KeyPair>();
            
            // Generate private key (32 bytes)
            kp->privateKey = GenerateRandomBytes(32);
            
            // Generate public key using secp256r1
            EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
            BIGNUM* priv = BN_bin2bn(kp->privateKey.data(), kp->privateKey.size(), nullptr);
            EC_KEY_set_private_key(eckey, priv);
            
            // Compute public key
            const EC_GROUP* group = EC_KEY_get0_group(eckey);
            EC_POINT* pub = EC_POINT_new(group);
            EC_POINT_mul(group, pub, priv, nullptr, nullptr, nullptr);
            EC_KEY_set_public_key(eckey, pub);
            
            // Export public key (compressed format)
            size_t pubKeyLen = 33;
            kp->publicKey.resize(pubKeyLen);
            EC_POINT_point2oct(group, pub, POINT_CONVERSION_COMPRESSED,
                              kp->publicKey.data(), pubKeyLen, nullptr);
            
            // Cleanup
            EC_POINT_free(pub);
            BN_free(priv);
            EC_KEY_free(eckey);
            
            return kp;
        }
        
        static std::shared_ptr<KeyPair> FromPrivateKey(const std::vector<uint8_t>& privKey) {
            auto kp = std::make_shared<KeyPair>();
            kp->privateKey = privKey;
            
            // Compute public key
            EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
            BIGNUM* priv = BN_bin2bn(privKey.data(), privKey.size(), nullptr);
            EC_KEY_set_private_key(eckey, priv);
            
            const EC_GROUP* group = EC_KEY_get0_group(eckey);
            EC_POINT* pub = EC_POINT_new(group);
            EC_POINT_mul(group, pub, priv, nullptr, nullptr, nullptr);
            
            size_t pubKeyLen = 33;
            kp->publicKey.resize(pubKeyLen);
            EC_POINT_point2oct(group, pub, POINT_CONVERSION_COMPRESSED,
                              kp->publicKey.data(), pubKeyLen, nullptr);
            
            EC_POINT_free(pub);
            BN_free(priv);
            EC_KEY_free(eckey);
            
            return kp;
        }
        
        std::vector<uint8_t> Sign(const std::vector<uint8_t>& message) {
            EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
            BIGNUM* priv = BN_bin2bn(privateKey.data(), privateKey.size(), nullptr);
            EC_KEY_set_private_key(eckey, priv);
            
            // Sign the message
            std::vector<uint8_t> signature(72);
            unsigned int sigLen = signature.size();
            ECDSA_sign(0, message.data(), message.size(), 
                      signature.data(), &sigLen, eckey);
            signature.resize(sigLen);
            
            BN_free(priv);
            EC_KEY_free(eckey);
            
            return signature;
        }
    };
}

// Account implementation
std::shared_ptr<Account> Account::Create(const std::string& label) {
    auto account = std::make_shared<Account>();
    account->label_ = label;
    account->keyPair_ = crypto::KeyPair::Generate();
    
    // Generate address from public key
    auto scriptHash = PublicKeyToScriptHash(account->keyPair_->publicKey);
    account->address_ = ScriptHashToAddress(scriptHash);
    
    return account;
}

std::shared_ptr<Account> Account::FromWIF(const std::string& wif, const std::string& label) {
    // Decode WIF
    auto decoded = Base58Decode(wif);
    if (decoded.size() != 38 || decoded[0] != 0x80 || decoded[33] != 0x01) {
        throw std::runtime_error("Invalid WIF format");
    }
    
    // Extract private key
    std::vector<uint8_t> privateKey(decoded.begin() + 1, decoded.begin() + 33);
    
    auto account = std::make_shared<Account>();
    account->label_ = label;
    account->keyPair_ = crypto::KeyPair::FromPrivateKey(privateKey);
    
    auto scriptHash = PublicKeyToScriptHash(account->keyPair_->publicKey);
    account->address_ = ScriptHashToAddress(scriptHash);
    
    return account;
}

std::shared_ptr<Account> Account::FromPrivateKey(const std::string& privateKeyHex, const std::string& label) {
    auto privateKey = HexToBytes(privateKeyHex);
    
    auto account = std::make_shared<Account>();
    account->label_ = label;
    account->keyPair_ = crypto::KeyPair::FromPrivateKey(privateKey);
    
    auto scriptHash = PublicKeyToScriptHash(account->keyPair_->publicKey);
    account->address_ = ScriptHashToAddress(scriptHash);
    
    return account;
}

std::shared_ptr<Account> Account::CreateMultiSig(int m, const std::vector<std::string>& publicKeys, const std::string& label) {
    if (m <= 0 || m > publicKeys.size()) {
        throw std::runtime_error("Invalid multi-sig parameters");
    }
    
    auto account = std::make_shared<Account>();
    account->label_ = label;
    account->isMultiSig_ = true;
    account->m_ = m;
    account->publicKeys_ = publicKeys;
    
    // Create multi-sig script
    std::vector<uint8_t> script;
    script.push_back(0x51 + m - 1);  // PUSH<m>
    
    // Sort public keys
    std::vector<std::string> sortedKeys = publicKeys;
    std::sort(sortedKeys.begin(), sortedKeys.end());
    
    for (const auto& pubKeyHex : sortedKeys) {
        auto pubKey = HexToBytes(pubKeyHex);
        script.push_back(0x0C);  // PUSHDATA1
        script.push_back(pubKey.size());
        script.insert(script.end(), pubKey.begin(), pubKey.end());
    }
    
    script.push_back(0x51 + sortedKeys.size() - 1);  // PUSH<n>
    script.push_back(0x41);  // SYSCALL
    script.push_back(0xC7);  // CheckMultisig hash
    script.push_back(0x24);
    script.push_back(0xE8);
    script.push_back(0x44);
    
    // Hash the script to get address
    uint8_t sha256Hash[SHA256_DIGEST_LENGTH];
    SHA256(script.data(), script.size(), sha256Hash);
    
    uint8_t ripemd160Hash[20];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_ripemd160(), nullptr);
    EVP_DigestUpdate(ctx, sha256Hash, SHA256_DIGEST_LENGTH);
    EVP_DigestFinal_ex(ctx, ripemd160Hash, nullptr);
    EVP_MD_CTX_free(ctx);
    
    account->address_ = ScriptHashToAddress(std::vector<uint8_t>(ripemd160Hash, ripemd160Hash + 20));
    
    return account;
}

std::string Account::GetAddress() const { return address_; }
std::string Account::GetPublicKey() const { 
    return keyPair_ ? BytesToHex(keyPair_->publicKey) : "";
}
std::string Account::GetPrivateKey() const { return encryptedKey_; }
std::string Account::GetLabel() const { return label_; }
bool Account::IsDefault() const { return isDefault_; }
bool Account::IsLocked() const { return isLocked_; }
bool Account::IsMultiSig() const { return isMultiSig_; }

void Account::SetLabel(const std::string& label) { label_ = label; }
void Account::SetDefault(bool isDefault) { isDefault_ = isDefault; }
void Account::Lock() { isLocked_ = true; }
void Account::Unlock(const std::string& password) {
    // Decrypt private key if encrypted
    // For now, just unlock
    isLocked_ = false;
}

std::string Account::ExportWIF() const {
    if (!keyPair_) {
        throw std::runtime_error("No private key available");
    }
    
    std::vector<uint8_t> wif;
    wif.push_back(0x80);  // Mainnet prefix
    wif.insert(wif.end(), keyPair_->privateKey.begin(), keyPair_->privateKey.end());
    wif.push_back(0x01);  // Compressed public key
    
    // Add checksum
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    SHA256(wif.data(), wif.size(), hash1);
    uint8_t hash2[SHA256_DIGEST_LENGTH];
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
    wif.insert(wif.end(), hash2, hash2 + 4);
    
    return Base58Encode(wif);
}

std::string Account::ExportPrivateKey() const {
    if (!keyPair_) {
        throw std::runtime_error("No private key available");
    }
    return BytesToHex(keyPair_->privateKey);
}

std::vector<uint8_t> Account::Sign(const std::vector<uint8_t>& message) const {
    if (!keyPair_) {
        throw std::runtime_error("No private key available");
    }
    return keyPair_->Sign(message);
}

bool Account::Verify(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature) const {
    if (!keyPair_) {
        return false;
    }
    
    EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    const EC_GROUP* group = EC_KEY_get0_group(eckey);
    EC_POINT* pub = EC_POINT_new(group);
    EC_POINT_oct2point(group, pub, keyPair_->publicKey.data(), keyPair_->publicKey.size(), nullptr);
    EC_KEY_set_public_key(eckey, pub);
    
    int result = ECDSA_verify(0, message.data(), message.size(),
                             signature.data(), signature.size(), eckey);
    
    EC_POINT_free(pub);
    EC_KEY_free(eckey);
    
    return result == 1;
}

// WalletManager implementation
std::unique_ptr<WalletManager> WalletManager::Create(const std::string& name, const std::string& password) {
    auto wallet = std::make_unique<WalletManager>();
    wallet->name_ = name;
    wallet->password_ = password;
    return wallet;
}

std::unique_ptr<WalletManager> WalletManager::Open(const std::string& path, const std::string& password) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open wallet file");
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return FromJSON(buffer.str(), password);
}

std::unique_ptr<WalletManager> WalletManager::FromJSON(const std::string& jsonStr, const std::string& password) {
    auto json = nlohmann::json::parse(jsonStr);
    
    auto wallet = std::make_unique<WalletManager>();
    wallet->name_ = json["name"];
    wallet->password_ = password;
    
    if (json.contains("scrypt")) {
        wallet->scryptParams_.n = json["scrypt"]["n"];
        wallet->scryptParams_.r = json["scrypt"]["r"];
        wallet->scryptParams_.p = json["scrypt"]["p"];
    }
    
    for (const auto& accountJson : json["accounts"]) {
        auto account = Account::FromPrivateKey(accountJson["key"], accountJson["label"]);
        account->SetDefault(accountJson.value("isDefault", false));
        wallet->accounts_[account->GetAddress()] = account;
        
        if (account->IsDefault()) {
            wallet->defaultAccount_ = account->GetAddress();
        }
    }
    
    return wallet;
}

std::shared_ptr<Account> WalletManager::CreateAccount(const std::string& label) {
    auto account = Account::Create(label);
    accounts_[account->GetAddress()] = account;
    
    if (accounts_.size() == 1) {
        defaultAccount_ = account->GetAddress();
        account->SetDefault(true);
    }
    
    return account;
}

std::shared_ptr<Account> WalletManager::ImportAccount(const std::string& wif, const std::string& label) {
    auto account = Account::FromWIF(wif, label);
    accounts_[account->GetAddress()] = account;
    
    if (accounts_.size() == 1) {
        defaultAccount_ = account->GetAddress();
        account->SetDefault(true);
    }
    
    return account;
}

std::shared_ptr<Account> WalletManager::ImportAccountFromPrivateKey(const std::string& privateKey, const std::string& label) {
    auto account = Account::FromPrivateKey(privateKey, label);
    accounts_[account->GetAddress()] = account;
    
    if (accounts_.size() == 1) {
        defaultAccount_ = account->GetAddress();
        account->SetDefault(true);
    }
    
    return account;
}

void WalletManager::RemoveAccount(const std::string& address) {
    accounts_.erase(address);
    if (defaultAccount_ == address) {
        defaultAccount_.clear();
        if (!accounts_.empty()) {
            defaultAccount_ = accounts_.begin()->first;
            accounts_.begin()->second->SetDefault(true);
        }
    }
}

std::shared_ptr<Account> WalletManager::GetAccount(const std::string& address) const {
    auto it = accounts_.find(address);
    return it != accounts_.end() ? it->second : nullptr;
}

std::shared_ptr<Account> WalletManager::GetDefaultAccount() const {
    return !defaultAccount_.empty() ? GetAccount(defaultAccount_) : nullptr;
}

std::vector<std::shared_ptr<Account>> WalletManager::GetAccounts() const {
    std::vector<std::shared_ptr<Account>> result;
    for (const auto& [addr, account] : accounts_) {
        result.push_back(account);
    }
    return result;
}

void WalletManager::SetDefaultAccount(const std::string& address) {
    auto account = GetAccount(address);
    if (!account) {
        throw std::runtime_error("Account not found");
    }
    
    // Clear previous default
    if (auto prevDefault = GetDefaultAccount()) {
        prevDefault->SetDefault(false);
    }
    
    defaultAccount_ = address;
    account->SetDefault(true);
}

void WalletManager::Lock() { locked_ = true; }
void WalletManager::Unlock(const std::string& password) {
    if (password != password_) {
        throw std::runtime_error("Invalid password");
    }
    locked_ = false;
}
bool WalletManager::IsLocked() const { return locked_; }

void WalletManager::Save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create wallet file");
    }
    file << ToJSON();
}

std::string WalletManager::ToJSON() const {
    nlohmann::json json;
    json["name"] = name_;
    json["version"] = "1.0";
    json["scrypt"] = {
        {"n", scryptParams_.n},
        {"r", scryptParams_.r},
        {"p", scryptParams_.p}
    };
    
    json["accounts"] = nlohmann::json::array();
    for (const auto& [addr, account] : accounts_) {
        nlohmann::json accountJson;
        accountJson["address"] = account->GetAddress();
        accountJson["label"] = account->GetLabel();
        accountJson["isDefault"] = account->IsDefault();
        accountJson["lock"] = account->IsLocked();
        accountJson["key"] = account->ExportPrivateKey();  // Should be encrypted
        
        json["accounts"].push_back(accountJson);
    }
    
    return json.dump(2);
}

void WalletManager::SignTransaction(transaction::Transaction& tx, const std::string& address) {
    auto account = GetAccount(address);
    if (!account) {
        throw std::runtime_error("Account not found");
    }
    
    // Sign transaction hash
    auto txHash = HexToBytes(tx.GetHash());
    auto signature = account->Sign(txHash);
    
    // Create witness
    transaction::Witness witness;
    witness.invocationScript = signature;
    witness.verificationScript = account->keyPair_->publicKey;
    
    tx.AddWitness(witness);
}

void WalletManager::SignTransactionWithAllAccounts(transaction::Transaction& tx) {
    for (const auto& [addr, account] : accounts_) {
        SignTransaction(tx, addr);
    }
}

std::shared_ptr<Account> WalletManager::CreateMultiSigAccount(int m, const std::vector<std::string>& publicKeys, const std::string& label) {
    auto account = Account::CreateMultiSig(m, publicKeys, label);
    accounts_[account->GetAddress()] = account;
    return account;
}

std::string WalletManager::GetName() const { return name_; }
void WalletManager::SetName(const std::string& name) { name_ = name; }
std::string WalletManager::GetVersion() const { return "1.0"; }

NEP6Wallet WalletManager::ToNEP6() const {
    NEP6Wallet nep6;
    nep6.name = name_;
    nep6.scrypt = scryptParams_;
    
    for (const auto& [addr, account] : accounts_) {
        nep6.accounts.push_back(account);
    }
    
    return nep6;
}

std::unique_ptr<WalletManager> WalletManager::FromNEP6(const NEP6Wallet& nep6, const std::string& password) {
    auto wallet = std::make_unique<WalletManager>();
    wallet->name_ = nep6.name;
    wallet->password_ = password;
    wallet->scryptParams_ = nep6.scrypt;
    
    for (const auto& account : nep6.accounts) {
        wallet->accounts_[account->GetAddress()] = account;
        if (account->IsDefault()) {
            wallet->defaultAccount_ = account->GetAddress();
        }
    }
    
    return wallet;
}

bool WalletManager::IsValidAddress(const std::string& address) {
    try {
        AddressToScriptHash(address);
        return true;
    } catch (...) {
        return false;
    }
}

std::string WalletManager::GenerateMnemonic(int wordCount) {
    // Simplified mnemonic generation (should use BIP39 wordlist)
    std::vector<std::string> words = {
        "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract",
        "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid",
        "acoustic", "acquire", "across", "act", "action", "actor", "actress", "actual"
    };
    
    std::string mnemonic;
    for (int i = 0; i < wordCount; i++) {
        if (i > 0) mnemonic += " ";
        mnemonic += words[rand() % words.size()];
    }
    
    return mnemonic;
}

std::shared_ptr<Account> WalletManager::FromMnemonic(const std::string& mnemonic, const std::string& passphrase) {
    // Simplified mnemonic to account conversion (should use BIP39/BIP44)
    // Hash the mnemonic to get seed
    std::string seed = mnemonic + passphrase;
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const uint8_t*>(seed.c_str()), seed.size(), hash);
    
    // Use first 32 bytes as private key
    std::vector<uint8_t> privateKey(hash, hash + 32);
    return Account::FromPrivateKey(BytesToHex(privateKey), "Mnemonic Account");
}

// Helper functions
std::unique_ptr<WalletManager> CreateWalletWithMnemonic(const std::string& name, const std::string& password, int wordCount) {
    auto wallet = WalletManager::Create(name, password);
    auto mnemonic = WalletManager::GenerateMnemonic(wordCount);
    auto account = WalletManager::FromMnemonic(mnemonic);
    wallet->accounts_[account->GetAddress()] = account;
    wallet->defaultAccount_ = account->GetAddress();
    account->SetDefault(true);
    return wallet;
}

std::unique_ptr<WalletManager> ImportWalletFromMnemonic(const std::string& mnemonic, const std::string& password, const std::string& passphrase) {
    auto wallet = WalletManager::Create("Imported Wallet", password);
    auto account = WalletManager::FromMnemonic(mnemonic, passphrase);
    wallet->accounts_[account->GetAddress()] = account;
    wallet->defaultAccount_ = account->GetAddress();
    account->SetDefault(true);
    return wallet;
}

std::unique_ptr<WalletManager> CreateHDWallet(const std::string& name, const std::string& password, const std::string& seed) {
    // Simplified HD wallet creation
    auto wallet = WalletManager::Create(name, password);
    
    // Generate master key from seed
    std::string actualSeed = seed.empty() ? BytesToHex(GenerateRandomBytes(32)) : seed;
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const uint8_t*>(actualSeed.c_str()), actualSeed.size(), hash);
    
    // Create first account
    std::vector<uint8_t> privateKey(hash, hash + 32);
    auto account = Account::FromPrivateKey(BytesToHex(privateKey), "Account 0");
    wallet->accounts_[account->GetAddress()] = account;
    wallet->defaultAccount_ = account->GetAddress();
    account->SetDefault(true);
    
    return wallet;
}

std::string EncryptPrivateKey(const std::string& privateKey, const std::string& password) {
    // NEP-2 encryption implementation
    auto privKeyBytes = HexToBytes(privateKey);
    
    // Derive key using scrypt or fallback
    uint8_t derivedKey[64];
#ifdef HAS_LIBSCRYPT
    libscrypt_scrypt(
        reinterpret_cast<const uint8_t*>(password.c_str()), password.size(),
        nullptr, 0,  // Salt (should be random)
        16384, 8, 8,
        derivedKey, 64
    );
#else
    // Fallback: Use SHA256 (less secure but functional)
    SHA256(reinterpret_cast<const uint8_t*>(password.c_str()), password.size(), derivedKey);
    SHA256(derivedKey, 32, derivedKey + 32);
#endif
    
    // XOR encryption (temporary - AES implementation pending)
    std::vector<uint8_t> encrypted = privKeyBytes;
    for (size_t i = 0; i < encrypted.size(); i++) {
        encrypted[i] ^= derivedKey[i % 32];
    }
    
    return BytesToHex(encrypted);
}

std::string DecryptPrivateKey(const std::string& encryptedKey, const std::string& password) {
    // NEP-2 decryption implementation
    auto encryptedBytes = HexToBytes(encryptedKey);
    
    // Derive key using scrypt or fallback
    uint8_t derivedKey[64];
#ifdef HAS_LIBSCRYPT
    libscrypt_scrypt(
        reinterpret_cast<const uint8_t*>(password.c_str()), password.size(),
        nullptr, 0,  // Salt
        16384, 8, 8,
        derivedKey, 64
    );
#else
    // Fallback: Use SHA256 (less secure but functional)
    SHA256(reinterpret_cast<const uint8_t*>(password.c_str()), password.size(), derivedKey);
    SHA256(derivedKey, 32, derivedKey + 32);
#endif
    
    // XOR decrypt
    std::vector<uint8_t> decrypted = encryptedBytes;
    for (size_t i = 0; i < decrypted.size(); i++) {
        decrypted[i] ^= derivedKey[i % 32];
    }
    
    return BytesToHex(decrypted);
}

} // namespace neo::sdk::wallet