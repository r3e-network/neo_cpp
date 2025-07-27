#include <neo/cryptography/ecc/keypair.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <stdexcept>
#include <string>

namespace neo::cryptography::ecc
{
KeyPair::KeyPair(const io::ByteVector& privateKey) : privateKey_(privateKey)
{
    if (!Secp256r1::IsValidPrivateKey(privateKey))
        throw std::invalid_argument("Invalid private key");
}

std::unique_ptr<KeyPair> KeyPair::Generate()
{
    auto privateKey = Secp256r1::GeneratePrivateKey();
    return std::make_unique<KeyPair>(privateKey);
}

std::unique_ptr<KeyPair> KeyPair::FromWIF(const std::string& wif)
{
    auto keyPair = Secp256r1::FromWIF(wif);
    return std::make_unique<KeyPair>(keyPair.GetPrivateKey());
}

const io::ByteVector& KeyPair::GetPrivateKey() const
{
    return privateKey_;
}

const ECPoint& KeyPair::GetPublicKey() const
{
    if (!publicKey_)
    {
        ComputePublicKey();
    }
    return *publicKey_;
}

io::UInt160 KeyPair::GetScriptHash() const
{
    // Calculate script hash from public key using standard Neo method
    auto publicKey = GetPublicKey();

    // Complete signature contract script hash generation (Neo standard)
    // Create proper signature verification script: PUSH(pubkey) + CHECKSIG
    try
    {
        auto publicKeyBytes = Secp256r1::ComputePublicKey(privateKey_);

        // Build the signature verification script
        io::ByteVector script;

        // Add PUSH33 opcode (0x21) to push 33-byte public key
        script.Push(0x21);

        // Add the 33-byte compressed public key
        for (size_t i = 0; i < publicKeyBytes.Size(); ++i)
        {
            script.Push(publicKeyBytes[i]);
        }

        // Add CHECKSIG opcode (0x41)
        script.Push(0x41);

        // Calculate Hash160 of the verification script (this is the standard Neo method)
        io::ByteSpan scriptSpan(script.Data(), script.Size());
        return Hash::Hash160(scriptSpan);
    }
    catch (const std::exception& e)
    {
        // Enhanced error recovery with proper script construction
        LOG_ERROR("Primary script creation failed: {}. Attempting alternative construction method.", e.what());

        try
        {
            // Alternative method: manually construct the verification script
            auto publicKeyBytes = Secp256r1::ComputePublicKey(privateKey_);

            // Ensure we have a valid 33-byte compressed public key
            if (publicKeyBytes.Size() != 33 || (publicKeyBytes[0] != 0x02 && publicKeyBytes[0] != 0x03))
            {
                throw std::runtime_error("Invalid public key format for script construction");
            }

            // Manually construct the verification script: PUSH(33) + publicKey + CHECKSIG
            std::vector<uint8_t> verificationScript;
            verificationScript.reserve(35);  // 1 + 33 + 1

            // Push opcode for 33 bytes (PUSHDATA1 0x21)
            verificationScript.push_back(0x21);

            // Add public key bytes
            for (size_t i = 0; i < publicKeyBytes.Size(); ++i)
            {
                verificationScript.push_back(publicKeyBytes[i]);
            }

            // Add CHECKSIG opcode (0x41)
            verificationScript.push_back(0x41);

            // Calculate Hash160 of the verification script
            io::ByteSpan scriptSpan(verificationScript.data(), verificationScript.size());
            auto result = Hash::Hash160(scriptSpan);

            LOG_DEBUG("Successfully created script hash using alternative method");
            return result;
        }
        catch (const std::exception& innerE)
        {
            LOG_ERROR("Alternative script construction also failed: {}. This indicates a serious cryptographic error.",
                      innerE.what());
            throw std::runtime_error("Failed to generate script hash: " + std::string(innerE.what()));
        }
    }
}

std::string KeyPair::ToWIF() const
{
    return Secp256r1::ToWIF(privateKey_);
}

io::ByteVector KeyPair::Sign(const io::ByteVector& data) const
{
    return Secp256r1::Sign(data, privateKey_);
}

bool KeyPair::Verify(const io::ByteVector& data, const io::ByteVector& signature) const
{
    auto publicKeyBytes = Secp256r1::ComputePublicKey(privateKey_);
    return Secp256r1::Verify(data, signature, publicKeyBytes);
}

void KeyPair::ComputePublicKey() const
{
    auto publicKeyBytes = Secp256r1::ComputePublicKey(privateKey_);
    io::ByteSpan span(publicKeyBytes.Data(), publicKeyBytes.Size());
    auto ecPoint = ECPoint::FromBytes(span, "secp256r1");
    publicKey_ = std::make_unique<ECPoint>(std::move(ecPoint));
}
}  // namespace neo::cryptography::ecc