#include <neo/wallets/helper.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/crypto.h>
#include <neo/vm/opcode.h>
#include <stdexcept>
#include <algorithm>
#include <random>

namespace neo::wallets
{
    std::string Helper::ToAddress(const io::UInt160& script_hash, uint8_t address_version)
    {
        std::vector<uint8_t> data;
        data.reserve(21);
        data.push_back(address_version);
        
        auto hash_bytes = script_hash.ToArray();
        data.insert(data.end(), hash_bytes.begin(), hash_bytes.end());
        
        return Base58CheckEncode(data);
    }

    io::UInt160 Helper::ToScriptHash(const std::string& address, uint8_t address_version)
    {
        try
        {
            auto decoded = Base58CheckDecode(address);
            if (decoded.size() != 21 || decoded[0] != address_version)
            {
                throw std::invalid_argument("Invalid address format");
            }
            
            std::vector<uint8_t> hash_bytes(decoded.begin() + 1, decoded.end());
            return io::UInt160(hash_bytes);
        }
        catch (...)
        {
            throw std::invalid_argument("Invalid address");
        }
    }

    std::vector<uint8_t> Helper::CreateSignatureScript(const cryptography::ecc::ECPoint& public_key)
    {
        std::vector<uint8_t> script;
        
        // PUSHDATA1 + public key length + public key + PUSHNULL + SYSCALL + CheckWitness
        auto pub_key_bytes = public_key.EncodePoint(true);
        
        script.push_back(static_cast<uint8_t>(vm::OpCode::PUSHDATA1));
        script.push_back(static_cast<uint8_t>(pub_key_bytes.size()));
        script.insert(script.end(), pub_key_bytes.begin(), pub_key_bytes.end());
        script.push_back(static_cast<uint8_t>(vm::OpCode::PUSHNULL));
        script.push_back(static_cast<uint8_t>(vm::OpCode::SYSCALL));
        
        // CheckWitness syscall hash
        std::vector<uint8_t> syscall_hash = {0x41, 0x9f, 0xd0, 0x96};
        script.insert(script.end(), syscall_hash.begin(), syscall_hash.end());
        
        return script;
    }

    std::vector<uint8_t> Helper::CreateMultiSigScript(int m, const std::vector<cryptography::ecc::ECPoint>& public_keys)
    {
        if (m < 1 || m > static_cast<int>(public_keys.size()) || public_keys.size() > 1024)
        {
            throw std::invalid_argument("Invalid multi-signature parameters");
        }
        
        std::vector<uint8_t> script;
        
        // Push m
        script.push_back(static_cast<uint8_t>(vm::OpCode::PUSH1) + m - 1);
        
        // Push public keys
        for (const auto& pub_key : public_keys)
        {
            auto pub_key_bytes = pub_key.EncodePoint(true);
            script.push_back(static_cast<uint8_t>(vm::OpCode::PUSHDATA1));
            script.push_back(static_cast<uint8_t>(pub_key_bytes.size()));
            script.insert(script.end(), pub_key_bytes.begin(), pub_key_bytes.end());
        }
        
        // Push n
        script.push_back(static_cast<uint8_t>(vm::OpCode::PUSH1) + static_cast<uint8_t>(public_keys.size()) - 1);
        
        // PUSHNULL + SYSCALL + CheckWitness
        script.push_back(static_cast<uint8_t>(vm::OpCode::PUSHNULL));
        script.push_back(static_cast<uint8_t>(vm::OpCode::SYSCALL));
        
        // CheckWitness syscall hash
        std::vector<uint8_t> syscall_hash = {0x41, 0x9f, 0xd0, 0x96};
        script.insert(script.end(), syscall_hash.begin(), syscall_hash.end());
        
        return script;
    }

    io::UInt160 Helper::ToScriptHash(std::span<const uint8_t> script)
    {
        auto hash = cryptography::Hash::Hash160(script);
        return hash;
    }

    bool Helper::IsValidAddress(const std::string& address, uint8_t address_version)
    {
        try
        {
            ToScriptHash(address, address_version);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    std::vector<uint8_t> Helper::Sign(std::span<const uint8_t> message, std::span<const uint8_t> private_key)
    {
        return cryptography::Crypto::Sign(message, private_key);
    }

    bool Helper::VerifySignature(std::span<const uint8_t> message, 
                                std::span<const uint8_t> signature, 
                                const cryptography::ecc::ECPoint& public_key)
    {
        return public_key.VerifySignature(message, signature);
    }

    std::vector<uint8_t> Helper::GeneratePrivateKey()
    {
        std::vector<uint8_t> private_key(32);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (auto& byte : private_key)
        {
            byte = dis(gen);
        }
        
        return private_key;
    }

    cryptography::ecc::ECPoint Helper::GetPublicKey(std::span<const uint8_t> private_key)
    {
        return cryptography::ecc::ECPoint::FromPrivateKey(private_key);
    }

    io::UInt160 Helper::GetScriptHash(const cryptography::ecc::ECPoint& public_key)
    {
        auto script = CreateSignatureScript(public_key);
        return ToScriptHash(script);
    }

    std::string Helper::ToHexString(std::span<const uint8_t> data, bool reverse)
    {
        std::string result;
        result.reserve(data.size() * 2);
        
        if (reverse)
        {
            for (auto it = data.rbegin(); it != data.rend(); ++it)
            {
                char hex[3];
                std::sprintf(hex, "%02x", *it);
                result += hex;
            }
        }
        else
        {
            for (uint8_t byte : data)
            {
                char hex[3];
                std::sprintf(hex, "%02x", byte);
                result += hex;
            }
        }
        
        return result;
    }

    std::vector<uint8_t> Helper::FromHexString(const std::string& hex, bool reverse)
    {
        if (hex.length() % 2 != 0)
        {
            throw std::invalid_argument("Invalid hex string length");
        }
        
        std::vector<uint8_t> result;
        result.reserve(hex.length() / 2);
        
        for (size_t i = 0; i < hex.length(); i += 2)
        {
            std::string byte_str = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
            result.push_back(byte);
        }
        
        if (reverse)
        {
            std::reverse(result.begin(), result.end());
        }
        
        return result;
    }

    std::vector<uint8_t> Helper::CalculateChecksum(std::span<const uint8_t> data)
    {
        auto hash = cryptography::Hash::Hash256(data);
        return std::vector<uint8_t>(hash.Data(), hash.Data() + 4);
    }

    std::string Helper::Base58Encode(std::span<const uint8_t> data)
    {
        return cryptography::Base58::Encode(data);
    }

    std::vector<uint8_t> Helper::Base58Decode(const std::string& encoded)
    {
        return cryptography::Base58::Decode(encoded);
    }

    std::string Helper::Base58CheckEncode(std::span<const uint8_t> data)
    {
        std::vector<uint8_t> with_checksum(data.begin(), data.end());
        auto checksum = CalculateChecksum(data);
        with_checksum.insert(with_checksum.end(), checksum.begin(), checksum.end());
        
        return Base58Encode(with_checksum);
    }

    std::vector<uint8_t> Helper::Base58CheckDecode(const std::string& encoded)
    {
        auto decoded = Base58Decode(encoded);
        if (decoded.size() < 4)
        {
            throw std::invalid_argument("Invalid base58check data");
        }
        
        std::vector<uint8_t> data(decoded.begin(), decoded.end() - 4);
        std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());
        
        if (!ValidateChecksum(decoded))
        {
            throw std::invalid_argument("Invalid checksum");
        }
        
        return data;
    }

    bool Helper::ValidateChecksum(std::span<const uint8_t> data)
    {
        if (data.size() < 4)
        {
            return false;
        }
        
        std::vector<uint8_t> payload(data.begin(), data.end() - 4);
        std::vector<uint8_t> checksum(data.end() - 4, data.end());
        
        auto calculated_checksum = CalculateChecksum(payload);
        
        return std::equal(checksum.begin(), checksum.end(), calculated_checksum.begin());
    }
}
