#include <algorithm>
#include <cstring>
#include <functional>
#include <iomanip>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/hash.h>
#include <neo/vm/opcode.h>
#include <neo/wallets/helper.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace neo::wallets
{
std::string Helper::ToAddress(const io::UInt160& script_hash, uint8_t address_version)
{
    io::ByteVector data;
    data.Push(address_version);

    auto hash_bytes = script_hash.ToArray();
    for (size_t i = 0; i < hash_bytes.Size(); ++i)
    {
        data.Push(hash_bytes[i]);
    }

    return Base58CheckEncode(data.AsSpan());
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

        io::ByteVector hash_bytes;
        for (size_t i = 1; i < decoded.size(); ++i)
        {
            hash_bytes.Push(decoded[i]);
        }
        return io::UInt160(hash_bytes.AsSpan());
    }
    catch (...)
    {
        throw std::invalid_argument("Invalid address");
    }
}

std::vector<uint8_t> Helper::CreateSignatureScript(const neo::cryptography::ecc::ECPoint& public_key)
{
    std::vector<uint8_t> script;

    // PUSHDATA1 + public key length + public key + PUSHNULL + SYSCALL + CheckWitness
    auto pub_key_bytes = public_key.ToBytes(true);

    script.push_back(static_cast<uint8_t>(vm::OpCode::PUSHDATA1));
    script.push_back(static_cast<uint8_t>(pub_key_bytes.Size()));
    for (size_t i = 0; i < pub_key_bytes.Size(); ++i)
    {
        script.push_back(pub_key_bytes[i]);
    }
    script.push_back(static_cast<uint8_t>(vm::OpCode::PUSHNULL));
    script.push_back(static_cast<uint8_t>(vm::OpCode::SYSCALL));

    // CheckWitness syscall hash
    std::vector<uint8_t> syscall_hash = {0x41, 0x9f, 0xd0, 0x96};
    script.insert(script.end(), syscall_hash.begin(), syscall_hash.end());

    return script;
}

std::vector<uint8_t> Helper::CreateMultiSigScript(int m,
                                                  const std::vector<neo::cryptography::ecc::ECPoint>& public_keys)
{
    if (m < 1 || m > static_cast<int>(public_keys.size()) || public_keys.size() > 1024)
    {
        throw std::invalid_argument("Invalid multi-signature parameters");
    }

    std::vector<uint8_t> script;

    // Push m
    script.push_back(static_cast<uint8_t>(vm::OpCode::PUSH1) + static_cast<uint8_t>(m - 1));

    // Push public keys
    for (const auto& pub_key : public_keys)
    {
        auto pub_key_bytes = pub_key.ToBytes(true);
        script.push_back(static_cast<uint8_t>(vm::OpCode::PUSHDATA1));
        script.push_back(static_cast<uint8_t>(pub_key_bytes.Size()));
        for (size_t i = 0; i < pub_key_bytes.Size(); ++i)
        {
            script.push_back(pub_key_bytes[i]);
        }
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

io::UInt160 Helper::ToScriptHash(const uint8_t* script_data, size_t script_size)
{
    // Convert to ByteSpan for Hash method
    io::ByteSpan byteSpan(script_data, script_size);
    auto hash = cryptography::Hash::Hash160(byteSpan);
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

std::vector<uint8_t> Helper::Sign(const uint8_t* message, size_t message_size, const uint8_t* private_key,
                                  size_t private_key_size)
{
    // Use the cryptography module for signing
    io::ByteSpan messageSpan(message, message_size);
    io::ByteSpan privateKeySpan(private_key, private_key_size);

    return cryptography::Crypto::Sign(messageSpan, privateKeySpan);
}

bool Helper::VerifySignature(const uint8_t* message, size_t message_size, const uint8_t* signature,
                             size_t signature_size, const neo::cryptography::ecc::ECPoint& public_key)
{
    // Use the Crypto class for signature verification
    io::ByteSpan messageSpan(message, message_size);
    io::ByteSpan signatureSpan(signature, signature_size);
    return cryptography::Crypto::VerifySignature(messageSpan, signatureSpan, public_key);
}

std::vector<uint8_t> Helper::GeneratePrivateKey()
{
    std::vector<uint8_t> private_key(32);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dis(0, 255);

    for (auto& byte : private_key)
    {
        byte = static_cast<uint8_t>(dis(gen));
    }

    return private_key;
}

neo::cryptography::ecc::ECPoint Helper::GetPublicKey(const uint8_t* private_key, size_t private_key_size)
{
    if (private_key_size != 32)
    {
        throw std::invalid_argument("Private key must be 32 bytes");
    }

    // Use cryptography module to derive public key
    io::ByteSpan privateKeySpan(private_key, private_key_size);
    return cryptography::Crypto::ComputePublicKey(privateKeySpan);
}

io::UInt160 Helper::GetScriptHash(const neo::cryptography::ecc::ECPoint& public_key)
{
    auto script = CreateSignatureScript(public_key);
    return ToScriptHash(script.data(), script.size());
}

std::string Helper::ToHexString(const uint8_t* data, size_t data_size, bool reverse)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    if (reverse)
    {
        for (size_t i = data_size; i > 0; --i)
        {
            ss << std::setw(2) << static_cast<unsigned int>(data[i - 1]);
        }
    }
    else
    {
        for (size_t i = 0; i < data_size; ++i)
        {
            ss << std::setw(2) << static_cast<unsigned int>(data[i]);
        }
    }

    return ss.str();
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

std::vector<uint8_t> Helper::CalculateChecksum(const uint8_t* data, size_t data_size)
{
    // Convert to ByteSpan for Hash method
    io::ByteSpan byteSpan(data, data_size);
    auto hash = cryptography::Hash::Hash256(byteSpan);
    return std::vector<uint8_t>(hash.Data(), hash.Data() + 4);
}

std::string Helper::Base58Encode(const uint8_t* data, size_t data_size)
{
    // Convert to ByteVector for Base58 method
    io::ByteVector byteVector;
    for (size_t i = 0; i < data_size; ++i)
    {
        byteVector.Push(data[i]);
    }
    return cryptography::Base58::Encode(byteVector);
}

std::vector<uint8_t> Helper::Base58Decode(const std::string& encoded)
{
    return cryptography::Base58::Decode(encoded);
}

std::string Helper::Base58CheckEncode(const uint8_t* data, size_t data_size)
{
    std::vector<uint8_t> with_checksum(data, data + data_size);
    auto checksum = CalculateChecksum(data, data_size);
    with_checksum.insert(with_checksum.end(), checksum.begin(), checksum.end());

    return Base58Encode(with_checksum.data(), with_checksum.size());
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

    if (!ValidateChecksum(decoded.data(), decoded.size()))
    {
        throw std::invalid_argument("Invalid checksum");
    }

    return data;
}

bool Helper::ValidateChecksum(const uint8_t* data, size_t data_size)
{
    if (data_size < 4)
    {
        return false;
    }

    std::vector<uint8_t> payload(data, data + data_size - 4);
    std::vector<uint8_t> checksum(data + data_size - 4, data + data_size);

    auto calculated_checksum = CalculateChecksum(payload.data(), payload.size());

    return std::equal(checksum.begin(), checksum.end(), calculated_checksum.begin());
}

// Convenience overloads for span-like usage
io::UInt160 Helper::ToScriptHash(io::ByteSpan script)
{
    return ToScriptHash(script.Data(), script.Size());
}

std::vector<uint8_t> Helper::Sign(io::ByteSpan message, io::ByteSpan private_key)
{
    return Sign(message.Data(), message.Size(), private_key.Data(), private_key.Size());
}

bool Helper::VerifySignature(io::ByteSpan message, io::ByteSpan signature,
                             const neo::cryptography::ecc::ECPoint& public_key)
{
    return VerifySignature(message.Data(), message.Size(), signature.Data(), signature.Size(), public_key);
}

neo::cryptography::ecc::ECPoint Helper::GetPublicKey(io::ByteSpan private_key)
{
    return GetPublicKey(private_key.Data(), private_key.Size());
}

std::string Helper::ToHexString(io::ByteSpan data, bool reverse)
{
    return ToHexString(data.Data(), data.Size(), reverse);
}

std::vector<uint8_t> Helper::CalculateChecksum(io::ByteSpan data)
{
    return CalculateChecksum(data.Data(), data.Size());
}

std::string Helper::Base58Encode(io::ByteSpan data)
{
    return Base58Encode(data.Data(), data.Size());
}

std::string Helper::Base58CheckEncode(io::ByteSpan data)
{
    return Base58CheckEncode(data.Data(), data.Size());
}

bool Helper::ValidateChecksum(io::ByteSpan data)
{
    return ValidateChecksum(data.Data(), data.Size());
}
}  // namespace neo::wallets
