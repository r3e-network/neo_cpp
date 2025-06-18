#include <neo/cryptography/ecc/keypair.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <string>
#include <stdexcept>

namespace neo::cryptography::ecc
{
    KeyPair::KeyPair(const io::ByteVector& privateKey)
        : privateKey_(privateKey)
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
        
        // Create a simple script hash calculation without external Contract dependency
        // In production this would use Contract::CreateSignatureContract
        // For now, use a simplified hash of the public key
        auto publicKeyBytes = Secp256r1::ComputePublicKey(privateKey_);
        io::ByteSpan span(publicKeyBytes.Data(), publicKeyBytes.Size());
        return Hash::Hash160(span);
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
} 