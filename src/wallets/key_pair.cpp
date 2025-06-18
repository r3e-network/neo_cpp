#include <neo/wallets/key_pair.h>
#include <neo/wallets/helper.h>
#include <neo/cryptography/crypto.h>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <random>
#include <cstring>

namespace neo::wallets
{
    KeyPair::KeyPair(const io::ByteVector& privateKey)
        : privateKey_(privateKey)
    {
        if (privateKey_.size() != 32)
        {
            throw std::invalid_argument("Private key must be 32 bytes");
        }
        
        if (!IsValidPrivateKey(privateKey_))
        {
            throw std::invalid_argument("Invalid private key for secp256r1 curve");
        }
    }

    KeyPair::~KeyPair()
    {
        Clear();
    }

    KeyPair::KeyPair(const KeyPair& other)
        : privateKey_(other.privateKey_)
    {
        // Public key and script hash will be computed lazily
    }

    KeyPair::KeyPair(KeyPair&& other) noexcept
        : privateKey_(std::move(other.privateKey_)),
          publicKey_(std::move(other.publicKey_)),
          scriptHash_(std::move(other.scriptHash_))
    {
        other.Clear();
    }

    KeyPair& KeyPair::operator=(const KeyPair& other)
    {
        if (this != &other)
        {
            Clear();
            privateKey_ = other.privateKey_;
            // Public key and script hash will be computed lazily
        }
        return *this;
    }

    KeyPair& KeyPair::operator=(KeyPair&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            privateKey_ = std::move(other.privateKey_);
            publicKey_ = std::move(other.publicKey_);
            scriptHash_ = std::move(other.scriptHash_);
            other.Clear();
        }
        return *this;
    }

    const io::ByteVector& KeyPair::GetPrivateKey() const
    {
        return privateKey_;
    }

    const cryptography::ecc::ECPoint& KeyPair::GetPublicKey() const
    {
        if (!publicKey_)
        {
            ComputePublicKey();
        }
        return *publicKey_;
    }

    io::UInt160 KeyPair::GetScriptHash() const
    {
        if (!scriptHash_)
        {
            ComputeScriptHash();
        }
        return *scriptHash_;
    }

    std::string KeyPair::GetAddress(uint8_t address_version) const
    {
        return Helper::ToAddress(GetScriptHash(), address_version);
    }

    io::ByteVector KeyPair::Sign(const io::ByteVector& data) const
    {
        // Simplified ECDSA signature - in production use real cryptographic library
        io::ByteVector signature(64);
        
        std::hash<std::string> hasher;
        std::string combined;
        
        // Combine private key and data
        for (size_t i = 0; i < privateKey_.size(); ++i)
        {
            combined += static_cast<char>(privateKey_[i]);
        }
        for (size_t i = 0; i < data.size(); ++i)
        {
            combined += static_cast<char>(data[i]);
        }
        
        size_t hashValue = hasher(combined);
        
        // Fill signature with hash-derived bytes
        for (size_t i = 0; i < 64; ++i)
        {
            signature[i] = static_cast<uint8_t>((hashValue >> (i % 8)) & 0xFF);
        }
        
        return signature;
    }

    bool KeyPair::Verify(const io::ByteVector& data, const io::ByteVector& signature) const
    {
        if (signature.size() != 64)
        {
            return false;
        }
        
        // Simplified verification - recreate signature and compare
        auto expectedSignature = Sign(data);
        
        // Compare signatures
        for (size_t i = 0; i < 64; ++i)
        {
            if (signature[i] != expectedSignature[i])
            {
                return false;
            }
        }
        
        return true;
    }

    std::string KeyPair::ToWIF() const
    {
        // Simplified WIF encoding - in production use proper Base58 encode
        std::string result = "K";
        
        for (size_t i = 0; i < privateKey_.size(); ++i)
        {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", privateKey_[i]);
            result += hex;
        }
        
        return result;
    }

    std::unique_ptr<KeyPair> KeyPair::Generate()
    {
        io::ByteVector privateKey(32);
        
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(1, 255);
        
        bool validKey = false;
        while (!validKey)
        {
            for (size_t i = 0; i < 32; ++i)
            {
                privateKey[i] = static_cast<uint8_t>(dis(gen));
            }
            
            validKey = IsValidPrivateKey(privateKey);
        }
        
        return std::make_unique<KeyPair>(privateKey);
    }

    std::unique_ptr<KeyPair> KeyPair::FromWIF(const std::string& wif)
    {
        if (wif.length() < 51 || wif.length() > 52)
        {
            throw std::invalid_argument("Invalid WIF length");
        }
        
        // Simplified WIF decoding - in production use proper Base58 decode
        io::ByteVector privateKey(32);
        
        std::hash<std::string> hasher;
        size_t hashValue = hasher(wif);
        
        for (size_t i = 0; i < 32; ++i)
        {
            privateKey[i] = static_cast<uint8_t>((hashValue >> (i % 8)) & 0xFF);
        }
        
        if (!IsValidPrivateKey(privateKey))
        {
            throw std::invalid_argument("WIF produces invalid private key");
        }
        
        return std::make_unique<KeyPair>(privateKey);
    }

    KeyPair KeyPair::FromHex(const std::string& hex)
    {
        auto private_key = Helper::FromHexString(hex);
        return KeyPair(private_key);
    }

    std::string KeyPair::ToHex() const
    {
        return Helper::ToHexString(privateKey_.AsSpan());
    }

    bool KeyPair::IsValid() const
    {
        return ValidatePrivateKey(privateKey_);
    }

    bool KeyPair::operator==(const KeyPair& other) const
    {
        return privateKey_ == other.privateKey_;
    }

    bool KeyPair::operator!=(const KeyPair& other) const
    {
        return !(*this == other);
    }

    bool KeyPair::ValidatePrivateKey(const io::ByteVector& private_key)
    {
        if (private_key.size() != 32)
        {
            return false;
        }
        
        // Check if private key is zero
        bool all_zero = std::all_of(private_key.begin(), private_key.end(), [](uint8_t b) { return b == 0; });
        if (all_zero)
        {
            return false;
        }
        
        // Additional validation could be added here
        return true;
    }

    void KeyPair::ComputePublicKey() const
    {
        // Simplified public key generation - in production use real ECC
        io::ByteVector pubKeyBytes(33);
        
        // Set compressed public key prefix
        pubKeyBytes[0] = 0x02;
        
        // Generate deterministic public key from private key
        std::hash<std::string> hasher;
        std::string privKeyStr;
        for (size_t i = 0; i < privateKey_.size(); ++i)
        {
            privKeyStr += static_cast<char>(privateKey_[i]);
        }
        
        size_t hashValue = hasher(privKeyStr);
        
        // Fill public key with hash-derived bytes
        for (size_t i = 1; i < 33; ++i)
        {
            pubKeyBytes[i] = static_cast<uint8_t>((hashValue >> ((i-1) % 8)) & 0xFF);
        }
        
        // Use FromBytes static method instead of constructor
        auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubKeyBytes.AsSpan());
        publicKey_ = std::make_unique<cryptography::ecc::ECPoint>(std::move(ecPoint));
    }

    void KeyPair::ComputeScriptHash() const
    {
        scriptHash_ = std::make_unique<io::UInt160>(Helper::GetScriptHash(GetPublicKey()));
    }

    void KeyPair::Clear()
    {
        // Clear sensitive data
        std::fill(privateKey_.begin(), privateKey_.end(), 0);
        publicKey_.reset();
        scriptHash_.reset();
    }

    bool KeyPair::IsValidPrivateKey(const io::ByteVector& privateKey)
    {
        if (privateKey.size() != 32)
        {
            return false;
        }
        
        // Check if private key is zero
        bool allZero = true;
        for (size_t i = 0; i < 32; ++i)
        {
            if (privateKey[i] != 0)
            {
                allZero = false;
                break;
            }
        }
        
        if (allZero)
        {
            return false;
        }
        
        // Check if private key is all 0xFF (invalid)
        bool allFF = true;
        for (size_t i = 0; i < 32; ++i)
        {
            if (privateKey[i] != 0xFF)
            {
                allFF = false;
                break;
            }
        }
        
        return !allFF;
    }

    std::string KeyPair::Base58Encode(const io::ByteVector& data)
    {
        // Simplified Base58 encoding
        static const char* alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        
        std::string result;
        
        // Simple encoding - in production use proper Base58 algorithm
        for (size_t i = 0; i < data.size(); ++i)
        {
            result += alphabet[data[i] % 58];
        }
        
        return result;
    }
}
