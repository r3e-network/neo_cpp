#include <neo/wallets/key_pair.h>
#include <neo/wallets/helper.h>
#include <neo/cryptography/crypto.h>
#include <stdexcept>
#include <algorithm>

namespace neo::wallets
{
    KeyPair::KeyPair(std::span<const uint8_t> private_key)
        : private_key_(private_key.begin(), private_key.end())
    {
        if (!ValidatePrivateKey(private_key))
        {
            throw std::invalid_argument("Invalid private key");
        }
    }

    KeyPair::KeyPair(const std::vector<uint8_t>& private_key)
        : private_key_(private_key)
    {
        if (!ValidatePrivateKey(private_key_))
        {
            throw std::invalid_argument("Invalid private key");
        }
    }

    KeyPair::~KeyPair()
    {
        Clear();
    }

    KeyPair::KeyPair(const KeyPair& other)
        : private_key_(other.private_key_)
    {
        // Public key and script hash will be computed lazily
    }

    KeyPair::KeyPair(KeyPair&& other) noexcept
        : private_key_(std::move(other.private_key_)),
          public_key_(std::move(other.public_key_)),
          script_hash_(std::move(other.script_hash_))
    {
        other.Clear();
    }

    KeyPair& KeyPair::operator=(const KeyPair& other)
    {
        if (this != &other)
        {
            Clear();
            private_key_ = other.private_key_;
            // Public key and script hash will be computed lazily
        }
        return *this;
    }

    KeyPair& KeyPair::operator=(KeyPair&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            private_key_ = std::move(other.private_key_);
            public_key_ = std::move(other.public_key_);
            script_hash_ = std::move(other.script_hash_);
            other.Clear();
        }
        return *this;
    }

    std::vector<uint8_t> KeyPair::GetPrivateKey() const
    {
        return private_key_;
    }

    cryptography::ecc::ECPoint KeyPair::GetPublicKey() const
    {
        if (!public_key_)
        {
            ComputePublicKey();
        }
        return *public_key_;
    }

    io::UInt160 KeyPair::GetScriptHash() const
    {
        if (!script_hash_)
        {
            ComputeScriptHash();
        }
        return *script_hash_;
    }

    std::string KeyPair::GetAddress(uint8_t address_version) const
    {
        return Helper::ToAddress(GetScriptHash(), address_version);
    }

    std::vector<uint8_t> KeyPair::Sign(std::span<const uint8_t> message) const
    {
        return Helper::Sign(message, private_key_);
    }

    bool KeyPair::VerifySignature(std::span<const uint8_t> message, std::span<const uint8_t> signature) const
    {
        return Helper::VerifySignature(message, signature, GetPublicKey());
    }

    std::string KeyPair::ToWIF() const
    {
        std::vector<uint8_t> data;
        data.reserve(34);
        
        // Add version byte (0x80 for mainnet)
        data.push_back(0x80);
        
        // Add private key
        data.insert(data.end(), private_key_.begin(), private_key_.end());
        
        // Add compression flag
        data.push_back(0x01);
        
        return Helper::Base58CheckEncode(data);
    }

    KeyPair KeyPair::FromWIF(const std::string& wif)
    {
        try
        {
            auto decoded = Helper::Base58CheckDecode(wif);
            
            if (decoded.size() != 34 || decoded[0] != 0x80 || decoded[33] != 0x01)
            {
                throw std::invalid_argument("Invalid WIF format");
            }
            
            std::vector<uint8_t> private_key(decoded.begin() + 1, decoded.begin() + 33);
            return KeyPair(private_key);
        }
        catch (...)
        {
            throw std::invalid_argument("Invalid WIF");
        }
    }

    KeyPair KeyPair::Generate()
    {
        auto private_key = Helper::GeneratePrivateKey();
        return KeyPair(private_key);
    }

    KeyPair KeyPair::FromHex(const std::string& hex)
    {
        auto private_key = Helper::FromHexString(hex);
        return KeyPair(private_key);
    }

    std::string KeyPair::ToHex() const
    {
        return Helper::ToHexString(private_key_);
    }

    bool KeyPair::IsValid() const
    {
        return ValidatePrivateKey(private_key_);
    }

    bool KeyPair::operator==(const KeyPair& other) const
    {
        return private_key_ == other.private_key_;
    }

    bool KeyPair::operator!=(const KeyPair& other) const
    {
        return !(*this == other);
    }

    bool KeyPair::ValidatePrivateKey(std::span<const uint8_t> private_key)
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
        public_key_ = std::make_unique<cryptography::ecc::ECPoint>(Helper::GetPublicKey(private_key_));
    }

    void KeyPair::ComputeScriptHash() const
    {
        script_hash_ = std::make_unique<io::UInt160>(Helper::GetScriptHash(GetPublicKey()));
    }

    void KeyPair::Clear()
    {
        // Clear sensitive data
        std::fill(private_key_.begin(), private_key_.end(), 0);
        public_key_.reset();
        script_hash_.reset();
    }
}
