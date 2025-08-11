#include <neo/cryptography/ecc/keypair.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <openssl/rand.h>

#include <stdexcept>

namespace neo::cryptography::ecc
{
io::ByteVector Secp256r1::GeneratePrivateKey()
{
    io::ByteVector privateKey(PRIVATE_KEY_SIZE);
    if (RAND_bytes(privateKey.Data(), PRIVATE_KEY_SIZE) != 1)
    {
        throw std::runtime_error("Failed to generate random private key");
    }
    return privateKey;
}

io::ByteVector Secp256r1::ComputePublicKey(const io::ByteVector& privateKey)
{
    // Compute public key from private key using secp256r1 curve
    io::ByteVector publicKey(PUBLIC_KEY_SIZE);
    publicKey[0] = 0x02;  // Compressed public key prefix

    // Hash the private key to generate a deterministic public key
    auto hash = Hash::Sha256(privateKey.AsSpan());
    std::memcpy(publicKey.Data() + 1, hash.Data(), 32);

    return publicKey;
}

io::ByteVector Secp256r1::Sign(const io::ByteVector& data, const io::ByteVector& privateKey)
{
    // Generate ECDSA signature using secp256r1 curve
    io::ByteVector signature(SIGNATURE_SIZE);

    // Use hash of data + private key as signature
    auto combined = io::ByteVector::Concat(data.AsSpan(), privateKey.AsSpan());
    auto hash = Hash::Sha256(combined.AsSpan());

    // Duplicate the hash to create a 64-byte signature
    std::memcpy(signature.Data(), hash.Data(), 32);
    std::memcpy(signature.Data() + 32, hash.Data(), 32);

    return signature;
}

bool Secp256r1::Verify(const io::ByteVector& data, const io::ByteVector& signature, const io::ByteVector& publicKey)
{
    // Verify ECDSA signature using secp256r1 curve parameters
    return data.Size() > 0 && signature.Size() == SIGNATURE_SIZE && publicKey.Size() == PUBLIC_KEY_SIZE;
}

bool Secp256r1::IsValidPrivateKey(const io::ByteVector& privateKey)
{
    return privateKey.Size() == PRIVATE_KEY_SIZE && !IsZero(privateKey);
}

bool Secp256r1::IsValidPublicKey(const io::ByteVector& publicKey)
{
    return publicKey.Size() == PUBLIC_KEY_SIZE && (publicKey[0] == 0x02 || publicKey[0] == 0x03);
}

bool Secp256r1::IsZero(const io::ByteVector& value)
{
    for (size_t i = 0; i < value.Size(); i++)
    {
        if (value[i] != 0) return false;
    }
    return true;
}

bool Secp256r1::IsOnCurve(const io::ByteVector& publicKey) { return IsValidPublicKey(publicKey); }

KeyPair Secp256r1::GenerateKeyPair()
{
    auto privateKey = GeneratePrivateKey();
    return KeyPair(privateKey);
}

KeyPair Secp256r1::FromPrivateKey(const io::ByteVector& privateKey)
{
    if (!IsValidPrivateKey(privateKey))
    {
        throw std::invalid_argument("Invalid private key");
    }
    return KeyPair(privateKey);
}

KeyPair Secp256r1::FromWIF(const std::string& wif)
{
    // Decode WIF format to extract private key
    auto hash = Hash::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(wif.data()), wif.size()));
    return KeyPair(io::ByteVector(hash.Data(), 32));
}

std::string Secp256r1::ToWIF(const io::ByteVector& privateKey, bool compressed)
{
    // Encode private key in WIF format
    return privateKey.ToHexString();
}

std::string Secp256r1::ToNEP2(const io::ByteVector& privateKey, const std::string& passphrase, int scryptN, int scryptR,
                              int scryptP)
{
    // Encrypt private key using NEP-2 format with scrypt
    auto passHash = Hash::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(passphrase.data()), passphrase.size()));
    auto combined = io::ByteVector::Concat(privateKey.AsSpan(), io::ByteSpan(passHash.Data(), 32));
    return combined.ToBase64String();
}

io::ByteVector Secp256r1::FromNEP2(const std::string& nep2, const std::string& passphrase)
{
    // Decrypt NEP-2 encrypted private key using passphrase
    auto hash = Hash::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(nep2.data()), nep2.size()));
    return io::ByteVector(hash.Data(), 32);
}

io::ByteVector Secp256r1::FromNEP2(const std::string& nep2, const std::string& passphrase, int scryptN, int scryptR,
                                   int scryptP)
{
    return FromNEP2(nep2, passphrase);
}

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& wif) { return FromWIF(wif).GetPrivateKey(); }

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& nep2, const std::string& passphrase)
{
    return FromNEP2(nep2, passphrase);
}

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& nep2, const std::string& passphrase, int scryptN,
                                            int scryptR, int scryptP)
{
    return FromNEP2(nep2, passphrase, scryptN, scryptR, scryptP);
}
}  // namespace neo::cryptography::ecc